---
layer: plugin
deps: [LumeEngine, LumeRender, Lume_3D]
summary: Camera Preview 渲染插件，包含插件注册、Shader 编译、ROFS 资产打包和动态库输出
---

# AGENTS.md — camera_preview_plugin

本模块把相机预览流接入 AGP 渲染插件链。修改前同时读取 [渲染资源与插件链路](../docs/knowledge/render-resource-plugin-pipeline.md)；涉及 Surface/相机 buffer 时再读 [平台适配与绑定链路](../docs/knowledge/platform-adapter-binding.md)。

## 关键锚点

| 路径/目标 | 职责 |
| --- | --- |
| `src/component_dll.cpp` | 插件类和组件注册入口 |
| `api/cam_preview/implementation_uids.h` | 实现 UID |
| `assets/shaders/shader/camera_stream.*` | 相机流顶点/片元 Shader 和 Shader 描述 |
| `assets/pipelinelayouts/camera_stream.shaderpl` | 管线布局 |
| `cam_preview_compile_shader` | Shader 编译目标 |
| `CAM_PREVIEW_ROFS` / `campreview_rofs_obj` | 资产打包和目标架构对象 |
| `libCamPreview` | 静态插件库 |
| `LIB_CAM_PREVIEW` | 安装到 `graphics3d` 的动态插件 |

## 数据和构建链路

```text
camera_stream.vert/.frag + .shader + .shaderpl
        ↓ cam_preview_compile_shader
编译后 Shader/资产
        ↓ CAM_PREVIEW_ROFS
架构相关 ROFS 对象
        ↓ libCamPreview / LIB_CAM_PREVIEW
运行时插件注册与相机预览渲染
```

`CAM_PREVIEW_EMBEDDED_ASSETS_ENABLED` 控制嵌入资产。独立编译场景会改用预构建的 `AGPBinaryCompile`；修改构建链时必须同时检查普通构建与 `ohos_indep_compiler_enable` 分支。

## 修改约束

- UID、插件接口、注册/注销和构建导出必须同步；不要只修改 `implementation_uids.h` 或只修改注册入口。
- `.vert`、`.frag`、`.shader`、`.shaderpl` 是源资产。不得直接编辑生成的 `.spv`、ROFS `.o` 或构建输出。
- 修改描述符、纹理格式、采样器或管线布局时同步检查 Shader 两端、C++ 绑定和 GLES/Vulkan 能力。
- 相机流属于外部 buffer 输入；必须验证格式、尺寸、颜色空间、同步/fence、生命周期和无帧/坏帧路径。
- 插件装载失败、资产缺失或不支持的 GPU 能力必须可诊断并安全退出，不能遗留半注册状态。

## 验证

本模块当前没有独立 `ohos_unittest` 目标，至少执行：

1. `cam_preview_compile_shader` 和 `CAM_PREVIEW_ROFS` 相关构建。
2. `libCamPreview` / `LIB_CAM_PREVIEW` 目标构建。
3. Scene/Widget Adapter 最近单测，确认插件装载与空输入路径没有回归。
4. 真机相机预览验证：记录产品/镜像、输入格式和尺寸、后端、预期/实际画面、切前后台与 Surface 重建结果。

没有完整 OHOS 构建根或真机时，明确记录未执行项，不得声称插件链或预览画面已验证。

## Ask First

- 改插件 UID、动态库名、安装位置、资产 URI 或产品裁剪开关。
- 改相机 buffer/fence 生命周期、格式/颜色空间或 GPU 管线布局。
- 只为单一后端或单一设备能力改变 fallback 行为。

## Never

- 不直接修改生成的 SPIR-V/ROFS 对象。
- 不绕过 ShaderCompiler/资产编译链把临时文件提交为源资产。
- 不把构建成功等同于真机预览、同步和画面正确。
