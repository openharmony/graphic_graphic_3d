---
layer: route
deps: []
summary: Lume 子树总路由；各模块依赖差异较大，实际依赖以最近模块指引和 BUILD.gn 为准
---

# AGENTS.md — lume

本文件适用于 `lume/` 子树。目录中存在更近的 `AGENTS.md` 时，必须同时读取并以更近文档补充本文件；根级安全、验证和提交约束仍然有效。

## 模块路由

| 模块 | 职责 | 就近指引/验证 |
| --- | --- | --- |
| `LumeBase` | 容器、数学、UID、基础工具 | [LumeBase/AGENTS.md](LumeBase/AGENTS.md)，`lume_base_api_test` |
| `LumeEngine` | ECS、插件、任务、IO、资源与平台抽象 | [LumeEngine/AGENTS.md](LumeEngine/AGENTS.md)，`lume_engine_api_test`、`lume_engine_src_test` |
| `LumeRender` | RenderNode、RenderDataStore、GPU 资源、GLES/Vulkan | [LumeRender/AGENTS.md](LumeRender/AGENTS.md)，`lume_render_api_test`、`lume_render_src_test` |
| `Lume_3D` | 3D ECS、glTF、材质、动画、渲染数据生产 | [Lume_3D/AGENTS.md](Lume_3D/AGENTS.md)，`lume_3d_api_test`、`lume_3d_src_test` |
| `LumeMeta` | 反射、属性、事件、动画、序列化 | [LumeMeta/AGENTS.md](LumeMeta/AGENTS.md)，`lume_meta_api_test` |
| `LumeScene` | 场景对象与 ECS/Meta 桥接 | [LumeScene/AGENTS.md](LumeScene/AGENTS.md)，`lume_scene_api_test` |
| `LumeJpg` / `LumePng` | 图片加载动态插件 | `lume_jpg_src_test` / `lume_png_src_test`，对应 ImageLoader fuzz |
| `LumeDotfield` / `LumeBoidsSwarm` / `LumeMRT` | 独立效果或渲染插件 | `lume_dotfield_api_test`、`lume_boids_swarm_api_test`；MRT 以构建与集成验证为主 |
| `LumeBinaryCompile` | ShaderCompiler、资产编译和 ROFS 工具 | `binary_compile_shader`、`binary_compile_asset` 构建目标 |

跨模块链路先读 [渲染资源与插件链路](../docs/knowledge/render-resource-plugin-pipeline.md)，测试目标先读 [构建与验证地图](../docs/knowledge/build-test-validation.md)。

## 共性约束

- 代码通常按 C++17 构建；异常、RTTI、可见性和后端宏以最近模块 `BUILD.gn`/`AGENTS.md` 为准，不能从一个模块推广到整个 `lume/`。
- `LumeBase` 是最底层；新增依赖不得反转核心依赖方向。需要把 Scene/Render/3D 类型引入 Engine/Base 时先确认设计。
- 对外接口和实现 UID 必须保持唯一且成对注册；修改插件类、接口或工厂时同步检查注册、注销和动态库导出。
- 修改 Render/GPU/Shader 语义时同步检查 GLES 与 Vulkan，不得只让单一后端工作。
- Shader 与资产源文件通过编译工具生成 SPIR-V/ROFS；不要直接编辑生成对象或输出目录。
- 图片、JSON、glTF、Shader 和场景导入均处理不可信输入；边界修复应补最近单测和对应 fuzz。

## 插件修改清单

1. 确认插件的公开接口、实现 UID 和注册入口。
2. 确认静态/动态库目标、`relative_install_dir` 和产品裁剪条件。
3. 若包含资产，确认源资产、Shader 编译、ROFS 打包和运行时 URI 一致。
4. 检查插件装载失败、重复注册、注销顺序和资源释放路径。
5. 运行插件最近单测；有解析入口时运行对应 fuzz；涉及画面或 GPU 能力时补设备验证。

## 测试路由

- Engine JSON/图片：`JsonFuzzTest`、`JsonParseFuzzTest`、`JsonTypedFuzzTest`、`ImageLoaderFuzzTest`、`ImageAstcFuzzTest`、`ImageKtxFuzzTest`。
- Render Shader/节点图：`PipelineLayoutLoaderFuzzTest`、`ShaderDataLoaderFuzzTest`、`ShaderStateLoaderFuzzTest`、`RenderNodeGraphLoaderFuzzTest`。
- 3D glTF：`Gltf2LoaderFuzzTest`、`Gltf2JsonLoaderFuzzTest`、`Gltf2GlbBuilderFuzzTest`。
- Scene 导入/属性：`GltfLoadFuzzTest`、`SceneImporterFuzzTest`、`PropertyPathFuzzTest`、Scene `JsonParseFuzzTest`。
- 图片插件：`JpgImageLoaderFuzzTest`、`PngImageLoaderFuzzTest`。

不要假设所有模块存在统一聚合目标；以目标模块 `BUILD.gn` 和 [构建与验证地图](../docs/knowledge/build-test-validation.md) 为准。

## Ask First

- 改 LumeBase/Engine 的公共容器、ECS、插件或任务接口并影响上层模块。
- 改 UID、插件 ABI、动态库名、安装路径、资源 URI 或资产格式。
- 改 GPU 描述、Shader/PipelineLayout/RenderNodeGraph 格式或后端特定行为。
- 改解析/序列化格式、安全边界或 fuzz 回归样例。

## Never

- 不手改生成的 SPIR-V、ROFS 对象和构建输出。
- 不用上层模块依赖反向污染 LumeBase/Engine。
- 不跳过双后端影响分析或把无设备验证描述为画面验证通过。
