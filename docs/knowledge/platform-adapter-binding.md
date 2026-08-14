# 平台适配与绑定链路

## 适用范围

本文件用于跨越 `kits/ets`、`kits/js`、`3d_scene_adapter`、`3d_widget_adapter`、`LumeScene` 和 `LumeMeta` 的任务，重点覆盖公开接口同步、线程切换、对象生命周期以及 Surface/Swapchain 所有权。

相关入口：

- [仓级 AGENTS.md](../../AGENTS.md)
- [kits/ets 指引](../../kits/ets/AGENTS.md)
- [kits/js 指引](../../kits/js/AGENTS.md)
- [3d_scene_adapter 指引](../../3d_scene_adapter/AGENTS.md)
- [3d_widget_adapter 指引](../../3d_widget_adapter/AGENTS.md)
- [LumeScene 指引](../../lume/LumeScene/AGENTS.md)
- [LumeMeta 指引](../../lume/LumeMeta/AGENTS.md)
- [ScenePluginAddon 细节](ScenePluginAddon.md)

## 分层职责

```text
ArkTS/ETS                     JavaScript
   │ Taihe/ANI                   │ NAPI
   ▼                             ▼
kits/ets                      kits/js
   └──────────────┬──────────────┘
                  ▼
     SceneBridge / SceneBridgeAni
                  ▼
          3d_scene_adapter
       SceneAdapter / SurfaceStream
                  ▼
          3d_widget_adapter
 GraphicsTask / GraphicsManager / TextureLayer
                  ▼
       LumeScene + LumeMeta
        场景对象 / 属性 / 任务
                  ▼
        Lume ECS / Render
```

该图表达典型调用链。实际实现中，`kits/js`、Scene Adapter 和 Widget Adapter 可能直接消费多个 Lume API；修改前仍需检查对应 `BUILD.gn` 和公开头文件。

| 层 | 核心职责 | 不应承担 |
| --- | --- | --- |
| `kits/ets` | Taihe IDL、ETS 包装、ANI 入口、ArkTS 可见类型 | 不在生成绑定里隐藏原生生命周期策略 |
| `kits/js` | NAPI 类注册、参数转换、属性代理、对象缓存、JS 回调 | 不在非 JS 线程直接调用 NAPI |
| `3d_scene_adapter` | 引擎/插件初始化、Scene 桥接、离屏渲染、SurfaceStream、任务队列 | 不把 JS/ArkTS 类型泄露到 Lume 公共接口 |
| `3d_widget_adapter` | ArkUI/窗口/Surface 适配、GraphicsTask、TextureLayer、引擎工厂 | 不复制场景对象或 ECS 业务逻辑 |
| `LumeScene` | 类型化场景对象、节点、组件、Future/Task、ECS 属性桥接 | 不依赖具体 JS/ETS 运行时 |
| `LumeMeta` | 反射、属性、事件、对象和任务注册 | 不依赖上层平台绑定 |

## 线程模型

Scene Adapter 注册并使用三类关键队列：

| 队列 | 典型用途 | 约束 |
| --- | --- | --- |
| `ENGINE_THREAD` | ECS、场景、渲染上下文和 GPU 相关操作 | 通过已有 TaskQueue/`ExecSyncTask` 调度；同步等待前检查重入与死锁 |
| `IO_QUEUE` | 场景/资源读取和 IO 工作 | 完成后切回正确的 Engine 或 JS 线程，不直接操作 JS 值 |
| `JS_RELEASE_THREAD` | JS 包装对象与原生资源释放协调 | 确认 NAPI 环境仍有效，避免引擎销毁后访问队列 |

跨线程修改检查：

