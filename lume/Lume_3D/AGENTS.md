---
layer: 3d
deps: [LumeBase, LumeEngine, LumeRender]
summary: ECS架构3D渲染层，39+组件和8系统提供glTF/PBR/动画/光照；组件必须用X-macro+UUID定义，系统执行顺序由systemGraph.json固定不可随意变更
---

# AGENTS.md — Lume_3D

本文件指导大模型（LLM）在修改本模块时遵循的规范和约束。

## 关键约束

### 语言与编译

- **C++17**，`-fno-exceptions`，`-fno-rtti`，`-fvisibility=hidden`
- 导出宏：`CORE3D_PUBLIC`
- 条件编译：`CORE3D_EMBEDDED_ASSETS_ENABLED`、`CORE3D_USE_MESHOPTIMIZER`、`GLTF2_EXTENSION_EXT_MESHOPT_COMPRESSION`

### 命名空间

- 主命名空间：`Core3D`（通过 `CORE3D_BEGIN_NAMESPACE()` / `CORE3D_END_NAMESPACE()`）
- 别名：`CORE3D_NS`、`BASE_NS`、`CORE_NS`、`RENDER_NS`

### 命名规范

| 元素 | 规范 | 示例 |
|------|------|------|
| 接口类 | `I` 前缀 | `IGraphicsContext`、`IRenderSystem` |
| 组件管理器接口 | `I{Name}ComponentManager` | `ITransformComponentManager` |
| 组件数据结构 | `{Name}Component` | `TransformComponent`、`CameraComponent` |
| 系统接口 | `I{Name}System` | `IRenderSystem`、`IAnimationSystem` |
| 实现类 | 无 `I` 前缀 | `NodeSystem`、`RenderSystem` |
| 渲染节点 | `RenderNode{Name}` | `RenderNodeDefaultCameraController` |
| 渲染数据存储 | `RenderDataStore{Name}` | `RenderDataStoreDefaultCamera` |
| 常量 | UPPER_SNAKE_CASE | `CORE_BRDF_PI`、`CORE_MATERIAL_FACTOR_BASE_IDX` |
| UID | `static constexpr BASE_NS::Uid UID { "uuid" }` | 每个组件/系统有唯一 UUID |
| 成员变量 | 尾下划线 | `device_`、`context_` |
| 方法 | PascalCase | `Init()`、`GetRenderContext()` |

### Include 路径

- API 头文件：`#include <3d/namespace.h>`、`#include <3d/ecs/components/transform_component.h>`
- Base/Core/Render：`#include <base/math/vector.h>`、`#include <core/ecs/intf_ecs.h>`
- 内部：`#include "graphics_context.h"`

## ECS 组件定义模式（X-macro）

组件使用唯一的头文件守卫 + `IMPLEMENT_MANAGER` 模式：

```cpp
#if !defined(API_3D_ECS_COMPONENTS_TRANSFORM_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_TRANSFORM_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
// ... includes ...
CORE3D_BEGIN_NAMESPACE()
#endif

BEGIN_COMPONENT(I{Name}ComponentManager, {Name}Component)
    DEFINE_PROPERTY(type, name, "Display Name", flags, default_value)
    DEFINE_ARRAY_PROPERTY(type, count, name, ...)
END_COMPONENT(I{Name}ComponentManager, {Name}Component, "uid-string")

#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif
```

每个组件有唯一 UUID。`IMPLEMENT_MANAGER` 宏触发在对应的 `*_component_manager.cpp` 中自动生成管理器实现。

**新增组件时**：必须分配新的唯一 UUID，遵循此模式。

## ECS 组件（39+ 个）

### 核心

`TransformComponent`（位置/旋转/缩放）、`LocalMatrixComponent`、`WorldMatrixComponent`、`NodeComponent`（父实体/启用标志）、`NameComponent`、`LayerComponent`

### 渲染

`CameraComponent`、`PhysicalCameraComponent`、`LightComponent`、`MeshComponent`、`RenderMeshComponent`、`RenderMeshBatchComponent`（GPU 实例化）、`MaterialComponent`、`GraphicsStateComponent`、`RenderHandleComponent`、`RenderConfigurationComponent`

### 蒙皮与变形

`SkinComponent`、`SkinIbmComponent`、`SkinJointsComponent`、`JointMatricesComponent`、`MorphComponent`

### 动画

