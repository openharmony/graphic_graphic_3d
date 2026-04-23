---
layer: scene
deps: [LumeBase, LumeEngine, LumeRender, Lume_3D, LumeMeta]
summary: 场景图层，桥接LumeMeta反射与Lume_3D ECS，提供场景图和资源工厂；操作默认异步返回Future<T>，属性通过SCENE_USE_COMPONENT_PROPERTY宏绑定ECS
---

# AGENTS.md — LumeScene

本文件指导大模型（LLM）在修改本模块时遵循的规范和约束。

## 关键约束

### 语言与编译

- **C++17**，`-fno-exceptions`，`-fno-rtti`，`-fvisibility=hidden`
- 导出宏：`CORE3D_PUBLIC`
- 宏定义：`CORE3D_SHARED_LIBRARY=1`、`CORE_USE_COMPILER_GENERATED_STATIC_LIST=1`

### 命名空间

- 主命名空间：`Scene`（通过 `SCENE_BEGIN_NAMESPACE()` / `SCENE_END_NAMESPACE()`）
- 内部命名空间：`SCENE_NS::Internal`
- 别名：`SCENE_NS`、`META_NS`、`BASE_NS`、`CORE_NS`、`RENDER_NS`、`CORE3D_NS`

### 命名规范

| 元素 | 规范 | 示例 |
|------|------|------|
| 接口 | `I` 前缀 | `IScene`、`INode`、`ICamera`、`ILight`、`IMaterial` |
| 实现类 | 描述性名称或 `Object` 后缀 | `SceneObject`、`EcsObject`、`InternalScene`、`Node` |
| API 包装类 | PascalCase | `Scene`、`Node`、`Camera`、`Light`、`Geometry` |
| 枚举 | PascalCase | `CameraProjection`、`LightType`、`MaterialType`、`RenderMode` |
| 属性 | PascalCase | `Position`、`Scale`、`Rotation`、`Enabled`、`FoV` |
| 宏前缀 | `SCENE_` 或 `META_` | `SCENE_USE_COMPONENT_PROPERTY`、`SCENE_STATIC_PROPERTY_DATA` |

### Include 路径

- 场景接口：`#include <scene/interface/intf_scene.h>`
- Meta：`#include <meta/interface/intf_object.h>`
- Core/3D：`#include <core/ecs/intf_ecs.h>`、`#include <3d/intf_graphics_context.h>`
- 内部：`#include "scene.h"`、`#include "core/ecs.h"`

## 核心架构

### 分层设计

```
公共 API 层 (api/scene.h, api/node.h)
    Scene, Node, Camera, Light, Geometry, Material, ...
        ↓
接口层 (interface/)
    IScene, INode, ICamera, ILight, IMaterial, IMesh, ...
        ↓
扩展/内部接口层 (ext/)
    IEcsContext, IEcsObject, IInternalScene, IComponent
        ↓
实现层 (src/)
    SceneObject, InternalScene, EcsObject, Node, CameraNode, ...
```

### 接口模式（基于 LumeMeta 反射）

```cpp
class INode : public ITransform {
    META_INTERFACE(ITransform, INode, "b875c0d3-e974-4106-bbd3-4dae92fe0a50")
public:
    META_PROPERTY(bool, Enabled)
    // ...
};
META_REGISTER_CLASS(Node, "4e5561d1-0313-4922-b91a-816f388efbbf", ...)
META_INTERFACE_TYPE(SCENE_NS::INode)
```

### 实现模式

```cpp
class Node : public META_NS::IntroduceInterfaces<NamedSceneObject, NodeInterface, INodeImport, ...> {
    META_OBJECT(Node, SCENE_NS::ClassId::Node, IntroduceInterfaces)
    META_BEGIN_STATIC_DATA()
    META_STATIC_FORWARDED_PROPERTY_DATA(ITransform, BASE_NS::Math::Vec3, Position)
    META_END_STATIC_DATA()
    SCENE_USE_COMPONENT_PROPERTY(BASE_NS::Math::Vec3, Position, "TransformComponent")
    // ...
};
```

