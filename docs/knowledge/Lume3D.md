# Lume3D AGENTS.md — ECS System & Component Reference

This document is the **canonical reference** for writing new ECS systems with components. The patterns here apply to Lume3D itself AND any plugin that extends it (e.g., LumeBoidsSwarm, future physics plugins, etc.).

## Module Dependencies

```
Lume3D
  ├── LumeEngine (CORE_ROOT_DIRECTORY) — ECS framework, plugin system, property tools
  ├── LumeRender (RENDER_ROOT_DIRECTORY) — rendering abstractions
  └── LumeBase — containers, math, strings, memory
```

Plugins that extend Lume3D (e.g., LumeBoidsSwarm) declare a dependency on `CORE3D_NS::UID_3D_PLUGIN` and reuse the same ECS patterns documented here.

## Architecture Overview

Components are plain data structs managed by `IComponentManager` subclasses. Systems implement `ISystem` and process entities via `ComponentQuery`.

Key reference implementations in Lume3D:
- **Simple system:** `src/ecs/systems/local_matrix_system.cpp` (basic query + iteration)
- **Complex system with listeners + threading:** `src/ecs/systems/animation_system.cpp`
- **System with node graph:** `src/ecs/systems/node_system.cpp` (node cache, tree traversal)

For a plugin example, see `submodules/LumeBoidsSwarm/src/ecs/systems/boids_swarm_system.cpp`.

## Step 1: Create the Component Header

**Location:** `api/<namespace>/ecs/components/<name>_component.h`

The header uses a **dual-mode include** pattern. When included normally it produces a struct + abstract manager interface. When included with `IMPLEMENT_MANAGER` defined, it produces a static `Property[]` metadata array.

```cpp
#if !defined(API_MY_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_MY_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
// namespace includes...
#endif

BEGIN_COMPONENT(IMyComponentManager, MyComponent)
    DEFINE_PROPERTY(float, speed, "Speed", 0, VALUE(1.0f))
    DEFINE_PROPERTY(BASE_NS::Math::Vec3, direction, "Direction", 0, ARRAY_VALUE(0.f, 1.f, 0.f))
END_COMPONENT(IMyComponentManager, MyComponent, "<generate-a-uuid>")

#if !defined(IMPLEMENT_MANAGER)
// namespace end...
#endif
#endif
```

### Macro Reference

| Macro | Purpose |
|-------|---------|
| `BEGIN_COMPONENT(Interface, StructName)` | Opens struct declaration |
| `DEFINE_PROPERTY(type, name, displayName, flags, default)` | Scalar property |
| `DEFINE_ARRAY_PROPERTY(type, count, name, displayName, flags, ARRAY_VALUE(...))` | Fixed-size array |
| `DEFINE_BITFIELD_PROPERTY(uint64_t, name, displayName, IS_BITFIELD, VALUE(default), EnumName)` | Bitfield |
| `END_COMPONENT(Interface, StructName, uuid)` | Closes struct + declares manager interface |

Property flags: `0` for none, or combine `CORE_NS::PropertyFlags` values (IS_HIDDEN, NO_SERIALIZE, IS_READONLY, HAS_MIN, IS_SLIDER, etc.).

## Step 2: Create the Component Manager Implementation

**Location:** `src/ecs/components/<name>_component_manager.cpp`

```cpp
#include <my_component.h>
#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"

#define IMPLEMENT_MANAGER
#include <core/property_tools/property_macros.h>

BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;
using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;

class MyComponentManager final : public BaseManager<MyComponent, IMyComponentManager> {
    BEGIN_PROPERTY(MyComponent, componentMetaData_)
#include <my_component.h>
    END_PROPERTY();

public:
    explicit MyComponentManager(IEcs& ecs)
        : BaseManager<MyComponent, IMyComponentManager>(ecs, CORE_NS::GetName<MyComponent>())
    {}
    ~MyComponentManager() = default;

    size_t PropertyCount() const override { return BASE_NS::countof(componentMetaData_); }
    const Property* MetaData(size_t index) const override {
        if (index < BASE_NS::countof(componentMetaData_)) return &componentMetaData_[index];
        return nullptr;
    }
    array_view<const Property> MetaData() const override { return componentMetaData_; }
};

IComponentManager* IMyComponentManagerInstance(IEcs& ecs) {
    return new MyComponentManager(ecs);
}
void IMyComponentManagerDestroy(IComponentManager* instance) {
    static_cast<MyComponentManager*>(instance)->~MyComponentManager();
    ::operator delete(instance);
}
END_NAMESPACE()
```