`AnimationComponent`、`AnimationInputComponent`、`AnimationOutputComponent`、`AnimationStateComponent`、`AnimationTrackComponent`

### 环境与后处理

`EnvironmentComponent`、`DynamicEnvironmentBlenderComponent`、`FogComponent`、`ReflectionProbeComponent`、`PlanarReflectionComponent`、`PostProcessComponent`、`PostProcessConfigurationComponent`、`PostProcessEffectComponent`

### 天气与水

`WeatherComponent`、`WaterRippleComponent`

## ECS 系统（执行顺序）

在 `assets/3d/systemGraph.json` 中定义：

1. `AnimationSystem` — 动画播放、混合、关键帧插值
2. `LocalMatrixSystem` — 从 TRS 计算本地变换矩阵
3. `NodeSystem` — 场景图层级、世界矩阵传播
4. `WeatherSystem` — 天气效果
5. `SkinningSystem` — 骨骼关节矩阵计算
6. `RenderPreprocessorSystem` — 预渲染数据准备
7. `MorphingSystem` — 变形目标权重处理
8. `RenderSystem` — 主渲染、剔除、绘制调用

**系统间的读写依赖**在 `static_plugin.cpp` 中编码。

## 渲染节点图（14 个预定义管线）

- `core3d_rng_scene.rng` — 基础场景图
- `core3d_rng_cam_scene_fwd.rng` — 标准前向管线
- `core3d_rng_cam_scene_deferred.rng` — 延迟管线
- `core3d_rng_cam_scene_hdrp.rng` — HDR 管线
- MSAA/后处理/预通道/反射相机变体

## 着色器共享头文件

`api/3d/shaders/common/` 包含 20 个 GLSL 头文件，提供 BRDF 函数（GGX/Charlie/Ashikhmin）、光照/阴影计算、IBL、雾效、延迟着色布局等。

## 构建目标

| 目标 | 类型 | 说明 |
|------|------|------|
| `libAGP3D` | ohos_static_library | 静态库（src + ROFS） |
| `libPluginAGP3D` | ohos_shared_library | 可加载 3D 插件（`libPluginAGP3D.z.so`） |
| `AGP3DApi` | ohos_shared_library | API 导出库 |

## 依赖

- **内部**：LumeBase、LumeEngine（含 ecshelper）、LumeRender
- **外部**：`c_utils`、`meshoptimizer`（条件）、`vulkan-headers`/`vulkan-loader`（条件）

## 测试

- `lume_3d_api_test` — API 级别测试（动态链接）
- `lume_3d_src_test` — 内部测试（静态链接）

覆盖：ECS 组件/系统、glTF、渲染数据存储、mesh builder、picking、scene util。

## 修改指南

### 新增 ECS 组件

1. 在 `api/3d/ecs/components/` 创建头文件，遵循 X-macro 模式
2. 分配唯一 UUID
3. 在 `src/ecs/components/` 创建 `*_component_manager.cpp`，使用 `#define IMPLEMENT_MANAGER` 包含头文件
4. 在 `static_plugin.cpp` 的 `RegisterTypes()` 中注册组件管理器
5. 在 `systemGraph.json` 中声明依赖关系（如有系统需要读写）
6. 添加对应测试

### 新增 ECS 系统

1. 在 `api/3d/ecs/systems/` 创建接口头文件
2. 在 `src/ecs/systems/` 实现系统
3. 在 `static_plugin.cpp` 注册系统（含读写依赖声明）
4. 在 `systemGraph.json` 中添加执行顺序
5. 添加对应测试

### 新增渲染数据存储

1. 在 `api/3d/render/` 创建接口
2. 在 `src/render/datastore/` 实现
3. 在 `static_plugin.cpp` 注册
4. 在对应渲染节点中消费数据

### 修改 PBR 材质

1. 材质常量定义在 `api/3d/render/default_material_constants.h`
2. 着色器端 BRDF 函数在 `api/3d/shaders/common/3d_dm_brdf_common.h`
3. GPU buffer 布局在 `3d_dm_structures_common.h`
4. 修改后需验证前向/延迟管线均正确

### 修改动画系统

1. `AnimationSystem` 处理所有动画播放和混合
2. 蒙皮系统（`SkinningSystem`）独立运行，读取 `JointMatricesComponent`
3. 变形（`MorphingSystem`）独立运行
4. 系统执行顺序不可随意变更（依赖链）
