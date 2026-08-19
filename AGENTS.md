# graphic_3d 仓库指引

## 项目定位

本仓库对应 OpenHarmony `foundation/graphic/graphic_3d`，实现 AGP（Ark Graphics Platform）3D 引擎、场景抽象、ArkUI 适配以及 ETS/JS 绑定。引擎采用 ECS 架构，渲染侧通过 RenderNode/RenderDataStore 组织管线，并支持 OpenGL ES 与 Vulkan 后端。

按以下目录优先定位任务：

- `kits/ets/`、`kits/js/`：Taihe/ETS 与 NAPI/JS 公开绑定、属性代理、对象缓存和跨线程回调。
- `3d_scene_adapter/`、`3d_widget_adapter/`：Scene/Widget 平台适配、引擎装载、任务队列、Surface/Swapchain、离屏渲染和 ArkUI 集成。
- `lume/LumeScene/`、`lume/LumeMeta/`：场景对象、ECS 属性桥接、反射、属性、事件、动画和序列化。
- `lume/Lume_3D/`、`lume/LumeRender/`：ECS 3D 功能、glTF、材质、动画、RenderNode/RenderDataStore、GPU 资源和 GLES/Vulkan 后端。
- `lume/LumeEngine/`、`lume/LumeBase/`：引擎生命周期、ECS、插件、任务、IO、基础容器和数学库。
- `lume/LumeJpg/`、`lume/LumePng/`：图片加载插件与异常输入 fuzz。
- `camera_preview_plugin/`、`lume/LumeDotfield/`、`lume/LumeMRT/`、`lume/LumeBoidsSwarm/`：相机预览和效果插件。
- `lume/LumeBinaryCompile/`：Shader 与资产编译工具；生成产物不能代替源资产修改。
- `test/` 与各模块的 `test/`：单元测试、fuzz 目标和测试资源。

## 典型工作流

1. 根据文件路径和关键词识别任务场景，读取本文件、目标目录最近的 `AGENTS.md`，再按“知识路由”读取相关 `docs/knowledge/`。
2. 明确公开接口与内部实现边界。涉及 ETS/JS 时从 IDL/声明层向原生层追踪；涉及渲染时从数据生产、RenderDataStore、RenderNode 追踪到 GPU 后端。
3. 沿依赖方向评估影响，优先在调用链较上层完成修改。若需要扩展到用户未指定的更底层公共模块，先说明原因、影响和验证计划并取得确认。
4. 小步修改，就近复用现有 UID、错误处理、日志、容器、线程队列、插件注册和测试模式。
5. 按“验证”矩阵运行最近目标；源码改动还必须完成整仓编译。涉及真实 Surface、GPU 能力、相机流或画面效果时补充设备验证。
6. 最终回复写明读取过的知识文档、修改范围、实际验证、未覆盖的 XTS/设备缺口，以及 commit/push 状态。

## 依赖和接口边界

主要消费链为：

```text
kits/ets、kits/js
        ↓
3d_scene_adapter、3d_widget_adapter
        ↓
LumeScene、LumeMeta
        ↓
Lume_3D、LumeRender
        ↓
LumeEngine
        ↓
LumeBase
```

该图表达主要抽象方向，不替代实际 GN 依赖。图片加载器、效果模块、Camera Preview 和二进制编译工具以插件或构建工具形式依赖 Engine/Render/3D API；修改前仍须检查对应 `BUILD.gn`、公开头文件和插件注册。

常见跨仓边界包括：

- 图形与窗口：`graphic_2d`、`graphic_surface`、`window_manager`、`egl`、`opengles`、`vulkan-loader`。
- ArkUI 与运行时：`arkui`、`ability_runtime`、`napi`、`runtime_core`、`qos_manager`。
- 系统服务：`hilog`、`hitrace`、`input`、`ipc`、`resource_schedule_service`。

触达跨仓接口、枚举、buffer 语义、线程约定或运行时能力时，不能只在本仓闭环；需要检查依赖方公开接口和调用方假设，并在交付说明中记录跨仓影响。

## 构建

完整 OpenHarmony 源码根目录可使用：

```bash
hb build graphic_3d
```

Linux OHOS 产品化构建示例（从提供该脚本的完整产品源码根，例如 `<prefix>/code` 执行；`build_system.sh` 不属于本组件 checkout）：

```bash
./build_system.sh --abi-type generic_generic_arm_64only --device-type general_all_phone_standard --ccache --build-target graphic_3d graphic_3d_ext --build-variant root
```

