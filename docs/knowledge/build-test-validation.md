# 构建与验证地图

## 适用范围

本文件汇总仓内可确认的构建、单测和 fuzz 目标，并规定 XTS 与真实设备验证的报告方式。目标是否能直接执行取决于完整 OpenHarmony 源码根、产品配置和本机工具链。

相关入口：

- [仓级 AGENTS.md](../../AGENTS.md)
- [Lume 路由](../../lume/AGENTS.md)

## 整体构建

`README.md` 给出的组件入口：

```bash
hb build graphic_3d
```

Linux OHOS 产品化构建示例（从提供该脚本的完整产品源码根，例如 `<prefix>/code` 执行；当前组件 checkout 不包含 `build_system.sh`）：

```bash
./build_system.sh --abi-type generic_generic_arm_64only --device-type general_all_phone_standard --ccache --build-target graphic_3d graphic_3d_ext --build-variant root
```

在完整 OpenHarmony 根目录中也可按环境使用：

```bash
./build.sh --product-name <product> --build-target <target>
```

`<product>` 和目标执行方式由实际源码树/产品决定。仅有本组件 checkout 时，通常无法完成上述 GN 全链路；此时只能做文档、路径和静态检查，不能写“编译通过”。

## 模块单测目标

下表目标均来自仓内 `BUILD.gn`。

| 场景 | 目标 |
| --- | --- |
| LumeBase | `lume_base_api_test` |
| LumeEngine | `lume_engine_api_test`、`lume_engine_src_test` |
| LumeRender | `lume_render_api_test`、`lume_render_src_test` |
| Lume_3D | `lume_3d_api_test`、`lume_3d_src_test` |
| LumeMeta | `lume_meta_api_test` |
| LumeScene | `lume_scene_api_test` |
| Widget Adapter | `3d_widget_adpater_test`（当前仅接入 `3d_widget_adapter_test.cpp`；其它 `*_unit_test.cpp` 未在本仓确认构建接入） |
| Scene Adapter | `SceneAdapterUnitTest`、`AGPOffscreenRenderUnitTest`、`SceneBridgeAniUnitTest`、`SceneBridgeUnitTest`、`SurfaceStreamUnitTest` |
| ETS/Taihe | `SceneETSUnitTest` |
| JPG/PNG | `lume_jpg_src_test`、`lume_png_src_test` |
| Dotfield/Boids | `lume_dotfield_api_test`、`lume_boids_swarm_api_test` |

选目标原则：

1. 先跑改动模块最近的 API/src 或 Adapter 测试。
2. 跨层改动同时跑直接调用方和被调用方测试。
3. 源码改动最后完成 `graphic_3d` 整体编译。
4. 测试目标构建成功不等于目标已在设备/测试环境执行成功；分别报告构建与运行结果。

## Fuzz 目标

| 场景 | 目标 |
| --- | --- |
| Engine JSON | `JsonFuzzTest`、`JsonEscapeFuzzTest`、`JsonParseFuzzTest`、`JsonTypedFuzzTest`、`JsonUnescapeFuzzTest` |
| Engine 图片 | `ImageLoaderFuzzTest`、`ImageAstcFuzzTest`、`ImageKtxFuzzTest` |
| Render Shader/管线 | `PipelineLayoutLoaderFuzzTest`、`ShaderDataLoaderFuzzTest`、`ShaderStateLoaderFuzzTest`、`RenderNodeGraphLoaderFuzzTest` |
| 3D glTF | `Gltf2LoaderFuzzTest`、`Gltf2JsonLoaderFuzzTest`、`Gltf2GlbBuilderFuzzTest` |
| Scene 导入/路径 | `GltfLoadFuzzTest`、`SceneImporterFuzzTest`、`PropertyPathFuzzTest`、Scene `JsonParseFuzzTest` |
| JPG/PNG | `JpgImageLoaderFuzzTest`、`PngImageLoaderFuzzTest` |
| 仓级 RS/IPC | `RSStubFuzzTest`、`RSTransactionIpcFuzzTest` |
| Meta JSON | `JsonImporterFuzzTest` |