### 异步/Future 模式

几乎所有场景操作返回 `Future<T>`：
- 方法通过 `internal_->AddTaskOrRunDirectly(...)` 分发到 ECS 线程
- API 包装类提供 `META_API_ASYNC` 方法

### 组件属性绑定

组件通过宏将属性转发到 ECS：
```cpp
SCENE_USE_COMPONENT_PROPERTY(BASE_NS::Math::Vec3, Position, "TransformComponent")
```
创建 `PropertyPosition()` 方法，委托给 `GetComponentProperty(this, "TransformComponent", "Position")`。

## 目录结构要点

```
include/scene/
  api/              → 高级便利包装类（Scene, Node, Camera, Light, ...）
  base/             → 命名空间宏、基础类型
  ext/              → 扩展接口（IEcsContext, IEcsObject, IInternalScene）
  interface/        → 核心公共接口（30+ 接口）
    postprocess/    → 14 个后处理效果接口
    resource/       → 资源管理接口
    serialization/  → 导入/导出接口
src/
  core/             → ECS 集成（Ecs, EcsObject, InternalScene）
  component/        → 12 个 ECS 组件包装器
  node/             → 节点实现（Node, CameraNode, LightNode, MeshNode）
  postprocess/      → 14 个后处理效果实现
  resource/         → 资源实现（Material, Image, Shader, Environment, ...）
  mesh/             → 网格实现
  serialization/    → 场景导入/导出
```

## 构建目标

| 目标 | 类型 | 说明 |
|------|------|------|
| `libPluginSceneWidget` | ohos_shared_library | 共享库插件 |
| `AGPSceneApi` | ohos_shared_library | API 导出库 |

## 依赖

- **内部**：LumeMeta（反射/属性/动画）、LumeEngine/ECS（ecshelper）、LumeBase、LumeRender、Lume_3D
- **外部**：`c_utils`
- **条件**：`vulkan-headers`、`graphic_2d:EGL/GLESv3`

## 测试

`lume_scene_api_test` — 覆盖 API、component（12）、core（7）、mesh（3）、node（5）、scene（3）。

## 修改指南

### 新增场景节点类型

1. 在 `include/scene/interface/` 定义节点接口
2. 在 `src/node/` 实现节点类
3. 使用 `META_OBJECT` + `IntroduceInterfaces` 模式
4. 在 `src/plugin.cpp` 注册对象类型
5. 在 `include/scene/api/node.h` 添加 API 包装类

### 新增 ECS 组件包装器

1. 在 `src/component/` 创建组件包装器
2. 使用 `SCENE_USE_COMPONENT_PROPERTY` 宏绑定属性到 ECS 组件
3. 在 `src/plugin.cpp` 注册

### 新增后处理效果

1. 在 `include/scene/interface/postprocess/` 定义接口
2. 在 `src/postprocess/` 实现
3. 在 `src/postprocess/postprocess.cpp` 聚合
4. 在 `src/plugin.cpp` 注册

### 新增资源类型

1. 在 `src/resource/types/` 创建资源类型实现
2. 实现 `IResourceType` 接口
3. 在 `src/plugin.cpp` 注册资源类型

### 修改场景导入/导出

1. 导入器在 `src/serialization/scene_importer.cpp`
2. 导出器在 `src/serialization/scene_exporter.cpp`
3. 序列化节点在 `src/serialization/scene_ser.h`
4. 修改后验证 .gltf/.glb 导入和场景导出均正常

### 修改组件属性桥接

1. `SCENE_USE_COMPONENT_PROPERTY` 宏在 `include/scene/ext/component.h`
2. `SCENE_STATIC_PROPERTY_DATA` 宏在 `include/scene/ext/scene_property.h`
3. ECS 属性获取通过 `GetComponentProperty()` 实现
4. 引擎属性注册在 `src/register_engine_access.cpp`
