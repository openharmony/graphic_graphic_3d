---
layer: render
deps: [LumeBase, LumeEngine]
summary: 多后端渲染抽象层，通过渲染节点图统一Vulkan和GLES；新增渲染能力必须实现为IRenderNode，禁止在节点中直接调用GL/Vulkan API
---

# AGENTS.md — LumeRender

本文件指导大模型（LLM）在修改本模块时遵循的规范和约束。

## 关键约束

### 语言与编译

- **C++17**，`-fno-exceptions`，`-fno-rtti`，`-fvisibility=hidden`
- 导出宏：`RENDER_PUBLIC`（即 `__attribute__((visibility("default")))`）
- 条件编译：`RENDER_HAS_GLES_BACKEND`、`RENDER_HAS_VULKAN_BACKEND` 控制后端启用

### 命名空间

- 主命名空间：`Render`（通过 `RENDER_BEGIN_NAMESPACE()` / `RENDER_END_NAMESPACE()`）
- 别名：`RENDER_NS`、`BASE_NS`、`CORE_NS`

### 命名规范

| 元素 | 规范 | 示例 |
|------|------|------|
| 接口类 | `I` 前缀 | `IRenderContext`、`IDevice`、`IGpuResourceManager` |
| 实现类 | 无 `I` 前缀 | `Renderer`、`RenderContext`、`Device` |
| 后端实现 | 后端后缀 | `DeviceGLES`、`DeviceVK`、`GpuBufferGles`、`GpuBufferVk` |
| 数据存储 | `RenderDataStore` 前缀 | `RenderDataStorePod`、`RenderDataStorePostProcess` |
| 渲染节点 | `RenderNode` 前缀 | `RenderNodeBloom`、`RenderNodeBackBuffer` |
| 后处理节点 | `RenderPostProcess` + 效果名 | `RenderPostProcessFxaaNode` |
| 枚举 | UPPER_SNAKE_CASE + `CORE_` 前缀 | `CORE_IMAGE_TYPE_2D`、`CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT` |
| 成员变量 | 尾下划线 | `renderContext_`、`device_` |
| UID 常量 | `UID_` 前缀 | `UID_RENDER_PLUGIN`、`UID_RENDER_CONTEXT` |
| 方法 | PascalCase | `Init()`、`GetDevice()`、`RenderFrame()` |

### Include 路径

- API 头文件：`#include <render/namespace.h>`、`#include <render/device/intf_device.h>`
- Base 库：`#include <base/containers/string.h>`
- Core 引擎：`#include <core/plugin/intf_plugin.h>`
- 内部源文件：`#include "renderer.h"`、`#include "device/device.h"`

## 核心架构

### 渲染节点图

渲染管线定义为 `IRenderNode` 实例的有向图。节点从 JSON 描述创建，支持动态插入/删除。`RenderGraph` 自动解析资源依赖并生成 GPU 屏障和转换。

节点生命周期：`InitNode()` → `PreExecuteFrame()` → `ExecuteFrame()`

### 数据存储模式

渲染数据通过 `IRenderDataStore` 实现类流转。数据存储创建一次，每帧由应用/ECS 层填充，渲染节点消费。解耦数据生产与渲染。

### GPU 资源管理

`IGpuResourceManager` 创建/销毁/获取 buffers、images、samplers、acceleration structures。支持命名资源、数据暂存、get-or-create 模式。

### 后端抽象

`IDevice` / `Device` 层次结构提供统一 GPU API。每个后端（`src/vulkan/`、`src/gles/`）实现 GPU buffers、images、samplers、shaders、command lists、swapchains。

## 目录结构要点

```
api/render/          → 公共 API 头文件（所有接口、设备抽象、数据存储、节点图、共享着色器头）
src/                 → 实现代码
  device/            → 设备抽象实现
  gles/              → OpenGL ES 后端（~40 文件）
  vulkan/            → Vulkan 后端（~40 文件）
  datastore/         → 数据存储实现（16 文件）
  nodecontext/       → 渲染节点图引擎（30 文件）
  node/              → 内置渲染节点（32 文件）
  postprocesses/     → 后处理效果节点（22 文件）
  loader/            → 资源加载器（21 文件）
  perf/              → 性能分析
  plugin/            → 插件入口
assets/render/       → 嵌入资产（着色器、管线布局、渲染节点图）
templates/           → 新资源模板文件
```

## 构建目标

| 目标 | 类型 | 说明 |
|------|------|------|
| `libPluginAGPRender` | ohos_shared_library | 最终共享库（`libPluginAGPRender.z.so`） |
| `AGPRenderApi` | ohos_shared_library | 仅 API 共享库 |
| `lume_render_api` | config | 公共 API include 路径 + 后端宏定义 |

## 依赖

- **内部**：LumeBase（base 容器/数学）、LumeEngine（ECS/插件/属性系统）
- **外部（通用）**：`c_utils`、`graphic_surface`、`skia`
- **外部（GLES）**：`graphic_2d:EGL`、`graphic_2d:GLESv3`
- **外部（Vulkan）**：`vulkan-headers`、`vulkan-loader`

## 着色器

### 文件类型

| 扩展名 | 用途 |
|--------|------|
| `.shader` | 着色器定义 JSON（引用 .vert/.frag/.comp） |
| `.shadergs` | 图形状态（blend、depth、stencil） |
| `.shadervid` | 顶点输入声明 |
| `.shaderpl` | 管线布局（描述符集） |
| `.rng` | 渲染节点图 JSON |
| `.vert`/`.frag`/`.comp` | GLSL 源文件（编译为 SPIR-V） |

### 着色器编译流程

GLSL 着色器 → `lume_compile_shader`（SPIR-V 编译）→ `lume_rofs`（打包为 ROFS）→ 链接到共享库。

### 共享着色器头文件

`api/render/shaders/common/` 包含 9 个头文件（tonemap、blur、color conversion、packing、post process 等），LumeRender 和 Lume_3D 共用。

## 测试

位于 `test/unittest/`：
- `lume_render_api_test` — API 级别集成测试
- `lume_render_src_test` — 内部单元测试

涵盖：datastore、device、gles、vulkan、node、nodecontext、loader、postprocesses、util。

## 修改指南

### 新增渲染节点

1. 在 `src/node/` 创建 `render_node_<name>.cpp/.h`
2. 继承 `IRenderNode`，实现 `InitNode()`、`PreExecuteFrame()`、`ExecuteFrame()`
3. 在 `src/node/core_render_node_factory.cpp` 注册工厂方法
4. 在 `templates/rendernodegraphs/` 添加 .rng 模板
5. 在 `implementation_uids.h` 添加 UID 常量
6. 添加对应测试

### 新增后处理效果

1. 在 `src/postprocesses/` 创建 `render_post_process_<name>_node.cpp/.h`
2. 在 `assets/render/shaders/` 添加着色器文件
3. 在 `implementation_uids.h` 添加 UID
4. 在 `RenderContext` 构造函数中注册

### 新增 GPU 资源描述

1. 在 `api/render/device/gpu_resource_desc.h` 添加描述结构体
2. 在 `src/device/gpu_resource_manager.cpp` 实现创建逻辑
3. 在 `src/gles/` 和 `src/vulkan/` 分别实现后端特定逻辑

### 修改着色器

1. 修改 GLSL 源文件（`assets/render/shaders/`）
2. 共享代码放在 `api/render/shaders/common/` 或 `assets/render/shaders/common/`
3. 着色器内使用 `CORE_` 前缀的枚举常量与 C++ 侧保持一致
4. 编译验证：`./build.sh --product-name rk3568 --build-target libPluginAGPRender`