**For enum properties:** Add `DECLARE_PROPERTY_TYPE(EnumName)` and `ENUM_TYPE_METADATA(EnumName, ENUM_VALUE(...))` before the manager class.

## Step 3: Create the System Interface (Public API)

**Location:** `api/<namespace>/ecs/systems/intf_<name>_system.h`

```cpp
class IMySystem : public CORE_NS::ISystem {
public:
    static constexpr BASE_NS::Uid UID { "<generate-a-uuid>" };
    // Domain-specific virtual methods...
protected:
    IMySystem() = default;
    IMySystem(const IMySystem&) = delete;
    IMySystem(IMySystem&&) = delete;
    IMySystem& operator=(const IMySystem&) = delete;
    IMySystem& operator=(IMySystem&&) = delete;
};

inline constexpr BASE_NS::string_view GetName(const IMySystem*) { return "MySystem"; }
```

## Step 4: Create the System Implementation

**Location:** `src/ecs/systems/<name>_system.h` and `src/ecs/systems/<name>_system.cpp`

### Constructor — Fetch component managers

```cpp
MySystem::MySystem(IEcs& ecs)
    : ecs_(ecs),
      myManager_(*(GetManager<IMyComponentManager>(ecs))),
      transformManager_(*(GetManager<ITransformComponentManager>(ecs)))
{}
```

### Initialize — Set up ComponentQuery + listeners

```cpp
void MySystem::Initialize() {
    const ComponentQuery::Operation operations[] = {
        { myManager_, ComponentQuery::Operation::REQUIRE },
    };
    query_.SetEcsListenersEnabled(true);
    query_.SetupQuery(transformManager_, operations, true);
    ecs_.AddListener(myManager_, *this);
}
```

### Update — Execute query, iterate results

```cpp
bool MySystem::Update(bool, uint64_t, uint64_t delta) {
    if (!active_) return false;
    if (generation_ == myManager_.GetGenerationCounter()) return false;
    generation_ = myManager_.GetGenerationCounter();

    query_.Execute();
    for (const auto& row : query_.GetResults()) {
        auto data = myManager_.Read(row.components[1]);
        // Process...
    }
    return true;
}
```

### Uninitialize

```cpp
void MySystem::Uninitialize() {
    ecs_.RemoveListener(myManager_, *this);
    query_.SetEcsListenersEnabled(false);
}
```

### Factory Functions (Required)

```cpp
ISystem* IMySystemInstance(IEcs& ecs) { return new MySystem(ecs); }
void IMySystemDestroy(ISystem* instance) { delete static_cast<MySystem*>(instance); }
```

## Step 5: Register in the Plugin

For Lume3D: edit `src/plugin/static_plugin.cpp`.
For external plugins: edit `src/plugin.cpp` (see LumeBoidsSwarm pattern).

Register component: `MANAGER(MY_COMPONENT_TYPE_INFO, IMyComponentManager);`
Define deps: `constexpr Uid MY_SYSTEM_RW_DEPS[] = { ... };`
Register system: `SYSTEM(MY_SYSTEM_TYPE_INFO, IMySystem, RW_DEPS, R_DEPS, afterSystem, beforeSystem);`
Add to arrays: `CORE_COMPONENT_TYPE_INFOS[]` / `CORE_SYSTEM_TYPE_INFOS[]`

## Accessing Component Data

There are three ways to access component data in a system:

### `Read(entity|componentId)` — Scoped Read Access (Preferred for reading)

Returns `ScopedHandle<const ComponentType>`. RAII lock: calls `RLock()` on construction, `RUnlock()` on destruction. Multiple concurrent readers allowed.

```cpp
if (auto handle = transformManager_.Read(entity)) {
    Math::Vec3 pos = handle->position;
}
```

