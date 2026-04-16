# 3D 图形 ArkTS 静态接口开发指南

## 目录

- [概述](#概述)
- [架构设计](#架构设计)
- [目录结构](#目录结构)
- [核心技术栈](#核心技术栈)
- [开发流程](#开发流程)
- [API 设计规范](#api-设计规范)
- [Taihe IDL 语法](#taihe-idl-语法)
- [接口实现层 C++ (ETS)](#接口实现层-c-ets)
- [属性代理系统](#属性代理系统)
- [编译构建](#编译构建)
- [推荐实践](#推荐实践)
- [常见问题](#常见问题)

---

## 概述

### 什么是 ArkTS 静态接口

ArkTS 静态接口是 AGP (Ark Graphics Platform) 3D 引擎提供给 ArkTS 应用的静态类型化接口层。通过 Taihe 编译器，将 C++ 实现编译为静态类型化的 ArkTS 绑定，提供类型安全和高性能的 3D 图形能力。

### 主要特性

- ✅ **静态类型安全**：编译时类型检查，减少运行时错误
- ✅ **高性能**：直接映射到 C++ 实现，无额外运行时开销
- ✅ **完整功能覆盖**：支持场景管理、节点操作、材质系统、后处理特效等
- ✅ **Promise/Async 支持**：异步操作更符合 ArkTS 编程习惯
- ✅ **属性代理**：自动化的属性绑定和更新机制

### 应用场景

```
┌─────────────────────────────────────────┐
│         ArkTS 应用层                     │
│  (使用 Scene, Node, Material 等 API)    │
└────────────────┬────────────────────────┘
                 │
┌────────────────▼────────────────────────┐
│     ArkTS 静态接口层 (本模块)            │
│  - Scene.abc                           │
│  - SceneNodes.abc                      │
│  - SceneResources.abc                  │
│  - SceneTypes.abc                      │
└────────────────┬────────────────────────┘
                 │
┌────────────────▼────────────────────────┐
│       语言适配层 (Taihe 框架生成)        │
│  - taihe/include/*Impl.h               │
│  - taihe/src/*Impl.cpp                 │
└────────────────┬────────────────────────┘
                 │
┌────────────────▼────────────────────────┐
│       接口实现层 C++ (scene_ani.z.so)   │
│  - SceneETS, NodeETS, MaterialETS...   │
└────────────────┬────────────────────────┘
                 │
┌────────────────▼────────────────────────┐
│       AGP 3D 引擎封装层                 │
│  - Lume Engine + Scene System          │
└─────────────────────────────────────────┘
```

---

## 架构设计

### 四层架构

```
┌─────────────────────────────────────────────────────────┐
│                 ArkTS 接口层                            │
│  (由 Taihe IDL 定义，生成 .abc 文件)                    │
│  - Scene.abc, SceneNodes.abc, SceneResources.abc       │
└────────────────────────────┬────────────────────────────┘
                             │
┌────────────────────────────▼────────────────────────────┐
│               语言适配层 (Taihe 框架生成)                │
│  - taihe/include/*Impl.h: 自动生成的头文件              │
│  - taihe/src/*Impl.cpp: Taihe 绑定实现                  │
│  职责: 将 ArkTS 调用转换为 C++ 调用                     │
└────────────────────────────┬────────────────────────────┘
                             │
┌────────────────────────────▼────────────────────────────┐
│              接口实现层 C++ (ETS)                       │
│  - include/*ETS.h: C++ 头文件定义                       │
│  - src/*ETS.cpp: C++ 业务逻辑实现                       │
│  职责: 实现具体的 3D 图形 API 业务逻辑                  │
└────────────────────────────┬────────────────────────────┘
                             │
┌────────────────────────────▼────────────────────────────┐
│               AGP 引擎封装层                             │
│  - Lume Engine, Scene System, Render Context           │
│  职责: 提供底层 3D 渲染引擎能力                         │
└─────────────────────────────────────────────────────────┘
```

### 模块划分

| 模块 | 说明 | 主要内容 |
|------|------|----------|
| **SceneTH** | 场景核心接口 | Scene, SceneResourceFactory, RenderContext |
| **SceneNodes** | 节点系统 | Node, Camera, Light, Geometry |
| **SceneResources** | 资源管理 | Material, Shader, Mesh, Image, Environment |
| **SceneTypes** | 基础类型 | Vec2/3/4, Quaternion, Color, GeometryDefinition |
| **ScenePostProcessSettings** | 后处理 | Bloom, ToneMap, Vignette, ColorFringe |

---

## 目录结构

```
kits/ets/
├── include/                           # 接口实现层 C++ (ETS) - 头文件
│   ├── SceneETS.h                    # 场景接口
│   ├── NodeETS.h                     # 节点接口
│   ├── MaterialETS.h                 # 材质接口
│   ├── CameraETS.h                   # 相机接口
│   ├── LightETS.h                    # 光源接口
│   ├── geometry_definition/          # 几何体定义
│   │   ├── CubeETS.h
│   │   ├── SphereETS.h
│   │   ├── PlaneETS.h
│   │   ├── CylinderETS.h
│   │   └── CustomETS.h
│   └── property_proxy/               # 属性代理
│       ├── Vec3Proxy.h
│       ├── QuatProxy.h
│       └── ColorProxy.h
│
├── src/                              # 接口实现层 C++ (ETS) - 实现
│   ├── SceneETS.cpp
│   ├── NodeETS.cpp
│   ├── MaterialETS.cpp
│   └── ...
│
├── taihe/                            # 语言适配层
│   ├── idl/                          # IDL 接口定义 (ArkTS 接口层)
│   │   ├── SceneTH.taihe            # 主场景接口
│   │   ├── SceneNodes.taihe         # 节点接口
│   │   ├── SceneResources.taihe     # 资源接口
│   │   ├── SceneTypes.taihe         # 类型定义
│   │   └── ScenePostProcessSettings.taihe
│   ├── include/                      # Taihe 自动生成的头文件 (语言适配层)
│   │   ├── SceneImpl.h
│   │   ├── NodeImpl.h
│   │   └── ...
│   ├── src/                          # Taihe 绑定实现 (语言适配层)
│   │   ├── SceneImpl.cpp
│   │   ├── NodeImpl.cpp
│   │   ├── MaterialImpl.cpp
│   │   └── ...
│   ├── ets/                          # 手写的 ArkTS 代码
│   │   └── RecordProxy.ets          # Record 类型代理
│   └── include/                      # Taihe 相关头文件
│
├── BUILD.gn                          # 构建配置
└── ARKTS_STATIC_API_GUIDE.md         # 本文档
```

---

## 核心技术栈

### 1. Taihe 编译器

Taihe 是 OpenHarmony 的跨语言接口定义语言（IDL）编译器，支持：

- **接口定义**：使用 `.taihe` 文件定义接口
- **代码生成**：自动生成 ArkTS 绑定（`.abc`）和 C++ 桥接代码
- **类型映射**：自动处理 ArkTS 和 C++ 之间的类型转换
- **异步支持**：通过 `@gen_promise` 自动生成 Promise 版本的异步接口

### 2. 属性代理系统 (PropertyProxy)

提供类型安全的属性访问和自动同步机制：

```cpp
// Vec3 属性代理
auto posProxy = node->GetPosition();
posProxy->SetX(1.0f);
posProxy->SetY(2.0f);
posProxy->SetZ(3.0f);
// 自动同步到引擎内部
```

---

## 开发流程

### 新增 API 的完整流程

按照四层架构，新增 API 需要从上到下依次实现各层代码。以新增 `SphereGeometry` API 为例：

#### 步骤 1: 定义 Taihe IDL 接口（ArkTS 接口层）

首先在 IDL 中定义接口规范，这是 ArkTS 和 C++ 之间的契约，Taihe 编译器将根据此生成 ArkTS 绑定代码和语言适配层框架。

**`taihe/idl/SceneTypes.taihe`**
```taihe
@class
interface SphereGeometry : GeometryDefinition {
    @get getRadius(): f64;
    @set setRadius(radius: f64): void;

    @get getSegmentCount(): i32;
    @set setSegmentCount(count: i32): void;
}

@ctor("SphereGeometry")
function CreateSphereGeometry(): SphereGeometry;
```

#### 步骤 2: 实现语言适配层（Taihe 绑定）

实现 Taihe 绑定类 `*Impl.cpp`，这是语言适配层的一部分（框架代码由 Taihe 工具自动生成在 taihe/include/*Impl.h）。这一层的职责是将 ArkTS 调用转换为 C++ 接口实现层的方法调用。

**`taihe/src/SphereGeometryImpl.cpp`**
```cpp
#include "SphereETS.h"
#include <taihe/ani.h>

using namespace OHOS::Render3D;

// 构造函数绑定：将 ArkTS 的 new SphereGeometry() 映射到 C++ 对象创建
BASE_NS::tuple<bool, BASE_NS::string_view, SphereGeometry*> CreateSphereGeometry()
{
    auto sphere = std::make_shared<SphereETS>(1.0f, 32);
    return {true, "", sphere.release()};
}

// Getter/Setter 绑定：将 ArkTS 的属性访问映射到 C++ 方法调用
double SphereGeometry_GetRadius(SphereGeometry* sphere)
{
    return sphere->GetRadius();
}

void SphereGeometry_SetRadius(SphereGeometry* sphere, double radius)
{
    sphere->SetRadius(static_cast<float>(radius));
}

int32_t SphereGeometry_GetSegmentCount(SphereGeometry* sphere)
{
    return sphere->GetSegmentCount();
}

void SphereGeometry_SetSegmentCount(SphereGeometry* sphere, int32_t count)
{
    sphere->SetSegmentCount(count);
}
```

#### 步骤 3: 实现接口实现层 C++（ETS）

实现 C++ ETS 类，这是接口实现层。这一层的职责是实现具体的 3D 图形 API 业务逻辑，并调用 AGP 引擎封装层的底层 API。

**3.1 定义 C++ 头文件**

**`include/SphereETS.h`**
```cpp
#ifndef OHOS_3D_SPHERE_ETS_H
#define OHOS_3D_SPHERE_ETS_H

#include <memory>
#include "GeometryETS.h"

namespace OHOS::Render3D {

class SphereETS : public GeometryETS {
public:
    SphereETS(float radius = 1.0f, int segments = 32);
    ~SphereETS() override;

    float GetRadius() const;
    void SetRadius(float radius);

    int GetSegmentCount() const;
    void SetSegmentCount(int count);

private:
    float radius_;
    int segments_;

    // 引擎底层对象引用
    CORE3D_NS::IGeometry::Ptr nativeGeometry_;
};

} // namespace OHOS::Render3D
#endif
```

**3.2 实现 C++ 类**

**`src/SphereETS.cpp`**
```cpp
#include "SphereETS.h"
#include <core3d/intf_geometry.h>

namespace OHOS::Render3D {

SphereETS::SphereETS(float radius, int segments)
    : GeometryETS(GeometryType::SPHERE), radius_(radius), segments_(segments)
{
    // 调用引擎底层 API 创建几何体
    auto& engine = GetEngine();
    nativeGeometry_ = engine->GetInterface<CORE_NS::IClassFactory>()
        ->CreateInstance(CORE3D_NS::UID_SPHERE_GEOMETRY);

    // 设置引擎底层参数
    nativeGeometry_->SetRadius(radius);
    nativeGeometry_->SetSegmentCount(segments);
}

SphereETS::~SphereETS()
{
    nativeGeometry_.reset();
}

float SphereETS::GetRadius() const
{
    return radius_;
}

void SphereETS::SetRadius(float radius)
{
    radius_ = radius;
    // 同步到引擎底层
    if (nativeGeometry_) {
        nativeGeometry_->SetRadius(radius);
    }
}

int SphereETS::GetSegmentCount() const
{
    return segments_;
}

void SphereETS::SetSegmentCount(int count)
{
    segments_ = count;
    // 同步到引擎底层
    if (nativeGeometry_) {
        nativeGeometry_->SetSegmentCount(count);
    }
}

} // namespace OHOS::Render3D
```

#### 步骤 4: 更新 BUILD.gn

将新添加的源文件添加到构建配置中。

```gni
sources += [
    "src/SphereETS.cpp",
    "taihe/src/SphereGeometryImpl.cpp",
]
```

#### 步骤 5: 编译和测试

```bash
# 编译
hb build graphic_3d

# 在 ArkTS 中使用
import { SphereGeometry } from 'graphics3d_scene_types';

let sphere = new SphereGeometry();
sphere.setRadius(2.0);
sphere.setSegmentCount(64);
```

---

## API 设计规范

### 命名规范

| 类型 | 命名规则 | 示例 |
|------|----------|------|
| **C++ 类** | `XXXETS.h/cpp` | `SceneETS`, `NodeETS` |
| **Taihe 接口** | 驼峰命名 | `IScene`, `INode` |
| **方法** | 动词开头 | `GetPosition`, `SetRotation` |
| **属性** | get/set 前缀 | `getX()`, `setY(value)` |
| **枚举** | 全大写下划线 | `LightType::POINT_LIGHT` |

### 方法设计原则

#### 1. Getter/Setter 模式

```cpp
// ✅ 推荐：使用显式的 Get/Set
Vec3 GetPosition() const;
void SetPosition(const Vec3& pos);

// ❌ 避免：不清晰的方法名
Vec3 Position() const;
void Position(const Vec3& pos);
```

#### 2. 返回值使用 InvokeReturn

```cpp
// ✅ 推荐：返回错误信息
InvokeReturn<std::shared_ptr<NodeETS>> CreateNode(const std::string& path);

// 使用
auto result = scene->CreateNode("/root/node");
if (!result) {
    CORE_LOG_E("Failed: %s", result.error.c_str());
}
auto node = result.value;
```

#### 3. 使用智能指针

```cpp
// ✅ 推荐：使用 shared_ptr
std::shared_ptr<NodeETS> GetChild(uint32_t index);

// ❌ 避免：裸指针
NodeETS* GetChild(uint32_t index);
```

### 异步 API 设计

#### Taihe Promise 自动生成

```taihe
// 定义同步版本
@gen_promise("createNode")
createNodeSync(params: SceneNodeParameters): Node;

// Taihe 自动生成异步版本
createNode(params: SceneNodeParameters): Promise<Node>;
```

#### ArkTS 使用

```typescript
// 同步使用
const node = scene.getResourceFactory().createNodeSync({ name: "MyNode", path: "/root" });

// 异步使用（自动生成）
const node = await scene.getResourceFactory().createNode({ name: "MyNode", path: "/root" });
```

---

## Taihe IDL 语法

### 基础类型

| Taihe 类型 | C++ 类型 | ArkTS 类型 |
|------------|----------|------------|
| `bool` | `bool` | `boolean` |
| `i32` | `int32_t` | `number` |
| `i64` | `int64_t` | `number` |
| `f64` | `double` | `number` |
| `String` | `BASE_NS::string` | `string` |
| `Array<T>` | `BASE_NS::vector<T>` | `T[]` |

### 接口定义

```taihe
// 基础接口
interface INode {
    @get getName(): String;           // getter
    @set setName(name: String): void; // setter

    getChild(index: i32): INode;      // 方法
}

// 类接口（可实例化）
@class
interface Scene : INode {
    renderFrame(params: Optional<RenderParameters>): bool;
}
```

### 结构体定义

```taihe
struct Vec3 {
    x: f64;
    y: f64;
    z: f64;
}

struct RenderParameters {
    alwaysRender: Optional<bool>;
}
```

### 联合类型

```taihe
// 可空类型
union NodeOrNull {
    node: Node;
    @null nValue;
}

// 多种类型
union VariousMaterial {
    shader: ShaderMaterial;
    pbr: PBRMaterial;
    unlit: UnlitMaterial;
}
```

### 可选类型

```taihe
// 可选参数
function createNode(params: SceneNodeParameters, parent: Optional<Node>): Node;

// 可选返回值
interface Scene {
    getEnvironment(): Optional<Environment>;
}
```

### 构造函数绑定

```taihe
// 定义构造函数
@class
interface CustomGeometry : GeometryDefinition {
    @get getVertices(): Array<Vec3>;
    @set setVertices(vertices: Array<Vec3>): void;
}

// 绑定构造函数
@ctor("CustomGeometry")
function CreateCustomGeometry(): CustomGeometry;
```

### 命名空间

```taihe
@!namespace("Scene")

interface Scene {
    // ...
}

@!namespace("SceneNodes")

interface Node {
    // ...
}
```

### 代码注入

```taihe
@!sts_inject("""
// 注入到生成的 ArkTS 代码中
import { ResourceStr } from 'arkui.component.units';
loadLibrary('scene_ani.z');
""")
```

---

## 接口实现层 C++ (ETS)

### ETS 基类设计

#### SceneResourceETS - 资源基类

```cpp
class SceneResourceETS {
public:
    virtual ~SceneResourceETS() = default;
    virtual void Destroy() = 0;
    virtual META_NS::IObject::Ptr GetNativeObj() const = 0;
    virtual std::string GetName() const = 0;

protected:
    BASE_NS::string name_;
};
```

#### NodeETS - 节点基类

```cpp
class NodeETS : public SceneResourceETS,
                public std::enable_shared_from_this<NodeETS> {
public:
    enum NodeType {
        NODE = 1,
        GEOMETRY = 2,
        CAMERA = 3,
        LIGHT = 4,
        TEXT = 5,
        CUSTOM = 255
    };

    // 位置、旋转、缩放
    std::shared_ptr<Vec3Proxy> GetPosition();
    void SetPosition(const BASE_NS::Math::Vec3& position);

    std::shared_ptr<QuatProxy> GetRotation();
    void SetRotation(const BASE_NS::Math::Quat& rotation);

    std::shared_ptr<Vec3Proxy> GetScale();
    void SetScale(const BASE_NS::Math::Vec3& scale);

    // 层级管理
    std::shared_ptr<NodeETS> GetParent();
    void AppendChild(const std::shared_ptr<NodeETS>& childNode);
    void RemoveChild(const std::shared_ptr<NodeETS>& childNode);

    // 可见性
    bool GetVisible();
    void SetVisible(const bool visible);

protected:
    NodeType type_;
    SCENE_NS::INode::WeakPtr node_;
    std::shared_ptr<Vec3Proxy> posProxy_;
    std::shared_ptr<QuatProxy> rotProxy_;
    std::shared_ptr<Vec3Proxy> sclProxy_;
};
```

### 线程安全

#### 任务队列执行

```cpp
#include "Utils.h"

// 在引擎线程同步执行
META_NS::IAny::Ptr ExecSyncTask([]() {
    // 这里的代码在引擎线程执行
    scene->AddNode(node);
    return nullptr;
});

// 在渲染线程执行
META_NS::GetTaskQueueRegistry()
    .GetTaskQueue(RENDER_THREAD)
    ->AddTask(META_NS::MakeCallback<>([]() {
        // 渲染相关操作
    }));
```

### 生命周期管理

#### 智能指针使用

```cpp
// 创建对象
auto node = std::make_shared<NodeETS>(nativeNode);

// 返回智能指针
return std::shared_ptr<NodeETS>(node);

// 持有弱引用避免循环
BASE_NS::weak_ptr<NodeETS> parent_;

// 使用时检查
if (auto parent = parent_.lock()) {
    parent->RemoveChild(shared_from_this());
}
```

---

## 属性代理系统

### 为什么需要属性代理

1. **类型安全**：编译时检查类型
2. **自动同步**：属性变化自动同步到引擎
3. **解耦**：分离接口和实现
4. **易于扩展**：支持自定义属性类型

### 基础代理类

```cpp
template <typename Type>
class PropertyProxy : public IPropertyProxy {
public:
    explicit PropertyProxy(const META_NS::IProperty::Ptr& prop)
        : prop_(prop) {}

    void SetValue(const Type& value) {
        META_NS::Property<Type>(prop_)->SetValue(value);
    }

    Type GetValue() const {
        return META_NS::Property<Type>(prop_)->GetValue();
    }

private:
    META_NS::IProperty::Ptr prop_;
};
```

### Vec3 属性代理

```cpp
class Vec3Proxy : public PropertyProxy<BASE_NS::Math::Vec3> {
public:
    // 便捷的分量访问
    float GetX() const { return GetValue().x; }
    void SetX(float x) {
        auto v = GetValue();
        v.x = x;
        SetValue(v);
    }

    float GetY() const { return GetValue().y; }
    void SetY(float y) {
        auto v = GetValue();
        v.y = y;
        SetValue(v);
    }

    float GetZ() const { return GetValue().z; }
    void SetZ(float z) {
        auto v = GetValue();
        v.z = z;
        SetValue(v);
    }
};
```

### 使用示例

```cpp
// 获取属性代理
auto posProxy = node->GetPosition();

// 设置整个向量
posProxy->SetValue(BASE_NS::Math::Vec3 {1.0f, 2.0f, 3.0f});

// 设置单个分量
posProxy->SetX(5.0f);

// 获取值
float x = posProxy->GetX();
```

---

## 编译构建

### BUILD.gn 配置

#### 1. 配置头文件路径

```gni
config("lume3d_config") {
  include_dirs = [
    "include",
    "include/property_proxy",
    "include/geometry_definition",
    "${LUME_BASE_PATH}/api",
    "${LUME_CORE_PATH}/api",
    "${LUME_RENDER_PATH}/api",
  ]

  cflags = [
    "-Wall",
    "-fexceptions",
    "-DCORE_PUBLIC=__attribute__((visibility(\"default\")))",
  ]
}
```

#### 2. 定义 Taihe 库

```gni
taihe_shared_library("scene_ani") {
  subsystem_name = "graphic"
  part_name = "graphic_3d"

  sources = [
    # C++ 实现
    "src/SceneETS.cpp",
    "src/NodeETS.cpp",

    # Taihe 绑定实现
    "taihe/src/SceneImpl.cpp",
    "taihe/src/NodeImpl.cpp",
  ]

  deps = [
    ":run_taihe",  # Taihe 生成代码
    "../../3d_widget_adapter:lib3dWidgetAdapter",
  ]

  external_deps = [
    "c_utils:utils",
    "hilog:libhilog",
    "napi:ace_napi",
  ]

  shlib_type = "ani"  # 生成 .z.so
}
```

#### 3. 生成静态 ABC

```gni
generate_static_abc("graphics3d_scene") {
  base_url = "$taihe_generated_file_path"
  files = [
    "$taihe_generated_file_path/graphics3d/Scene.ets",
  ]
  is_boot_abc = "True"
  device_dst_file = "/system/framework/graphics3d_scene.abc"
}
```

#### 4. 安装到系统

```gni
ohos_prebuilt_etc("graphics3d_scene_etc") {
  source = "$target_out_dir/graphics3d_scene.abc"
  module_install_dir = "framework"
  subsystem_name = "graphic"
  part_name = "graphic_3d"
  deps = [ ":graphics3d_scene" ]
}
```

### 编译命令

```bash
# 完整编译
hb build graphic_3d

# 仅编译 ETS 接口
hb build graphic_3d_kits_ets_graphics_3d_taihe

# 清理后重新编译
hb clean
hb build graphic_3d --gn-target-filter="graphic_3d_kits_ets*"
```

### 输出产物

编译成功后会生成：

| 文件 | 位置 | 说明 |
|------|------|------|
| `libscene_ani.z.so` | `/system/lib/` | C++ 实现库 |
| `graphics3d_scene.abc` | `/system/framework/` | 场景接口 |
| `graphics3d_scene_nodes.abc` | `/system/framework/` | 节点接口 |
| `graphics3d_scene_resources.abc` | `/system/framework/` | 资源接口 |
| `graphics3d_scene_types.abc` | `/system/framework/` | 类型定义 |

---

## 推荐实践

### 1. 资源管理

#### 使用 RAII 原则

```cpp
// ✅ 推荐：使用智能指针自动管理
class SceneETS {
    ~SceneETS() {
        Destroy();
    }

    void Destroy() {
        environmentETS_.reset();
        renderConfigurationETS_.reset();
        scene_.reset();
    }
};

// ❌ 避免：手动管理内存
class SceneETS {
    ~SceneETS() {
        delete environmentETS_;  // 容易忘记或重复释放
    }
};
```

#### 资源获取即初始化 (RAII)

```cpp
// ✅ 推荐：构造时完成所有初始化
SceneETS::SceneETS() {
    renderContext_ = CreateRenderContext();
    engine_ = CreateEngine();
}

// ❌ 避免：分离的 Init 函数
SceneETS::SceneETS() {
    // 什么都不做
}

void SceneETS::Init() {  // 容易忘记调用
    renderContext_ = CreateRenderContext();
}
```

### 2. 错误处理

#### 使用 InvokeReturn

```cpp
InvokeReturn<std::shared_ptr<NodeETS>> SceneETS::CreateNode(const std::string& path)
{
    if (disposed_) {
        return InvokeReturn<std::shared_ptr<NodeETS>>(
            nullptr, "Scene has been disposed");
    }

    auto node = scene_->CreateNode(path);
    if (!node) {
        return InvokeReturn<std::shared_ptr<NodeETS>>(
            nullptr, "Failed to create node: " + path);
    }

    return InvokeReturn<std::shared_ptr<NodeETS>>(
        std::make_shared<NodeETS>(node));
}

// 使用
auto result = scene->CreateNode("/root/node");
if (!result) {
    CORE_LOG_E("Error: %s", result.error.c_str());
    return;
}
auto node = result.value;
```

### 3. 性能优化

#### 避免频繁的属性访问

```cpp
// ❌ 避免：多次调用
for (int i = 0; i < 1000; i++) {
    float x = node->GetPosition()->GetX();  // 每次都要查询
    node->GetPosition()->SetX(x + 1.0f);
}

// ✅ 推荐：缓存代理
auto pos = node->GetPosition();
for (int i = 0; i < 1000; i++) {
    float x = pos->GetX();
    pos->SetX(x + 1.0f);
}
```

#### 批量操作

```cpp
// ✅ 推荐：批量设置
void SetTransform(NodeETS* node, const Vec3& pos, const Quat& rot, const Vec3& scale)
{
    node->SetPosition(pos);
    node->SetRotation(rot);
    node->SetScale(scale);
    // 最后统一触发更新
    node->MarkDirty();
}
```

### 4. 线程安全

#### 正确使用任务队列

```cpp
// ✅ 推荐：在引擎线程执行引擎操作
void SceneETS::AddNode(std::shared_ptr<NodeETS> node)
{
    ExecSyncTask([this, node]() {
        scene_->AddNode(node->GetInternalNode());
    });
}

// ❌ 避免：跨线程直接调用
void SceneETS::AddNode(std::shared_ptr<NodeETS> node)
{
    scene_->AddNode(node->GetInternalNode());  // 可能不是在引擎线程
}
```

### 5. API 兼容性

#### 向后兼容

```cpp
// 添加新方法时保留旧方法
class NodeETS {
public:
    // 新方法
    std::shared_ptr<Vec3Proxy> GetPosition();

    // 旧方法（标记为已弃用）
    [[deprecated("Use GetPosition() instead")]]
    BASE_NS::Math::Vec3 GetPositionValue() const;
};
```

---

## 常见问题

### Q1: Taihe 编译错误

**问题**: `undefined reference to xxx`

**原因**:
1. IDL 中定义了接口但没有实现
2. 函数签名不匹配

**解决方案**:
```cpp
// 确保实现完全匹配 Taihe IDL 定义
// IDL:
// @get getName(): String;

// C++ 实现:
BASE_NS::string_view NodeImpl_GetName(NodeETS* node)
{
    return node->GetName();
}
```

### Q2: 属性访问崩溃

**问题**: 访问属性时程序崩溃

**原因**: 代理对象的生命周期管理不当

**解决方案**:
```cpp
// ✅ 推荐：持有代理的 shared_ptr
class NodeETS {
    std::shared_ptr<Vec3Proxy> posProxy_;

    std::shared_ptr<Vec3Proxy> GetPosition() {
        if (!posProxy_) {
            posProxy_ = std::make_shared<Vec3Proxy>(
                node_->GetPositionProperty());
        }
        return posProxy_;
    }
};
```

### Q3: 内存泄漏

**问题**: 内存持续增长

**原因**: 循环引用导致无法释放

**解决方案**:
```cpp
// 使用 weak_ptr 打破循环
class NodeETS {
    // ✅ 使用弱引用引用父节点
    BASE_NS::weak_ptr<NodeETS> parent_;

    // ✅ 子节点可以使用 shared_ptr
    BASE_NS::vector<std::shared_ptr<NodeETS>> children_;
};
```

### Q4: 类型转换错误

**问题**: Taihe 类型和 C++ 类型不匹配

**解决方案**:
```cpp
// IDL: f64
// C++: double (不是 float)

// ✅ 正确
double SphereGeometry_GetRadius(SphereETS* sphere)
{
    return sphere->GetRadius();  // 自动转换
}

// ❌ 错误
float SphereGeometry_GetRadius(SphereETS* sphere)  // 类型不匹配
{
    return sphere->GetRadius();
}
```

### Q5: Promise 不工作

**问题**: 调用异步方法时没有返回 Promise

**检查清单**:
1. IDL 中是否添加了 `@gen_promise`
2. 同步方法名是否以 `Sync` 结尾
3. 是否正确使用了 `Optional` 类型

```taihe
// ✅ 正确
@gen_promise("createNode")
createNodeSync(params: SceneNodeParameters): Node;

// ❌ 错误：缺少 Sync 后缀
@gen_promise("createNode")
createNode(params: SceneNodeParameters): Node;
```

### Q6: 编译产物找不到

**问题**: 编译成功但运行时找不到 .abc 文件

**解决方案**:
1. 检查 `device_dst_file` 路径是否正确
2. 确认 `is_boot_abc = "True"`
3. 验证 `ohos_prebuilt_etc` 配置正确

```bash
# 在设备上检查
hdc shell ls -l /system/framework/graphics3d_*.abc
```

---

## 附录：完整示例

### 示例 1: 创建自定义节点

**C++ 头文件** (`MyCustomNodeETS.h`):
```cpp
#ifndef MY_CUSTOM_NODE_ETS_H
#define MY_CUSTOM_NODE_ETS_H

#include "NodeETS.h"

namespace OHOS::Render3D {

class MyCustomNodeETS : public NodeETS {
public:
    MyCustomNodeETS(const SCENE_NS::INode::Ptr& node);
    ~MyCustomNodeETS() override = default;

    // 自定义属性
    float GetCustomValue() const { return customValue_; }
    void SetCustomValue(float value) { customValue_ = value; }

private:
    float customValue_ = 0.0f;
};

} // namespace OHOS::Render3D
#endif
```

**Taihe IDL** (`SceneTH.taihe`):
```taihe
@class
interface MyCustomNode : Node {
    @get getCustomValue(): f64;
    @set setCustomValue(value: f64): void;
}

@ctor("MyCustomNode")
function CreateMyCustomNode(): MyCustomNode;
```

**Taihe 实现** (`MyCustomNodeImpl.cpp`):
```cpp
#include "MyCustomNodeETS.h"

BASE_NS::tuple<bool, BASE_NS::string_view, MyCustomNode*> CreateMyCustomNode()
{
    // 创建原生节点
    auto nativeNode = GetEngine()->CreateNode();
    auto customNode = std::make_shared<MyCustomNodeETS>(nativeNode);

    return {true, "", customNode.get()};
}

double MyCustomNode_GetCustomValue(MyCustomNode* node)
{
    return node->GetCustomValue();
}

void MyCustomNode_SetCustomValue(MyCustomNode* node, double value)
{
    node->SetCustomValue(static_cast<float>(value));
}
```

**ArkTS 使用**:
```typescript
import { MyCustomNode } from 'graphics3d_scene';

const customNode = new MyCustomNode();
customNode.setCustomValue(42.0);
console.log(`Value: ${customNode.getCustomValue()}`);
```

### 示例 2: 异步加载场景

```typescript
import { Scene } from 'graphics3d_scene';

async function loadSceneAsync() {
    try {
        const scene = await Scene.load($r('app.media.sample_scene'));
        const root = scene.getRoot();

        if (root) {
            console.log(`Root node: ${root.getName()}`);
        }

        // 渲染一帧
        scene.renderFrame({ alwaysRender: true });

    } catch (error) {
        console.error(`Failed to load scene: ${error}`);
    }
}
```

---

## 参考资料

### 核心文档
- [AGP 引擎文档](../README.md)
- [OpenHarmony 图形子系统](https://gitee.com/openharmony/graphic_graphic_3d)
- [ArkTS 语言指南](https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/arkts/arkts-get-started.md)

### Taihe 相关
- [Taihe IDL 语言规范](https://gitee.com/asanrocks/taihe/blob/main/docs/public/spec/IdlReference.md) - **官方 IDL 语法参考**
- [Taihe Cookbook](https://gitee.com/asanrocks/taihe/blob/main/cookbook/README.md) - **实用示例和推荐实践**
- [Taihe 编译器仓库](https://gitee.com/openharmony/taihe_compiler) - 源码和文档

### ANI (Ark Native Interface) 相关
- [ANI 快速入门指南](https://gitcode.com/LeechyLiang/docs/blob/ANIDoc/zh-cn/application-dev/ani/ani-usage-scenarios.md) - **ArkTS 与 C++ 互操作基础**
- [ANI Cookbook](https://gitcode.com/LeechyLiang/ani_cookbook) - **ANI 实用示例集合**
- [ANI 头文件](https://gitcode.com/openharmony/arkcompiler_runtime_core/blob/master/static_core/plugins/ets/runtime/ani/ani.h) - **ANI API 接口定义**

---

**版本**: 1.0
**更新日期**: 2025-01-29
**维护者**: AGP 引擎团队