具体模块目标和测试目标见 [构建与验证地图](docs/knowledge/build-test-validation.md)。当前 checkout 不含完整 OpenHarmony 构建环境时，不要把路径/静态检查描述为编译通过。

## 验证

| 改动类型 | 近端验证 | 额外要求 |
| --- | --- | --- |
| Agent、知识文档、注释 | 检查链接、路径、术语、代码锚点、规则继承和 `git diff --check` | 不改变行为时通常无需编译 |
| 模块内部 C++ | 对应 `*_api_test` / `*_src_test` 或最近模块单测 | 源码改动必须再完成 `graphic_3d` 编译；使用独立构建子任务迭代解决编译错误 |
| ETS/JS/NAPI/ANI | `SceneETSUnitTest`、Scene/Widget Adapter 对应测试 | 检查 IDL、绑定、错误/默认值和跨语言兼容；确认相关 XTS |
| ECS、场景、glTF、JSON、图片加载 | 最近单测 + 对应 fuzz | 覆盖截断、畸形、超大输入和资源释放 |
| Shader、RenderNode、GPU 资源 | `lume_render_*_test`、`lume_3d_*_test`、Shader/资产编译 | 同步检查 GLES/Vulkan；必要时验证真实画面 |
| Surface、Swapchain、相机预览 | `3d_widget_adpater_test`（当前仅接入 WidgetAdapter 主测试）、Scene Adapter 对应单测 | 先确认其它 Widget `*_unit_test.cpp` 的构建接入；需要真实设备验证 buffer、fence、生命周期和 fallback |
| 跨模块或底层公共能力 | 受影响模块测试 + 整体编译 | 列出所有下游模块和未覆盖场景 |

主要近端目标、fuzz 分类、XTS 与设备记录要求统一维护在 [构建与验证地图](docs/knowledge/build-test-validation.md)。没有完整源码根、产品配置、XTS 仓或真实设备时，记录未执行原因、已完成的本地检查和待人工补测命令，不得声称完整验证。

## 知识路由

稳定背景知识放在 `docs/knowledge/`。一个任务跨多个场景时，同时读取多个入口。

| 场景 | 先读 | 就近模块指引 | 验证重点 |
| --- | --- | --- | --- |
| ETS/Taihe、JS/NAPI、SceneBridge、线程队列、Surface | [平台适配与绑定链路](docs/knowledge/platform-adapter-binding.md) | [kits/ets](kits/ets/AGENTS.md)、[kits/js](kits/js/AGENTS.md)、[Scene Adapter](3d_scene_adapter/AGENTS.md)、[Widget Adapter](3d_widget_adapter/AGENTS.md) | 多语言同步、JS/Engine 线程、对象与 Surface 生命周期 |
| LumeScene 节点、组件、属性转发、ECS 接口 | [LumeScene](docs/knowledge/LumeScene.md) | [LumeScene](lume/LumeScene/AGENTS.md)、[LumeMeta](lume/LumeMeta/AGENTS.md) | 属性桥接、Future/Task、对象生命周期 |
| 新增 JS 类型、代理属性、NAPI 包装 | [ScenePluginAddon](docs/knowledge/ScenePluginAddon.md) | [kits/js](kits/js/AGENTS.md) | 类注册、对象缓存、线程安全回调、ETS 同步 |
| ECS 组件/系统、glTF、材质、动画 | [Lume3D](docs/knowledge/Lume3D.md) | [Lume_3D](lume/Lume_3D/AGENTS.md)、[LumeEngine](lume/LumeEngine/AGENTS.md) | 组件管理器、系统顺序、序列化、fuzz |
| RenderNode、RenderDataStore、GPU、Shader、插件/ROFS | [渲染资源与插件链路](docs/knowledge/render-resource-plugin-pipeline.md) | [LumeRender](lume/LumeRender/AGENTS.md)、[Lume 路由](lume/AGENTS.md)、[Camera Preview](camera_preview_plugin/AGENTS.md) | 双后端、资源屏障、UID/注册、资产编译 |
| 构建目标、单测、fuzz、XTS、设备验证 | [构建与验证地图](docs/knowledge/build-test-validation.md) | 目标模块最近的 `AGENTS.md` | 选择最近目标并准确报告环境缺口 |

专项资料：

- ETS 静态 API：[ARKTS_STATIC_API_GUIDE.md](kits/ets/docs/ARKTS_STATIC_API_GUIDE.md)
- ETS 单测：[UNITTEST_GUIDE.md](kits/ets/test/docs/UNITTEST_GUIDE.md)
- Git 提交规则：[AGENTS_COMMIT_RULES.md](AGENTS_COMMIT_RULES.md)

