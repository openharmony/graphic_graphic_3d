# ScenePluginAddon AGENTS.md

This document describes how to expose LumeScene interfaces to JavaScript/ArkTS via N-API bindings.

## Module Dependencies

```
ScenePluginAddon (scene3d.node)
  ├── LumeScene::API       — scene interfaces (IScene, INode, ICamera, etc.)
  ├── AGP3D::AGP3DAPI      — Lume3D API
  ├── AGPRender::AGPRenderAPI — render API
  ├── LumeMeta::API        — META property system API
  ├── EcsSerializer        — ECS serialization
  ├── RuntimeUtil          — runtime utilities
  ├── node-api-headers     — N-API v8 (from npm)
  └── [optional] PluginBoidsSwarmAPI — boids swarm plugin API
```

## Architecture Overview

ScenePluginAddon is a N-API native addon (`scene3d.node`) that wraps LumeScene types:

```
TrueRootObject               (native object storage, type tagging, lifetime)
  |
  +-- BaseObject             (NAPI ctor, Finalize/Dispose lifecycle, property macros)
        |
        +-- SceneJS          (Scene wrapper: static load(), factory methods, properties)
        +-- NodeJS           (BaseObject + NodeImpl: position/rotation/scale, children)
        +-- CameraJS         (BaseObject + NodeImpl: fov, near, far, projection)
        +-- SpotLightJS      (BaseObject + BaseLight: color, intensity, shadow)
        +-- DirectionalLightJS
        +-- PointLightJS
        +-- SceneComponentJS (dynamic property auto-discovery)
```

Shared behavior via multiple inheritance: `NodeImpl` (shared node logic), `BaseLight` (shared light logic), `SceneResourceImpl` (shared resource logic).

## Thread Safety

### Thread-Safe (can be called from any thread)

| Class | Mechanism | Protected State |
|-------|-----------|-----------------|
| `NodeJSTaskQueue` | `std::recursive_mutex` + `napi_threadsafe_function` | Task queue, scheduling, ref count |
| `ThreadSafeCallback` (AnimationJS) | `napi_threadsafe_function` | Cross-thread JS callback invocation |
| `JSWrapperState` (JsObjectCache) | Atomic refcount + TSF dispatch | JS reference cleanup |
| `RenderResources` (RenderContextJS) | `CORE_NS::Mutex` | Bitmap cache map |

### NOT Thread-Safe (JS-thread only)

All JS wrapper classes and proxies must only be called from the JS thread. They have no internal synchronization and access NAPI objects.

| Class | Notes |
|-------|-------|
| `BaseObject`, `TrueRootObject` | Base wrappers. `ExecSyncTask()` dispatches to engine thread. |
| `SceneJS` | Has unused `CORE_NS::Mutex` — not actually thread-safe. |
| `NodeJS`, `CameraJS` | Delegate to `NodeImpl`, use `ExecSyncTask`. |
| `SpotLightJS`, `DirectionalLightJS`, `PointLightJS` | No synchronization. |
| `MeshJS`, `MaterialJS`, `BaseMaterial` | No synchronization. |
| `SceneComponentJS` | Dynamic property discovery, no locking. |
| `BloomConfiguration`, `PostProcJS`, `ImageJS` | Use `ExecSyncTask` for engine access. |
| `BoidsSwarmWorldJS` | Heavy `ExecSyncTask` usage for all ECS ops. |
| `AnimationJS` (wrapper part) | Uses `ThreadSafeCallback` for events, but wrapper is JS-thread only. |
| `Promise` | All methods documented "Call only from the JS thread." |
| `Vec3Proxy`, `Vec4Proxy`, `Vec2Proxy`, `QuatProxy`, `ColorProxy` | Comments: "should be executed in the javascript thread." |
| `PropertyProxy`, `ObjectPropertyProxy`, `TypeProxy` | JS-thread bound, no locking. |
| `DisposeContainer` | No synchronization. |
| `RenderContextJS` | Has unused `CORE_NS::Mutex` — not actually thread-safe. |

### LumeScene Layer Thread Safety

