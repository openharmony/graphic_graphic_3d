# LumeScene AGENTS.md

This document describes how to extract ECS component interfaces to the upper scene layer in LumeScene.

## Module Dependencies

```
LumeScene
  ├── Lume3D          — ECS components (Transform, Camera, Light, etc.), graphics context
  ├── LumeRender      — render context, renderer
  ├── LumeEngine      — core ECS (IEcs, IComponentManager, Entity), plugin system
  ├── LumeMeta        — META property system, object registry, reflection, serialization
  └── LumeBase        — containers, math, strings, memory
```

## Architecture Overview

LumeScene wraps raw ECS data into high-level, typed, property-driven interfaces. The architecture has three layers:

```
ECS IComponentManager (raw engine components)
        |
        v
  IEcsObject / EcsObject (engine value bridge via IEngineValue)
        |
        v
  Component subclasses (typed scene components)
        |
        v
  Node subclasses (scene nodes with forwarded properties)
```

The central bridge is `EcsObject` (`src/core/ecs_object.h`), which creates `IEngineValue` bindings from ECS component managers and exposes them as META properties.

## Thread Safety

LumeScene uses a **task-dispatch threading model** rather than broad mutex-based protection. The primary mechanism is routing operations to the correct thread via `AddTaskOrRunDirectly()` / `RunDirectlyOrInTask()`. The ECS/render thread owns all ECS entity/component operations.

### Thread-Safe (internally protected with locks/atomics)

| Class | Mechanism | Scope |
|-------|-----------|-------|
| `RenderResource` | `std::shared_mutex` | Render handle read/write |
| `Shader` / `GraphicsState` | `shared_mutex` (inherited) | Graphics state + render handle |
| `Image` | `shared_mutex` (inherited) | Render handle |
| `Mesh` | `std::mutex` | Material override state |
| `RenderResourceManager` | Static `std::mutex` (pool + pending) | Deferred image loading |
| `RenderContext` | `std::atomic<int32_t>` + `thread_local` | Scene load scope tracking |
| `Material` | `std::atomic<bool>` | Metadata update scheduling flag |
| `InternalScene` | `std::shared_mutex` | **Partial only**: `syncs_`, `pendingRender_`, `groups_`. Does NOT protect `nodes_`, `componentFactories_`, or ECS operations. |
| `SubMesh` (read path) | No lock, but documented safe | `GetMaterialOverride()` can be called from any thread |

### Thread-Safe via Task Dispatch (public API dispatches to ECS thread)

| Class | Pattern |
|-------|---------|
| `SceneObject` (IScene impl) | All public API methods use `AddTaskOrRunDirectly()` |
| `Node` (base) | All public API methods use `AddTaskOrRunDirectly()` |
| `CameraComponent` | Task dispatch for SetRenderTarget, raycasts |
| `EcsObject` (impl) | Task dispatch for property operations |
| `MeshNode`, `LightNode`, `LightProbeGroupNode` | Inherited from Node base |
| `MeshCreator`, `Texture` | Task dispatch |
| `Effect`, `Material`, `RenderConfiguration` | `RunDirectlyOrInTask()` |

### NOT Thread-Safe (ECS/engine thread only)

| Class | Evidence |
|-------|----------|
| `Ecs` | Thread ID tracking + `SCENE_ASSERT_THREAD` on all methods (disabled by default: `SCENE_CHECK_ECS_THREAD 0`) |
| `EcsScene` (API) | Explicit docs: "No thread safety is guaranteed" |
| `EcsObject` (API) | Explicit docs: "No thread safety is guaranteed" |
| `EcsObject` (impl internals) | Section labeled `// call in engine thread`; `Build()`, `SyncProperties()`, `GetPath()`, `SetName()` unprotected |
| `InternalScene` (ECS operations) | `nodes_`, `componentFactories_`, `renderingCameras_` NOT under mutex |
| All Component subclasses | Thin ECS wrappers, no locking (TransformComponent, LightComponent, CameraComponent, etc.) |
| `SubMesh` (write path) | `SetMaterialOverride()` documented as "Must be called in engine thread" |
| All PostProcess types | No locking |
| `SceneManager` | No internal locking |

### Caveats

- **ECS thread assertions are disabled by default** (`SCENE_CHECK_ECS_THREAD 0` in `src/core/ecs.cpp`). Thread violations are silently ignored in release builds.
- **Unused `<shared_mutex>` includes** in `CameraComponent` and `Environment` are misleading — neither class uses a mutex member.
- `InternalScene` has a `shared_mutex` but only protects 3 of its many data members. Do not assume all InternalScene operations are thread-safe.

## Three Methods for Extracting ECS Interfaces

### Method 1: Typed Scene Component

Use when an ECS component has a known, stable `IComponentManager` with well-defined properties.

**1. Define ClassId** -- `META_REGISTER_CLASS(MyComponent, "uuid-...")`

**2. Define the component class** -- Inherit from `IntroduceInterfaces<Component, IMyInterface>`:

```cpp
class MyComponent final : public META_NS::IntroduceInterfaces<Component, IMyInterface> {
    META_BEGIN_STATIC_DATA()
        SCENE_STATIC_PROPERTY_DATA(IMyInterface, float, Speed, "MyEcsComponent.speed")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(float, Speed)

public:
    BASE_NS::string GetName() const override { return "MyEcsComponent"; }
};
```

