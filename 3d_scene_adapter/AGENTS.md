---
layer: adapter
deps: [kits/js, 3d_widget_adapter, LumeEngine]
summary: OpenHarmony平台适配层，桥接应用与Lume引擎，处理Surface和插件加载；所有渲染必须在ENGINE_THREAD执行，SceneAdapter为进程级单例
---

# AGENTS.md — 3d_scene_adapter

本文件指导大模型（LLM）在修改本模块时遵循的规范和约束。

## 关键约束

### 语言与编译

- **C++17**，`-fno-exceptions`，`-fno-rtti`
- 宏定义：`__OHOS_PLATFORM__`、`__SCENE_ADAPTER__`、`CORE_HAS_GLES_BACKEND=1`、`CORE_HAS_VULKAN_BACKEND=1`
- 条件编译：`__FG_MODULE__`（帧生成）、`__SR_MODULE__`（超分辨率）
- 动态引擎库名：`LIB_ENGINE_CORE` 即 `libAGPDLL.z.so`

### 命名空间

- 主命名空间：`OHOS::Render3D`
- 子命名空间：`FG`（帧生成）
- 别名：`BASE_NS`、`CORE_NS`、`META_NS`、`SCENE_NS`、`RENDER_NS`

### 命名规范

| 元素 | 规范 | 示例 |
|------|------|------|
| 接口 | `I` 前缀 | `ISceneAdapter`、`IOffScreenScene`、`ISurfaceStream`、`IMrtDepthAdapter` |
| 实现类 | PascalCase | `SceneAdapter`、`SurfaceStream`、`FGModule`、`SRModule` |
| 成员变量 | 尾下划线 | `sceneWidgetObj_`、`renderContext_`、`ecs_` |
| 静态成员 | 尾下划线 | `engineInitSuccessful_`、`srInitialized_` |
| 方法 | PascalCase | `LoadPluginsAndInit()`、`OnWindowChange()`、`RenderFrame()` |
| Include guards | `OHOS_RENDER_3D_<NAME>_H` | |
| 日志宏 | `WIDGET_LOGD/I/W/E` | 来自 `3d_widget_adapter_log.h` |
| 追踪宏 | `WIDGET_SCOPED_TRACE()` | HiTrace 性能标记 |
| 空检查宏 | `RETURN_IF_NULL`、`RETURN_FALSE_IF_NULL` | |

### Include 路径

- 模块内部：`#include "scene_adapter.h"`（尖括号）
- Lume 引擎：`#include <scene/interface/intf_scene.h>`
- Base/Core：`#include <base/containers/vector.h>`、`#include <core/intf_engine.h>`
- 智能指针：使用 Lume 的 `BASE_NS::shared_ptr`/`BASE_NS::refcnt_ptr` 和 OHOS 的 `sptr`

## 核心架构

```
应用层 (ArkTS/JS)
    ↓
Bridge 层 (SceneBridge/SceneBridgeAni)
    ↓
适配接口 (ISceneAdapter, IOffScreenScene, IMrtDepthAdapter, ISurfaceStream)
    ↓
SceneAdapter（核心实现）
  - 引擎生命周期（dlopen）
  - 插件加载
  - 线程/队列管理
  - Swapchain/渲染目标管理
  - 帧渲染
    ↓
FGModule / SRModule / SurfaceStream
    ↓
Lume 引擎核心
```

### SceneAdapter 核心职责

1. **引擎生命周期** — 单例 `EngineInstance` 管理引擎库句柄、渲染上下文、应用上下文
2. **插件加载** — 通过 `dlopen`/`dlsym` 动态加载 `libAGPDLL.z.so`，加载 scene、camera preview、PNG/JPG 插件
3. **线程管理** — 创建命名任务队列：
   - `ENGINE_THREAD`（UID `2070e705-...`）— 所有渲染操作
   - `IO_QUEUE` — IO 操作
   - `JS_RELEASE_THREAD` — JS 对象清理
4. **渲染管线** — 创建 swapchains、渲染目标（位图）、附加相机
5. **多后端** — 条件使用 OpenGL ES 或 Vulkan
6. **AR 相机预览** — `AcquireImage()`/`UpdateSurfaceBuffer()`/`InitEnvironmentResource()`

### 关键类

| 类 | 说明 |
|---|------|
| `SceneAdapter` | 核心适配器，实现 `ISceneAdapter`，~1334 行 |
| `OffScreenScene` | 离屏渲染，组合 `SceneAdapter`，实现 `IOffScreenScene` |
| `MrtDepthAdapter` | 多渲染目标+深度，实现 `IMrtDepthAdapter` |
| `SurfaceStream` | 外部 Surface 流（生产者/消费者），META 附件 |
| `SceneBridge` | NAPI 桥接，从 JS 对象提取 `ISceneAdapter` |
| `SceneBridgeAni` | ArkTS/ANI 桥接，从 ArkTS 对象提取 `ISceneAdapter` |
| `FGModule` | 帧生成（插帧），使用 FG ECS 组件和渲染节点图 |
| `SRModule` | 超分辨率放大，使用渲染节点图 |
| `CameraConfigs` | 相机配置：位置、旋转、内参、清除色 |