1. 记录调用线程、目标线程、同步/异步语义和回调线程。
2. 检查 lambda 捕获的对象是否在任务执行前销毁；优先使用项目已有弱引用/引用计数模式。
3. 检查 JS/UI → Engine → JS/UI 是否形成互相同步等待。
4. 检查引擎未初始化、队列未注册、应用退出和 NAPI 环境销毁路径。
5. JS 回调使用 `NodeJSTaskQueue`、`ThreadSafeCallback` 等既有机制，不在 Engine/IO 线程直接调用 NAPI。
6. 涉及 ECS/Scene 对象时遵守 [LumeScene 线程安全说明](LumeScene.md#thread-safety)。

## 对象和属性数据流

JS/NAPI 典型链路：

```text
register_module.cpp 注册类
        ↓
<Type>JS 包装器 / BaseObject
        ↓
PropertyProxy / Vec3Proxy / ColorProxy 等
        ↓
SCENE_NS / META_NS 接口与属性
        ↓
LumeScene 属性桥接到 ECS 组件
```

ETS/Taihe 典型链路：

```text
Taihe IDL / 生成接口
        ↓
ETS 包装类与 Taihe Impl
        ↓
SceneBridgeAni / 原生适配接口
        ↓
LumeScene / LumeMeta
```

新增或修改公开属性时同步检查：

- IDL/声明层名称、类型、可空性、默认值和枚举值。
- JS/ETS 包装类注册、getter/setter 和参数校验。
- 向量、颜色、四元数或对象类型是否需要属性代理与双向同步。
- 原生 Scene/Meta 接口、属性注册和 ECS 组件映射。
- `JsObjectCache` 等对象缓存是否仍保证同一原生对象只有一个活动包装器。
- JS、ETS、ANI/NAPI 的错误和异步结果是否一致。
- 对应单测、XTS 和旧调用方兼容性。

## Surface、Swapchain 与 TextureLayer

涉及 `Surface`、`SurfaceBuffer`、`TextureLayer`、Swapchain、离屏目标或相机流时，先回答：

1. 谁创建对象，谁持有引用，谁负责最终释放？
2. acquire/release 是否成对，fence 在何处等待和关闭？
3. Surface 重建、窗口尺寸变化、前后台切换和引擎销毁的顺序是什么？
4. buffer 格式、尺寸、usage、颜色空间和后端布局是否匹配？
5. GLES 与 Vulkan 的创建、转换、同步和 fallback 是否等价？
6. 失败中途是否会遗留 native buffer、swapchain、GPU image、fd 或任务？
7. 回调是否可能晚于 Surface/窗口/NAPI 环境销毁？

mock 单测只能证明局部控制流；真实 buffer、fence、窗口系统、GPU 能力和最终画面必须在设备上验证。

## 常见改动路径

### 新增公开场景类型

1. 在 LumeScene/Meta 定义或复用原生接口与属性。
2. 在 JS/ETS 层增加包装与注册，保持两种语言入口一致。
3. 若涉及平台资源，在 Scene/Widget Adapter 中增加最窄桥接。
4. 添加对象创建、属性读写、异常参数、销毁和跨线程回调测试。
5. 确认 XTS/API 兼容策略。

### 修改引擎初始化或插件装载

1. 检查 EngineFactory、SceneAdapter 初始化顺序和动态库/插件集合。
2. 检查所有中途失败的逆序清理。
3. 检查重复初始化、应用退出和多场景实例。
4. 运行 Scene/Widget Adapter 单测并完成整体构建。

### 修改异步加载或回调

1. 标出 IO、Engine、JS/UI 三个阶段及线程切换点。
2. 明确取消、超时、对象销毁和异常完成语义。
3. 检查回调最多一次、顺序稳定且不持有失效 NAPI 值。

## 验证重点

- ETS：`SceneETSUnitTest`，并参考 [ETS 单测指南](../../kits/ets/test/docs/UNITTEST_GUIDE.md)。
- Scene Adapter：`SceneAdapterUnitTest`、`AGPOffscreenRenderUnitTest`、`SceneBridgeAniUnitTest`、`SceneBridgeUnitTest`、`SurfaceStreamUnitTest`。
- Widget Adapter：`3d_widget_adpater_test` 当前只从 `BUILD.gn` 接入 `3d_widget_adapter_test.cpp`；目录中其它 `*_unit_test.cpp` 的构建接入需另行确认。
- Scene/Meta：`lume_scene_api_test`、`lume_meta_api_test` 及相关 Scene fuzz。
- 公开 API：确认对应 XTS；本仓找不到目标时明确写“XTS 目标未确认”。
- Surface/相机/GPU：记录真实设备、镜像、后端、buffer 参数、步骤、预期和实际结果。

完整目标地图见 [构建与验证地图](build-test-validation.md)。

## Ask First

- 改公开 API/ABI、IDL、枚举、默认值、错误或异步完成语义。
- 改 `ENGINE_THREAD`、`IO_QUEUE`、`JS_RELEASE_THREAD` 或 JS 回调线程模型。
- 改 Surface/buffer/fence 所有权、窗口重建或相机流行为。
- 需要真实设备验证但当前没有设备，仍准备提交或推送。
