# 渲染、资源、Shader 与插件链路

## 适用范围

本文件用于跨越 Lume_3D、LumeRender、LumeEngine、Shader/资产编译工具以及各动态插件的任务。

相关入口：

- [仓级 AGENTS.md](../../AGENTS.md)
- [Lume 路由](../../lume/AGENTS.md)
- [Lume_3D 指引](../../lume/Lume_3D/AGENTS.md)
- [LumeRender 指引](../../lume/LumeRender/AGENTS.md)
- [LumeEngine 指引](../../lume/LumeEngine/AGENTS.md)
- [Camera Preview 指引](../../camera_preview_plugin/AGENTS.md)
- [Lume3D ECS 指南](Lume3D.md)

## 渲染数据流

```text
LumeScene / glTF / 应用属性
        ↓
Lume_3D ECS 组件与系统
        ↓ 每帧生产
RenderDataStore（相机、灯光、材质、网格、后处理等）
        ↓ RenderNodeGraph 解析资源依赖
RenderNode::PreExecuteFrame / ExecuteFrame
        ↓
IDevice / GPU resource / command list 抽象
        ├── GLES backend
        └── Vulkan backend
```

- Lume_3D 负责把 ECS/场景状态整理成渲染可消费数据，不应在组件管理器中直接调用 GL/Vulkan。
- LumeRender 的 RenderDataStore 解耦数据生产与 RenderNode 消费；新增字段时同步检查生产者、所有消费者、默认值和逐帧清理。
- RenderNodeGraph 负责节点及资源依赖。新增节点要注册工厂/UID，并通过图描述或模板接入，不能在调用方硬编码执行顺序。
- GPU 资源通过设备与资源管理接口创建/销毁。后端实现必须保持描述语义一致。

## Shader 与资产类型

| 扩展名 | 职责 | 修改时检查 |
| --- | --- | --- |
| `.vert` / `.frag` / `.comp` | GLSL 顶点、片元、计算 Shader 源码 | 入口、宏、描述符、精度、后端能力 |
| `.shader` | Shader 定义，关联阶段和变体 | URI、阶段组合、宏和编译产物 |
| `.shadergs` | Graphics State | blend、depth、stencil、cull 与 pass 预期 |
| `.shadervid` | Vertex Input Declaration | attribute location、格式、stride |
| `.shaderpl` | Pipeline Layout | descriptor set/binding、push constant、阶段可见性 |
| `.rng` | RenderNodeGraph | 节点 UID、输入输出资源、顺序和条件 |

共享 Shader 结构/常量可能同时被 C++ 和 GLSL 使用。调整字段、对齐、枚举或 binding 时，必须同步检查两端和所有变体。

## 编译与打包链路

典型资产链：

```text
GLSL + .shader/.shaderpl/.shadergs/.shadervid/.rng
        ↓ LumeShaderCompiler / lume_compile_shader
SPIR-V 与编译后描述
        ↓ lumeassetcompiler / lume_rofs
架构相关 ROFS 对象
        ↓ 静态/动态插件库
运行时 URI 查找、插件注册与 RenderNode 创建
```

- 普通 OHOS 构建使用仓内 `LumeBinaryCompile` 工具；独立编译可能使用 `prebuilts/graphics_3d/AGPBinaryCompile`。
- 生成的 SPIR-V、ROFS `.o` 和构建目录内容不是源文件，不得直接修改或作为修复提交。
- 修改源资产后验证 Shader 编译、ROFS 打包、最终插件链接和运行时 URI；只验证 GLSL 文本不足以证明链路正确。

## 插件生命周期

LumeEngine 提供插件与类工厂机制。典型动态插件需要保持：

1. 接口 UID、实现 UID 和导出类信息一致且唯一。
2. 注册入口能完整注册类/系统/RenderNode/资源加载器。
3. 注销和动态库卸载按逆序释放，不留下活动实例或任务。
4. 构建目标、动态库名、安装位置和运行时装载路径一致。
5. 资产插件的 URI、ROFS 根和内嵌开关一致。