术语路由：

| 触发词 | 优先读取 |
| --- | --- |
| `napi`、`ani`、Taihe、proxy、`SceneBridge`、`ExecSyncTask`、JS 回调 | `platform-adapter-binding.md`、`ScenePluginAddon.md` |
| Surface、SurfaceBuffer、Swapchain、TextureLayer、offscreen、camera preview | `platform-adapter-binding.md`、`3d_widget_adapter/AGENTS.md`、`3d_scene_adapter/AGENTS.md` |
| ECS、component manager、system graph、glTF、animation、material | `Lume3D.md`、`Lume_3D/AGENTS.md`、`LumeEngine/AGENTS.md` |
| RenderNode、RenderDataStore、GPU resource、shader、SPIR-V、ROFS、plugin UID | `render-resource-plugin-pipeline.md`、`LumeRender/AGENTS.md`、`lume/AGENTS.md` |
| fuzz、XTS、真实设备、测试目标、构建命令 | `build-test-validation.md` |

## 项目约束

### Always

- 修改源码前确认目标模块；任务路径明确时无需重复询问，但不得自行把改动扩展到更底层公共模块。
- C++ 约束、命名空间、容器、异常/RTTI 和 include 规则以最近模块 `AGENTS.md` 与 `BUILD.gn` 为准。
- 改公开行为时同步检查 ETS、JS/NAPI/ANI、Scene Adapter 和对应原生接口，而不是只改一个入口。
- 改 GPU/Shader 行为时同步检查 GLES 与 Vulkan 语义、资源布局和生成链路。
- 改插件时同步检查 UID、接口导出、注册/注销、构建目标、安装位置和资产打包。
- 源码修改后必须编译验证；文档修改至少完成链接、锚点和格式检查。

### Ask First

以下场景是继续修改前的门禁：

- 改公开 API/ABI、IDL、枚举、结构体布局、默认值、错误语义或 XTS 预期。
- 改 NAPI/ANI/Taihe 绑定命名、对象缓存、线程模型或回调调度。
- 改 Engine/IO/JS release 队列、锁、原子或跨线程生命周期。
- 改 Surface/SurfaceBuffer/Swapchain/fence、相机流或离屏渲染行为。
- 改 GPU 资源描述、Shader/PipelineLayout/RenderNodeGraph 格式或只实现一个后端。
- 改插件 UID、装载路径、资产格式、ROFS、第三方依赖或产品裁剪。
- 改不可信输入解析、序列化、fuzz 样例或历史安全修复。
- 需要真实设备验证但当前没有设备，仍准备 commit 或 push。

### Never

- 不修改 `submodules/`，除非用户明确要求。
- 不直接编辑 ShaderCompiler/资产编译器生成的 SPIR-V、ROFS 对象或构建输出。
- 不在帧循环、渲染节点、资源上传、属性同步等热点路径增加无界扫描、重复大拷贝或高频 INFO 日志。
- 不绕过现有 Lume 容器、UID、插件、任务队列和资源生命周期抽象。
- 不执行破坏性 Git/文件操作或未经授权的大范围机械重构。
- 不在没有执行证据时声称构建、单测、fuzz、XTS 或真实设备验证通过。

## 提交和推送

任何 commit/push 都必须先读取 [AGENTS_COMMIT_RULES.md](AGENTS_COMMIT_RULES.md)：

- 未经用户明确请求，不执行 `git commit`、`git push` 或其它改变远端/提交历史的操作。
- commit 使用 `git commit -s`，标题采用仓库约定的类型前缀，并以独立段落加入 `Co-Authored-By: Agent`。
- 仅文档、注释、测试或构建配置变更无需稳定性专项检视。
- 业务 C++、公开接口、线程、资源、Surface、GPU/Shader 或插件行为变更，在 commit/push 前询问是否执行 `ohos-dev-graphics-stability-code-review`；检视失败时不得自行 push。

## 完成定义

最终回复必须包含：

- 读取过的知识文档和对应任务场景。
- 修改文件、行为影响面，以及明确未修改的关键模块或后端。
- 实际执行的编译、单测、fuzz、XTS、设备验证或文档检查命令及结果。
- 未执行项的原因、风险和待人工补测内容。
- commit/push 状态；如已执行，说明 sign-off、Agent co-author 和稳定性检视情况。