The property macro format is `SCENE_STATIC_PROPERTY_DATA(Interface, Type, PropertyName, "EcsComponentName.fieldName")`. Variants: `SCENE_STATIC_DYNINIT_PROPERTY_DATA` for lazy-init objects, `SCENE_STATIC_ARRAY_PROPERTY_DATA` for arrays.

**3. Register in plugin** (`src/plugin.cpp`): `META_NS::RegisterObjectType<MyComponent>()` in `RegisterInterfaces()`.

**4. Register component factory mapping** (`src/component_factory.h` in `AddBuiltinComponentFactories()`):

```cpp
s->RegisterComponent(IMyEcsComponentManager::UID, ClassId::MyComponent);
```

**5. Register Any types and engine access** (only if new enum/struct types): `src/register_anys.cpp` and `src/register_engine_access.cpp`.

**Reference examples:**
- Minimal: `src/component/transform_component.h` (3 properties)
- Complex: `src/component/camera_component.h` (22+ properties)
- Enums: `src/component/environment_component.h`

### Method 2: Property Forwarding to Scene Nodes

Use when you want ECS component properties to appear directly on a `Node` (e.g., `node.position` instead of `node.transform.position`).

For an existing node, add forwarded properties:
```cpp
// In META_BEGIN_STATIC_DATA block:
META_STATIC_FORWARDED_PROPERTY_DATA(ITransform, Vec3, Position)

// In class body:
SCENE_USE_COMPONENT_PROPERTY(Vec3, Position, "TransformComponent")
```

Macro variants: `SCENE_USE_COMPONENT_PROPERTY` (read-write), `SCENE_USE_READONLY_COMPONENT_PROPERTY` (read-only), `SCENE_USE_ARRAY_COMPONENT_PROPERTY` (array).

For a new specialized node type, create a class inheriting `IntroduceInterfaces<NamedSceneObject, NodeInterface, IMyInterface>` and register with `META_NS::RegisterObjectType<MyNode>()`.

For auto-detection from ECS components, update `DeducePrimaryNodeType()` in `src/core/internal_scene.cpp`.

**Reference examples:**
- `src/node/node.h` — base Node with Position/Rotation/Scale/Enabled/LayerMask forwarding
- `src/node/camera_node.h` — specialized node with 22+ camera properties

### Method 3: Generic Component

Use as a fallback when no typed wrapper exists. No code needed — `InternalScene::AttachComponents()` creates a `GenericComponent` that dynamically discovers all ECS properties via `PopulateAllProperties()`.

Access from JS: `component.property.fieldName`. Lowercase-starting property names are exposed; uppercase-starting are excluded as internal.

## Key Files

| File | Role |
|------|------|
| `src/core/internal_scene.cpp` | Main orchestrator: node construction, component attachment, factory dispatch |
| `src/core/ecs_object.h/.cpp` | Engine value bridge: creates IEngineValue bindings from ECS |
| `src/core/ecs.h/.cpp` | Holds pointers to all ECS component managers |
| `src/component_factory.h` | UID-to-ClassId factory mapping |
| `src/plugin.cpp` | Plugin registration: RegisterObjectType for all types |
| `include/scene/ext/component.h` | Component base class, forwarding macros |
| `include/scene/ext/scene_property.h` | Property construction macros |
| `include/scene/ext/ecs_lazy_property.h` | EcsLazyProperty base, NamedSceneObject |

## Data Flow

```
scene.CreateNode("path")
  -> InternalScene::CreateNode()
    -> Creates ECS Entity with default components
    -> ConstructNodeImpl():
         -> EcsObject wrapper created for entity
         -> AttachComponents():
              -> For each ECS component on entity:
                   -> FindComponentFactory(manager.UID)
                   -> If found: typed Component (Method 1)
                   -> If not found: GenericComponent (Method 3)
         -> Properties forwarded to Node (Method 2)

node.Position
  -> SCENE_USE_COMPONENT_PROPERTY redirects to
     GetComponentProperty(node, "TransformComponent", "Position")
  -> Returns META property bound to ECS via IEngineValue
  -> Read/write syncs to ECS TransformComponent.position
```

## Boundaries

### Always

- Run `clang-format` on all changes. The CMake `SCENE_CLANG_FORMAT_CHECK` option is enabled by default and checks API, Plugin, and Test sources using `clang_format_check.cmake` from LumeBase.

### Ask First

- **Which wrapping method do you need?** Before implementing ECS interface extraction, confirm which of the three methods to use:
  1. **Typed Scene Component** — for known, stable ECS components with well-defined properties
  2. **Property Forwarding to Nodes** — when properties should appear directly on node objects
  3. **Generic Component** — fallback for unknown/plugin-provided ECS components (no code needed)
  
  The choice affects which files need modification and how much boilerplate is required. Methods 1 and 2 can be combined (typed component + node forwarding).

### Never

- **Do NOT call ECS functions (IEcs, IComponentManager, IEntityManager) or InternalScene functions directly from a non-ECS thread.** These are not thread-safe. Accessing them from another thread requires dispatching via `AddTaskOrExecute()` or the scene's task queue mechanism. Direct cross-thread access causes data races.
- Do NOT create ECS entities or modify components outside the ECS thread without proper task scheduling.