## 目录结构要点

```
include/scene_adapter/
  intf_scene_adapter.h       → ISceneAdapter 接口
  intf_offscreen_scene.h     → IOffScreenScene 接口
  intf_surface_stream.h      → ISurfaceStream 接口
  intf_surface_buffer_info.h → SurfaceBufferInfo 结构体
  intf_camera_utils.h        → CameraConfigs 等相机工具类型
  intf_mrt_depth_adapter.h   → IMrtDepthAdapter 接口
  scene_adapter.h            → SceneAdapter 实现
  scene_bridge.h             → NAPI 桥接
  scene_bridge_ani.h         → ArkTS/ANI 桥接
  surface_stream.h           → SurfaceStream 实现
  fg_module.h / fg_component.h → 帧生成模块和 ECS 组件
  sr_module.h                → 超分辨率模块
src/
  scene_adapter.cpp          → 核心实现（~1334 行）
  scene_bridge.cpp           → NAPI 桥接实现
  scene_bridge_ani.cpp       → ANI 桥接实现
  offscreen_scene.cpp        → 离屏渲染实现
  surface_stream.cpp         → Surface 流实现
  mrt_depth_adapter.cpp      → MRT 深度适配器
  fg_module.cpp              → 帧生成实现
  sr_module.cpp              → 超分辨率实现
  camera_utils.cpp           → 相机工具
```

## 构建目标

| 目标 | 类型 | 说明 |
|------|------|------|
| `scene_adapter_static` | ohos_static_library | 静态库 |
| `sceneAdapterInterface` | group | 导出 `scene_adapter_config` |
| `scene_adapter_config` | config | 公共 include 路径 + `KIT_3D_ENABLE` 定义 |

## 依赖

- **内部**：`kits/js:napiInterface`、`3d_widget_adapter:widget_adapter_config`、LumeEngine（ecshelper）
- **外部**：`ability_runtime`、`bundle_framework`、`c_utils`、`graphic_2d:EGL/GLESv3`、`graphic_surface`、`hilog`、`hitrace`、`init:libbegetutil`、`input:libmmi-client`、`ipc:ipc_single`、`napi:ace_napi`、`vulkan-headers`、`runtime_core:ani`

## 测试

位于 `test/`：
- `SceneAdapterUnitTest` — SceneAdapter 生命周期、引擎初始化、窗口变更、渲染
- `AGPOffscreenRenderUnitTest` — 离屏渲染、相机设置
- `SceneBridgeAniUnitTest` — ANI 桥接空环境处理
- `SceneBridgeUnitTest` — NAPI 桥接空环境处理
- `SurfaceStreamUnitTest` — Surface 流创建、附件、buffer 流

## 修改指南

### 新增适配接口

1. 在 `include/scene_adapter/` 定义接口
2. 在 `src/` 实现，通常组合 `SceneAdapter` 内部实例
3. 在 BUILD.gn 添加源文件
4. 添加对应单元测试

### 修改引擎加载逻辑

1. `SceneAdapter::LoadPluginsAndInit()` 是引擎初始化入口
2. 使用 `dlopen`/`dlsym` 动态加载 `libAGPDLL.z.so`
3. 插件路径通过宏定义（32 位 vs 64 位不同路径）
4. 初始化顺序：引擎库 → 插件注册 → 渲染上下文创建 → ECS 创建

### 修改线程模型

1. `ENGINE_THREAD` — 所有渲染操作必须在此线程
2. `IO_QUEUE` — IO 操作队列
3. `JS_RELEASE_THREAD` — JS 对象释放队列
4. 全局 `std::mutex mute` 用于合成器锁定
5. 帧索引使用 `std::atomic`

### 修改 Surface/Swapchain 管理

1. `OnWindowChange()` 处理 Surface 变更
2. 支持 OpenGL ES（EGL Surface）和 Vulkan（Swapchain）
3. `SurfaceStream` 实现生产者/消费者 Surface 对
4. Buffer 缓存最多 3 个

### 新增 FG/SR 功能

1. 帧生成使用 `#ifdef __FG_MODULE__` 条件编译
2. 超分辨率使用 `#ifdef __SR_MODULE__` 条件编译
3. FG 使用专用 ECS 组件（`IFGComponentManager`）和渲染节点图
4. SR 使用渲染节点图（`sr_bilinear.rng`、`sr_lut.rng`、`sr_hgsr1.rng`）
5. DFX 开关通过系统参数控制（`sys.graphic3D.offscreenRenderDFX`、`AGP_MRT_DEBUG` 等）

### 修改桥接层

1. `SceneBridge` 使用 NAPI `napi_unwrap()` 提取 JS 对象中的原生指针
2. `SceneBridgeAni` 使用 ArkTS ANI 调用 `getSceneNative()` 方法
3. 两者均需处理空环境和空指针情况