Fuzz 报告至少包含目标、构建/运行命令、输入来源与大小、最小化状态、运行时长/次数、退出码、sanitizer、架构和复现结果。只构建或短跑一次不能表述为完整 fuzz 覆盖。

## 尚未确认构建接入的测试源码

以下文件存在于仓内，但当前没有在本仓 `BUILD.gn` 中确认到可运行目标，不能当作已经构建或执行的覆盖：

- `test/fuzztest/scene_init.cpp`、`scene_init.h`：场景/引擎初始化辅助源码；当前未确认 fuzz 消费者。
- `test/render/systemtest/pipeline/rs_base_render_engine_unit_test.cpp`
- `test/render/systemtest/pipeline/rs_drop_frame_processor_unit_test.cpp`
- `test/render/systemtest/pipeline/rs_render_service_unit_test.cpp`
- `test/render/systemtest/pipeline/rs_uni_render_unit_test.cpp`

修改这些源码前先定位实际消费它们的产品或上游 BUILD。查不到时报告“构建目标未确认”；若要在本仓新接入目标，先确认 RS 跨仓依赖、运行设备、测试资源和目标命名，不自行杜撰可运行命令。

## 按改动类型选择验证

| 改动类型 | 必做 | 额外要求 |
| --- | --- | --- |
| 文档、Agent、注释 | Markdown 链接、路径、代码锚点、术语、`git diff --check` | 不改行为时无需编译 |
| C++ 内部实现 | 最近模块单测 + 整体编译 | 错误路径、资源释放、线程退出 |
| ETS/JS/NAPI/ANI | `SceneETSUnitTest` + Adapter 最近测试 | 多语言同步、XTS/API 兼容 |
| ECS/Scene/glTF/JSON/图片 | 最近 API/src 单测 + 对应 fuzz | 畸形、截断、超大输入 |
| Render/GPU/Shader | Render/3D 单测 + Shader/资产/插件构建 | GLES/Vulkan 分别报告，必要时画面验证 |
| Surface/Swapchain/相机 | Widget/Scene Adapter 测试 | 真实设备 buffer/fence/生命周期/画面 |
| Base/Engine 公共能力 | 本模块 + 直接下游测试 + 整体编译 | 列出所有下游影响 |

## XTS

XTS 用例不在本仓完整维护。涉及公开 API、IDL、枚举、默认值、异常、权限、异步结果或多语言一致性时：

1. 查询对应 OpenHarmony XTS 仓、CI 配置或团队映射。
2. 记录确认到的目标和 API 场景。
3. 查不到时在最终回复中写“XTS 目标未确认”，列出已跑本仓测试和需要人工确认的公开行为。
4. 不用本仓单测替代 XTS 兼容性结论。

## 真实设备验证

以下场景通常需要真实设备：Surface/SurfaceBuffer、Swapchain/fence、窗口重建、DMA/外部纹理、相机预览、GLES/Vulkan 能力差异、最终画面和性能。

记录至少包含：

- 产品名、设备形态、系统版本/镜像、CPU/GPU/驱动和渲染后端。
- 输入场景/模型/图片/相机格式、尺寸、颜色空间和关键 buffer 参数。
- 触发 API、命令和操作步骤。
- 期望结果、实际结果、日志/trace/截图位置。
- 前后台、旋转/尺寸变化、Surface 重建、重复进入退出和异常输入结果。

没有设备时不要声称设备验证完成；如改动依赖设备行为，提交或 push 前先取得人工确认。

## 环境不可用时的报告模板

```text
已完成：<路径/链接/静态检查或可运行的本地目标>
未执行：<构建、单测、fuzz、XTS 或设备验证>
原因：<缺少完整 OHOS 根、产品配置、工具链、XTS 仓或真实设备>
待补命令/场景：<已确认的目标名、命令和输入条件>
剩余风险：<公开兼容、线程、资源、后端、画面或安全风险>
```

## 文档变更检查

Agent/知识文档建设至少执行：

```bash
git diff --check
git status --short
```

另外逐项确认 Markdown 相对链接、路由目录、模块 `BUILD.gn` 中的目标名和引用的代码锚点。文档没有改变源码行为时，不需要为满足形式而启动完整编译。