| 插件/工具 | 主要入口 | 最近验证 |
| --- | --- | --- |
| `LumeJpg` | JPG ImageLoader 插件 | `lume_jpg_src_test`、`JpgImageLoaderFuzzTest` |
| `LumePng` | PNG ImageLoader 插件 | `lume_png_src_test`、`PngImageLoaderFuzzTest` |
| `camera_preview_plugin` | Camera Preview 组件、Shader/ROFS | Shader/ROFS/插件构建 + Scene/Widget Adapter + 真机预览 |
| `LumeDotfield` | Dotfield 效果插件 | `lume_dotfield_api_test` + 画面验证 |
| `LumeBoidsSwarm` | Boids 效果插件 | `lume_boids_swarm_api_test` + 画面验证 |
| `LumeMRT` | MRT 渲染插件 | 插件构建 + Scene Adapter/MRT 集成 + 设备验证 |
| `LumeBinaryCompile` | Shader/资产工具 | `binary_compile_shader`、`binary_compile_asset` 构建链 |

## 双后端同步检查

修改 GPU/Shader/RenderNode 时逐项检查：

- `gpu_resource_desc`、usage、format、layout 和内存属性在 GLES/Vulkan 中是否等价。
- Descriptor/pipeline layout 与 Shader binding 是否一致；后端不支持时 fallback 是否明确。
- RenderPass、attachment、load/store、barrier/transition 是否覆盖全部读写关系。
- 坐标系、深度范围、纹理原点、颜色空间和采样差异是否已由抽象层处理。
- 资源创建失败、Surface 重建、设备丢失或插件缺失时是否安全清理。
- 性能路径是否引入逐帧分配、同步等待、Shader 重编译或重复资源上传。

只在一个后端编译/运行不代表双后端验证完成；最终回复必须分别说明两者状态。

## 常见改动路径

### 新增 RenderNode

1. 定义 `IRenderNode` 实现和生命周期方法。
2. 注册工厂与实现 UID。
3. 定义或更新 `.rng`，声明输入输出和节点顺序。
4. 如有 Shader，完成源码、状态、顶点输入和 pipeline layout。
5. 添加 Render 单测，构建资产，检查两个后端和实际画面。

### 修改 RenderDataStore

1. 找齐 Lume_3D/场景生产者和所有 RenderNode 消费者。
2. 定义逐帧重置、所有权、空值和版本兼容语义。
3. 检查数据大小、对齐和 GPU 上传成本。
4. 运行 3D 与 Render 的 API/src 测试。

### 修改插件或资产

1. 检查 UID、注册/注销、构建目标和安装路径。
2. 从源资产走完整 ShaderCompiler/ROFS 链。
3. 覆盖资产缺失、GPU 能力不足和插件装载失败。
4. 插件有外部输入时补 fuzz；有画面行为时补设备验证。

## 安全和稳定性边界

- `.rng`、Shader 描述、pipeline layout、图片、glTF 和 JSON 都可能来自不可信或损坏输入；检查长度、计数、整数溢出、递归深度和资源上限。
- GPU buffer/image 大小计算必须防止乘法溢出和越界，且不能信任外部 stride/offset。
- 插件卸载前确保任务队列、ECS 系统、RenderNode 和 GPU 资源不再引用动态库代码或静态数据。
- 帧循环中避免无界容器增长、重复大内存拷贝、高频日志和强制 CPU/GPU 同步。

## 验证重点

- Lume_3D：`lume_3d_api_test`、`lume_3d_src_test`、glTF fuzz。
- LumeRender：`lume_render_api_test`、`lume_render_src_test`、Shader/RenderNodeGraph fuzz。
- LumeEngine：`lume_engine_api_test`、`lume_engine_src_test`、JSON/图片 fuzz。
- Shader/资产：对应 `lume_compile_shader`、`lume_rofs`、最终插件链接。
- 设备：分别记录后端、GPU/驱动、资源格式、输入场景、期望/实际画面和性能/稳定性现象。

完整目标见 [构建与验证地图](build-test-validation.md)。

## Ask First

- 改 GPU 公共描述、Shader/PipelineLayout/RenderNodeGraph 格式或公开材质布局。
- 改接口/实现 UID、动态库名、安装目录、运行时 URI 或资产格式。
- 只实现一个后端、改变 fallback，或缺少设备画面验证仍准备提交。
- 修改安全解析、历史漏洞或 fuzz 回归输入。