Use when: You only need to read data. No side effects. This is the default choice for reading.

### `Write(entity|componentId)` — Scoped Write Access (Marks component dirty)

Returns `ScopedHandle<ComponentType>`. RAII lock: calls `WLock()` on construction, `WUnlock()` on destruction. Exclusive access. **Calling `Write()` increments the component's generation counter**, which triggers consecutive systems that monitor that component to re-run.

```cpp
if (auto handle = transformManager_.Write(entity)) {
    handle->position = newPos;
}
```

Use when: You need to modify data AND want downstream systems to see the change.

### `GetData(entity|componentId)` — Raw Untyped Handle (Advanced)

Returns `IPropertyHandle*`. No automatic locking. Used for dynamic property access where the component type is not known at compile time. Must manually wrap with `ScopedHandle<T>(handle)` for safe access.

```cpp
auto* rawHandle = nodeManager_.GetData(componentId);
if (auto handle = ScopedHandle<NodeComponent>(rawHandle)) {
    handle->enabled = true;
}
```

Use when: You need to access a component through a generic `IComponentManager*` pointer, or you need to combine read-then-write in a single lock.

### Choosing Between Read, Write, and GetData

| Method | Lock | Dirty | Use When |
|--------|------|-------|----------|
| `Read()` | Shared (RLock/RUnlock) | No | Only reading data |
| `Write()` | Exclusive (WLock/WUnlock) | **Yes** | Modifying data that downstream systems need to see |
| `GetData()` + `ScopedHandle` | Manual | Manual | Generic access via base `IComponentManager*` |

**Key insight:** `Write()` increments the generation counter. If your system writes to a component that another system monitors via generation counter checks, the downstream system will re-process that entity. Be intentional about which properties you write to — unnecessary `Write()` calls cause wasted work in subsequent systems.

## ComponentQuery Result Indexing

`SetupQuery(baseManager, operations)` produces result rows where:
- `row.components[0]` = ComponentId in the **base** manager
- `row.components[N]` = ComponentId in `operations[N-1]` manager
- `row.entity` = the entity
- `row.IsValidComponentId(N)` = false if operation was `OPTIONAL` and entity lacks that component

## Cache-Friendly Data Access Pattern

For performance-critical systems processing many entities, gather component data into contiguous arrays before processing. This improves memory locality compared to scattered ECS component manager reads.

**Example from AnimationSystem** (`src/ecs/systems/animation_system.h:113-121,193-194`):

```cpp
struct TrackValues {
    InitialTransformComponent initial;
    InitialTransformComponent result;
    float timePosition { 0.f };
    float weight { 0.f };
    TrackState state { TrackState::STOPPED };
    bool forward { true };
    bool updated { false };
};

BASE_NS::vector<TrackValues> trackValues_;   // indexed by ComponentId
BASE_NS::vector<FrameData> frameIndices_;    // indexed by ComponentId
```

The pattern:
1. **Resize** arrays to `manager.GetComponentCount()` — ensures contiguous memory
2. **Gather** data from ECS into `trackValues_[]` via `InitializeTrackValues()` — linear scan
3. **Process** in batches via thread pool tasks operating on sub-ranges of `trackOrder_` indices
4. **Write back** results to ECS components after processing

This avoids pointer-chasing through scattered ECS component handles during computation. The same pattern is used in NodeSystem's `UpdatePreviousWorldMatrices()` for linear scanning of world matrix components.

## System Execution Order (Lume3D)

```
AnimationSystem       -> before LocalMatrixSystem
LocalMatrixSystem     -> after Animation, before NodeSystem
NodeSystem            -> after LocalMatrix, before RenderPreprocessor
SkinningSystem        -> after Node, before RenderPreprocessor
MorphingSystem        -> after Node, before RenderSystem
WeatherSystem         -> after Node, before RenderSystem
RenderPreprocessor    -> after Node
RenderSystem          -> after RenderPreprocessor
```

## File Checklist for New Component + System

| Create | Path |
|--------|------|
| Component header | `api/<namespace>/ecs/components/<name>_component.h` |
| Manager implementation | `src/ecs/components/<name>_component_manager.cpp` |
| System interface | `api/<namespace>/ecs/systems/intf_<name>_system.h` |
| System header | `src/ecs/systems/<name>_system.h` |
| System implementation | `src/ecs/systems/<name>_system.cpp` |