When ScenePluginAddon calls into LumeScene, these rules apply:
- **Thread-safe (call from any thread):** `IScene`, `INode`, `ICamera` public API — internally dispatches to ECS thread via `AddTaskOrRunDirectly()`.
- **Thread-safe (internally locked):** `RenderResource`, `Shader`, `Image`, `Mesh`, `RenderResourceManager` — use `shared_mutex` or `mutex`.
- **NOT thread-safe (engine/ECS thread only):** `InternalScene`, `IEcs`, `IComponentManager`, `IEntityManager`, `EcsObject` internals, all Component subclasses.

## Public API Definition

The JS/ArkTS public API is defined in TypeScript declaration files:
- **Primary:** `@pr/LumeTS/ModuleDeclaration/api/graphics3d/scene.d.ts` — Scene, SceneResourceFactory, SceneComponent, RenderContext, etc.
- **Related:** `sceneNodes.d.ts` (Camera, Light, Node, Geometry), `sceneTypes.d.ts` (Vec3, Color, enums), `sceneResources.d.ts` (Shader, Material, Animation, etc.)

APIs that appear in these `.d.ts` files are **public**. APIs exposed in NAPI but NOT in the `.d.ts` files are **private/internal** — they exist for internal use or testing and are not part of the public contract.

## Step-by-Step: Exposing a New Type to JS

### 1. Register the class

**File:** `src/register_module.cpp`

```cpp
void RegisterClasses(napi_env env, napi_value exports) {
    MyTypeJS::Init(env, exports);
    MyTypeJS::RegisterEnums(env, exports);  // if it has enums
}
```

### 2. Create the JS wrapper class

Inherit from `BaseObject` (and `NodeImpl` for node types).

Required methods: `Init()`, constructor, `GetInstanceImpl()`, `DisposeNative()`, `Finalize()`.

### 3. Define member properties

```cpp
void MyTypeJS::Init(napi_env env, napi_value exports) {
    BASE_NS::vector<napi_property_descriptor> props;
    NodeImpl::GetPropertyDescs(props);  // inherit base props

    props.push_back(GetSetProperty<float, MyTypeJS, &MyTypeJS::GetSpeed, &MyTypeJS::SetSpeed>("speed"));
    props.push_back(TROGetSetProperty<float, MyTypeJS, &MyTypeJS::GetSpeed, &MyTypeJS::SetSpeed>("speed"));

    napi_value func;
    napi_define_class(env, "MyType", NAPI_AUTO_LENGTH,
        BaseObject::ctor<MyTypeJS>(), nullptr,
        props.size(), props.data(), &func);
    mis->StoreCtor("MyType", func);
}
```

| Template | Purpose |
|----------|---------|
| `GetProperty<Type, Class, &Getter>("name")` | Read-only |
| `GetSetProperty<Type, Class, &Getter, &Setter>("name")` | Read-write |
| `TROGetProperty<Type, Class, &F>("name")` | Via TrueRootObject dispatch |
| `TROGetSetProperty<Type, Class, &Getter, &Setter>("name")` | TRO read-write |
| `Method<FC, Class, &Method>("name")` | Instance method |

### 4. Proxy properties (Vec3, Quat, Color)

```cpp
posProxy_ = BASE_NS::make_unique<Vec3Proxy>(env, node->Position());
```

Proxy hierarchy: `PropertyProxy` -> `ObjectPropertyProxy` -> `Vec3Proxy`/`QuatProxy`/`ColorProxy`

### 5. Dynamic properties (SceneComponentJS)

Auto-discovers properties via `IComponent::EnumerateProperties()`. Access: `component.property.fieldName`.

### 6. Accessing ECS-backed properties from JS

Use the **full dotted property name** for O(1) lookup:
```cpp
BASE_NS::string fullName = compName + "." + jsKey;
auto prop = meta->GetProperty(fullName);
```

### 7. Enums

```cpp
void MyTypeJS::RegisterEnums(napi_env env, napi_value exports) {
    NapiApi::Object en(env);
    napi_value v;
    napi_create_uint32(env, static_cast<uint32_t>(MyEnum::VALUE_A), &v);
    en.Set("VALUE_A", v);
    exports.Set("MyEnum", en);
}
```

