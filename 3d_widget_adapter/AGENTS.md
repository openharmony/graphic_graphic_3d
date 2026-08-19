---
layer: adapter
deps: [LumeEngine, LumeRender, Lume_3D, graphic_surface, arkui]
summary: ArkUI 与 AGP 引擎适配层，负责 GraphicsTask、Surface/TextureLayer、引擎装载和自定义渲染输入
---

# AGENTS.md — 3d_widget_adapter

本模块是 ArkUI/平台对象与 Lume 引擎之间的适配边界。涉及 Scene Adapter、ETS/JS 或场景对象时，还要读取 [平台适配与绑定链路](../docs/knowledge/platform-adapter-binding.md) 及对应上层指引。

## 关键职责

| 锚点 | 职责 |
| --- | --- |
| `include/ohos/graphics_manager.h`、`src/ohos/graphics_manager.cpp` | OHOS 图形管理、窗口/Surface 与引擎协作 |
| `include/graphics_task.h`、`src/graphics_task.cpp` | 图形任务封装与调度 |
| `include/widget_adapter.h`、`src/widget_adapter.cpp` | Widget 对外适配入口 |
| `include/ohos/texture_layer.h`、`src/ohos/texture_layer.cpp` | TextureLayer 与 Surface/纹理生命周期 |
| `include/engine_factory.h`、`core/src/engine_factory.cpp` | 引擎实例创建和平台实现选择 |
| `core/src/lume/` | Lume 集成、自定义渲染、Shader 输入和渲染配置 |
| `include/offscreen_context_helper.h`、`src/offscreen_context_helper.cpp` | 离屏上下文辅助 |

## 修改约束

- `GraphicsTask`/GraphicsManager 的任务必须保持既有线程与生命周期约定；需要改变执行线程、同步等待或回调顺序时先确认，并联动检查 `3d_scene_adapter`。
- Surface、TextureLayer、窗口变化和离屏上下文涉及生产者/消费者、native buffer、fence 和 GPU 资源；必须明确创建者、持有者、释放线程和异常退出路径。
- `widget_adapter_source` 同时配置 GLES/Vulkan 相关宏；接口或资源语义变更必须检查两个后端，不能只验证当前设备后端。
- 自定义渲染描述和 `ShaderInputBuffer` 是公开输入边界；修改结构、布局、大小或默认值前确认 ABI/API 兼容性。
- 引擎动态库名、插件路径和平台宏由构建配置提供，不要在实现中复制硬编码路径。
- 日志复用 `3d_widget_adapter_log.h`，避免在帧循环、输入事件或资源更新路径增加高频 INFO 日志。

## 构建与测试

- 主库：`lib3dWidgetAdapter`。
- 公共配置/接口：`3dWidgetAdapterInterface`、`widget_adapter_config`。
- 单测：`3d_widget_adpater_test`（仓内目标保留了 `adpater` 拼写）。

当前 `3d_widget_adpater_test` 的 `BUILD.gn` 只编译 `3d_widget_adapter_test.cpp`，用于 WidgetAdapter 主路径。目录中还存在 GraphicsManager、GraphicsTask、EngineFactory、TextureLayer、离屏上下文、数据类型和自定义渲染等 `*_unit_test.cpp`，但尚未在本仓 `BUILD.gn` 中确认构建接入；不能把这些文件视为已运行覆盖。Surface/GPU/窗口行为仍需真实设备补测；没有设备时记录产品、输入、期望行为和待补步骤。

## 修改清单

### Surface/TextureLayer

1. 追踪创建、绑定、窗口变化、销毁和错误返回路径。
2. 检查 buffer/fence 是否在正确线程和唯一位置释放。
3. 检查 GLES 与 Vulkan 分支、应用前后台和 Surface 重建。
4. 运行 `3d_widget_adpater_test` 验证 WidgetAdapter 主路径；为 GraphicsManager、TextureLayer 等改动另行确认测试文件的构建接入，再记录设备验证结果或缺口。

### GraphicsTask/线程

1. 确认调用线程、目标线程、同步/异步语义和对象捕获方式。
2. 禁止让等待环形成 JS/UI/Engine 线程互锁。
3. 对析构、取消、引擎未初始化和队列不可用路径补测试。

### 自定义渲染/Shader 输入

1. 同步检查公开头文件、Lume 适配实现和调用方。
2. 校验 buffer 布局、对齐、范围和 GPU 资源生命周期。
3. 检查两个后端并运行 Shader/资产相关验证。

## Ask First

- 改公开头文件、结构布局、默认值或 ArkUI/应用可见行为。
- 改任务线程、同步等待、Surface/buffer/fence 所有权。
- 改引擎装载、动态库路径、插件集合或后端选择策略。

## Never

- 不在 UI/JS 线程直接执行可能阻塞的渲染或 GPU 操作。
- 不缓存已失效的 Surface、窗口或 GPU 句柄。
- 不只修一个后端，也不把 mock 单测当作真实 Surface/GPU 验证。