| Modify | Path |
|--------|------|
| Plugin registration | `src/plugin/static_plugin.cpp` (Lume3D) or `src/plugin.cpp` (external plugins) |

## Unit Tests

Tests use **GoogleTest** with the `UNIT_TEST(SuiteName, TestName)` macro (maps to `TEST()`).

**Test structure:**
- API tests (public headers only): `test/unittest/api_unit_test/src/ecs/systems/`
- SRC tests (internal headers allowed): `test/unittest/src_unit_test/src/ecs/systems/`

**Test pattern:**
```cpp
#include <3d/ecs/components/my_component.h>
#include <3d/ecs/systems/intf_my_system.h>
#include "test_framework.h"
#include "test_runner.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace CORE3D_NS;

UNIT_TEST(API_EcsMySystem, MyBasicTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto mySystem = GetSystem<IMySystem>(*ecs);
    ASSERT_NE(nullptr, mySystem);

    // Create entity, add components, call Update(), verify results
    Entity entity = ecs->GetEntityManager().Create();
    auto& myManager = *(GetManager<IMyComponentManager>(*ecs));
    myManager.Create(entity);

    mySystem->Update(true, 0, 16666);

    auto data = myManager.Read(entity);
    EXPECT_EQ(data->speed, expectedSpeed);
}
```

`UTest::GetTestContext()` returns `{engine, renderContext, graphicsContext, ecs}` with a fresh ECS per test.

**Reference examples:**
- `test/unittest/api_unit_test/src/ecs/systems/node_system_test.cpp`
- `test/unittest/api_unit_test/src/ecs/systems/animation_system_test.cpp`
- `test/unittest/src_unit_test/src/ecs/systems/local_matrix_system_test.cpp`

**For a new test file**, add the `.cpp` to the appropriate CMakeLists.txt:
- API tests: `test/unittest/api_unit_test/CMakeLists.txt`
- SRC tests: `test/unittest/src_unit_test/CMakeLists.txt`

**Building and running on Windows:**
```powershell
# 1. Enable tests in CMake configuration
cmake -B build_vs2026 -DCORE3D_UNIT_TESTS_ENABLED=ON

# 2. Build the test runners
cmake --build build_vs2026 --target Core3DAPITestRunner --config Debug
cmake --build build_vs2026 --target Core3DSRCTestRunner --config Debug

# 3. Run via CTest
ctest --test-dir build_vs2026 -C Debug -R core3dAPI
ctest --test-dir build_vs2026 -C Debug -R core3dSRC

# 4. Or run directly with GTest filtering
.\Core3DAPITestRunner.exe --gtest_filter=API_EcsMySystem.*
```

Two test executables: `Core3DAPITestRunner` (loads plugins dynamically, public API only) and `Core3DSRCTestRunner` (links statically, can access internal headers). Requires Vulkan runtime.

## Boundaries

### Always

- Run `clang-format` on all C/C++ changes. This repo uses a `.pre-commit-config.yaml` with `clang-format v15.0.6`. A CMake `FormatAll3D` target is also available.
- Generate new UUIDs for every new component and system UID.
- Use `Read()` for read-only access. Use `Write()` only when modifying data.
- Declare system dependencies (RW_DEPS and R_DEPS) correctly to enable parallel execution.
- **Write unit tests for every new system and component.** Tests are critical for ensuring correctness. See the `## Unit Tests` section for details.

### Ask First

- **Which properties need to be dirty (`Write()`)?** Before writing to component properties, confirm which ones need to trigger downstream system updates. Unnecessary `Write()` calls cause cascade re-processing. Consider using `GetData()` + `ScopedHandle` if you need to modify data without triggering generation counter bumps, but be aware this bypasses the dirty tracking system.
- **Where does the new system run in the execution order?** Determine the `afterSystem` and `beforeSystem` constraints based on which components it reads vs. writes.

### Never

- Do NOT call `Write()` on a component when you only need to read it.
- Do NOT skip registering system dependencies — this can cause race conditions in parallel execution.