### 8. Lifetime management

- `PtrType::WEAK` — Scene owns native object (nodes, cameras, lights)
- `PtrType::STRONG` — JS owns native object (materials, images, standalone resources)

### 9. Threading patterns

LumeScene **public API** (`IScene`, `INode`, `ICamera`, etc.) is thread-safe — these internally dispatch to the ECS thread via `AddTaskOrRunDirectly()`. However, `InternalScene`, raw ECS functions (`IEcs`, `IComponentManager`), and `EcsObject` internals are **not** thread-safe. The threading concern in ScenePluginAddon is about NAPI, not LumeScene public API.

- **Async (Promise-based):** Return `Promise`, schedule resolution on JS thread via `NodeJSTaskQueue`.
- **Sync from JS callbacks:** Use `ExecSyncTask()` when you need to perform engine work synchronously from a JS callback. This dispatches a task to the engine thread queue and blocks until complete.

Thread IDs:
- Engine: `2070e705-d061-40e4-bfb7-90fad2c280af`
- JS: `b2e8cef3-453a-4651-b564-5190f8b5190d`

Cross-thread scheduling uses `napi_threadsafe_function` via `NodeJSTaskQueue` and `JsObjectCache`. The only NAPI function safe to call from non-JS threads is `napi_call_threadsafe_function` (via `NodeJSTaskQueue::RescheduleTimer`).

### 10. Native-to-JS class name mapping

If native and JS class names differ, add mapping in `src/BaseObjectJS.cpp:GetConstructorName()`.

## Key Files

| File | Role |
|------|------|
| `src/native_module_export.cpp` | NAPI module entry point |
| `src/register_module.cpp` | Central class registration hub |
| `include/BaseObjectJS.h` | Base class for all wrappers, ExecSyncTask |
| `src/BaseObjectJS.cpp` | CreateFromNativeInstance factory, name mapping |
| `include/TrueRootObject.h` | Native object storage, lifetime management |
| `include/napi/class_definition_helpers.h` | Property/method registration templates |
| `include/PropertyProxy.h` | Proxy hierarchy for complex types |
| `include/NodeImpl.h` / `src/NodeImpl.cpp` | Shared node logic |
| `src/SceneComponentJS.cpp` | Dynamic property auto-discovery |
| `src/nodejstaskqueue.cpp` | Thread-safe JS task scheduling |
| `src/JsObjectCache.cpp` | Thread-safe JS object cache |

## Boundaries

### Always

- Use the full dotted property name (`"ComponentName.propertyName"`) when accessing ECS-backed properties via the META system.

### Ask First

- **Which interfaces should be exposed as public API?** Check the public API definition in `@pr/LumeTS/ModuleDeclaration/api/graphics3d/scene.d.ts` (and related `.d.ts` files). New public APIs must be added to these declaration files. If the interface should NOT be public, explicitly confirm it is a private/internal API.
- **Are there private/internal NAPI APIs to add?** Some APIs are exposed in NAPI but do not appear in the `.d.ts` files (e.g., internal test interfaces, plugin-specific helpers). Confirm whether new bindings are public or private before implementing.
- **Which LumeScene interfaces need wrapping?** Not all LumeScene interfaces have JS wrappers. Confirm which interfaces need exposure before creating new wrapper classes.

### Never

- **Do NOT call NAPI functions from a non-JS thread**, with the sole exception of `napi_call_threadsafe_function` (used internally by `NodeJSTaskQueue`). All other `napi_*` functions — including `napi_create_*`, `napi_get_reference_value`, `napi_call_function`, `napi_define_class`, `napi_create_object`, etc. — MUST only be called on the JS thread. Violating this causes undefined behavior and crashes.
- **Do NOT call ECS or InternalScene functions directly from JS callbacks.** Raw ECS functions (`IEcs`, `IComponentManager`, `IEntityManager`), `InternalScene`, and `EcsObject` internals are not thread-safe and must be dispatched to the engine thread. The LumeScene **public API** (`IScene`, `INode`) is thread-safe. Use `ExecSyncTask()` or async task scheduling for operations that need the engine thread.
