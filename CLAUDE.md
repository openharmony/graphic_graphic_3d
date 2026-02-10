# CLAUDE.md

本文件为 Claude Code (claude.ai/code) 在处理此代码仓库时提供指导。

## 项目概述

这是用于 OpenHarmony 的 **AGP (Ark Graphics Platform) 3D 引擎** —— 一个高性能、跨平台的实时 3D 渲染引擎。该引擎采用实体-组件-系统（ECS）架构，支持 OpenGL ES 和 Vulkan 后端。

## 构建命令

构建整个 graphic_3d 组件：
```bash
./build.sh --product-name rk3568 --build-target graphic_3d
```

构建输出（生成于 `out/` 目录）：
- `lib3dWidgetAdapter.z.so` - ArkUI 的 Widget 适配器
- `libAGPDLL.z.so` - 核心引擎 DLL
- `libPluginAGP3D.z.so` - 3D 渲染插件
- `libPluginAGPRender.z.so` - 渲染后端插件
- `libPluginAGPJpg.z.so` - JPG 图像解码插件
- `libPluginAGPPng.z.so` - PNG 图像解码插件
- `libPluginCamPreview.z.so` - 相机预览流插件，目前仅在AI Engine场景默认使能
- `libPluginDotfield.z.so` - 点云渲特效染插件
- `libPluginSceneWidget.z.so` - 场景渲染插件
- `libPluginMetaObject.z.so` - 元对象插件

构建类型在 `lume/lume_config.gni` 中配置（默认：Release）。

## 测试

测试使用 `ohos_unittest` 框架（Google Test）。每个主要组件都有 `test/` 子目录及其各自的 `BUILD.gn`。

运行所有测试：
```bash
./build.sh --product-name rk3568 --build-target graphic_3d_test
```

各个测试目标遵循每个测试 `BUILD.gn` 中定义的模式。

## 架构

代码库遵循分层架构：

```
应用层 (ArkUI / NAPI / ETS)
         ↓
适配层 (3d_widget_adapter, 3d_scene_adapter)
         ↓
绑定层 (kits/ets, kits/js)
         ↓
场景层 (LumeScene, LumeMeta)
         ↓
引擎核心层 (Lume_3D, LumeRender, LumeEngine, LumeBase)
         ↓
图形后端 (OpenGL ES / Vulkan)
```

### 核心组件 (`lume/`)

- **LumeBase** - 基础：数学库（向量、矩阵、四元数）、容器（vector、map、string）、工具类
- **LumeEngine** - 核心引擎：ECS 框架、资源管理、线程、插件系统、平台抽象
- **Lume_3D** - 3D 功能：GLTF 模型加载、动画（骨骼、变形目标）、PBR 材质、后处理（ToneMapping、Bloom、HDR、FXAA）、几何图元
- **LumeRender** - 渲染：延迟/前向管线、着色器管理、GPU 资源、多后端（GLES/Vulkan）
- **LumeMeta** - 反射：运行时类型信息、属性系统、关键帧动画、事件系统
- **LumeScene** - 场景图：分层场景表示、变换层级、相机系统、组件集成

### 集成层

- **3d_widget_adapter/** - ArkUI 集成：UI 框架集成、输入事件处理、Surface 管理、渲染配置
- **3d_scene_adapter/** - 场景管理：高级场景操作、离屏渲染
- **kits/ets/** - ETS（类 TypeScript）绑定 - 生成为静态 ABC 文件
- **kits/js/** - NAPI 绑定

### 关键架构模式

**ECS（实体-组件-系统）**：贯穿 LumeEngine 和 Lume_3D 的核心设计模式。组件是纯数据结构；系统处理具有特定组件的实体。

**插件系统**：扩展的动态加载（PNG/JPG 解码器、 场景/元数据 转发插件）。插件通过插件注册系统在运行时加载。

**多后端渲染**：着色器编译和 GPU 资源管理抽象了 OpenGL ES 和 Vulkan 的差异。

**属性系统**：LumeMeta 提供具有绑定支持的类型安全属性系统，广泛用于动画和数据流。

## 编程语言

- **C++** (98/11/14/17) - 核心引擎
- **GLSL** - 着色器
- **TypeScript** - 通过 NAPI/TAIHE 绑定

## 构建配置

关键配置文件：`lume/lume_config.gni`

## 依赖

OpenHarmony 组件（来自 `bundle.json`）：
- c_utils, hilog, graphic_2d, graphic_surface, hitrace, init, input, ipc
- napi, ability_runtime, bundle_framework, qos_manager
- 第三方：libpng, libjpeg-turbo, vulkan-loader, skia, freetype, zlib, runtime_core, meshoptimizer

---

## 深入探究：核心 Lume 模块

### 模块依赖层次

```
┌─────────────────────────────────────────────────────────────────┐
│                        应用层                                    │
│                 (ArkUI / NAPI / ETS / JS)                       │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                      适配层                                      │
│           (3d_widget_adapter, 3d_scene_adapter)                 │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                      绑定层                                      │
│                  (kits/ets, kits/js)                            │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                      场景层                                      │
│                  (LumeScene, LumeMeta)                          │
│       (场景图, 节点层级, 对象管理, 反射, 属性系统)               │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                     引擎核心层                                   │
│            (Lume_3D, LumeRender, LumeEngine, LumeBase)          │
│  (3D 渲染, GLTF, PBR 材质, 动画, 后处理, 渲染管线, ECS)         │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    图形后端层                                    │
│                (OpenGL ES / Vulkan)                             │
└─────────────────────────────────────────────────────────────────┘
```

### LumeMeta (`lume/LumeMeta/`)

**用途**：提供反射、运行时类型信息和动态属性系统的基础层。

**目录结构**：
```
lume/LumeMeta/
├── include/meta/
│   ├── api/                  # 公共 API
│   ├── base/                 # 基础类型和宏
│   ├── ext/                  # 扩展功能
│   └── interface/            # 核心接口（30+）
```

**核心概念**：

1. **反射系统**：所有引擎对象的运行时类型内省
2. **属性系统**：具有更改通知的类型安全动态属性
3. **信号/槽**：事件处理机制
4. **对象工厂**：通过类型名称/ID 动态创建对象
5. **序列化**：对象保存/加载功能

### LumeScene (`lume/LumeScene/`)

**用途**：中层场景图，提供分层对象管理，并桥接高级场景概念与底层 ECS。

**目录结构**：
```
lume/LumeScene/
├── include/scene/
│   ├── api/                  # 公共场景 API
│   ├── base/                 # 基础类型和工具
│   ├── ext/                  # 扩展接口和辅助工具
│   └── interface/            # 核心接口（30+）
```

**核心类**：
- **Scene** - 所有场景对象的根容器
- **Node** - 分层场景对象的基类
- **Camera** - 带投影参数的相机节点
- **Light** - 光源节点
- **Geometry** - 网格/可渲染节点
- **Component** - 附加到节点的 ECS 组件包装器

**ECS 桥接模式**：
- 场景节点可以选择性地映射到 ECS 实体
- 附加到节点的组件与 ECS 同步
- 延迟加载 - 仅在需要时创建 ECS 组件
- `IEcsContext` 提供场景与 ECS 之间的桥接

**设计模式**：
- **场景图** - 分层变换传播
- **享元** - 共享资源（材质、网格）
- **组合** - 统一的节点/场景接口
- **包装器** - 场景组件包装 ECS 组件

### Lume_3D (`lume/Lume_3D/`)

**用途**：使用 ECS 架构的高级 3D 渲染功能，以实现最大性能和灵活性。

**目录结构**：
```
lume/Lume_3D/
├── api/3d/
│   ├── ecs/                  # ECS 定义
│   │   ├── components/       # 30+ 组件定义
│   │   └── systems/          # 7 系统接口
│   ├── render/               # 渲染接口
│   ├── gltf/                 # glTF 2.0 加载器
│   ├── shaders/common/       # 共享着色器代码
│   └── util/                 # 工具（拾取、网格构建器等）
├── assets/3d/                # 嵌入资源/着色器
└── src/
    ├── ecs/                  # ECS 实现
    ├── gltf/                 # glTF 加载器实现
    └── render/               # 渲染实现
```

**ECS 组件（关键组件）**：

| 组件 | 用途 |
|-----------|---------|
| `TransformComponent` | 本地位置、旋转、缩放 |
| `WorldMatrixComponent` | 缓存的世界变换矩阵 |
| `CameraComponent` | 相机投影、视口、曝光 |
| `LightComponent` | 光源类型、颜色、强度、阴影 |
| `MeshComponent` | 几何数据引用 |
| `RenderMeshComponent` - 就绪渲染的网格数据 |
| `MaterialComponent` | 材质属性（PBR） |
| `AnimationComponent` | 动画播放状态 |
| `SkinComponent` | 骨骼动画数据 |
| `SkinJointsComponent` | 蒙皮关节索引 |
| `JointMatricesComponent` | 计算的蒙皮矩阵 |
| `MorphComponent` | 变形目标权重 |
| `NodeComponent` | 场景图层级 |
| `LayerComponent` | 渲染层掩码 |
| `EnvironmentComponent` | 环境光照（IBL） |
| `PostProcessComponent` | 后处理配置 |
| `ReflectionProbeComponent` | 反射捕获点 |

**ECS 系统**：
- **RenderSystem** - 主渲染、剔除、绘制调用准备
- **AnimationSystem** - 动画播放和混合
- **NodeSystem** - 变换层级更新
- **SkinningSystem** - 骨骼动画顶点蒙皮
- **MorphingSystem** - 变形目标动画
- **WeatherSystem** - 天气效果（雨、雾等）
- **RenderPreprocessorSystem** - 预渲染数据准备

**功能**：
- **GLTF 2.0** - 完整的 glTF 导入/导出（模型、动画、皮肤）
- **PBR 材质** - 基于物理的渲染（反照率、金属度、粗糙度、法线、遮挡）
- **动画** - 骨骼动画、变形目标、关键帧动画
- **光照** - 方向光、点光源、聚光灯；阴影映射（PCF）
- **后处理** - ToneMapping、Bloom、HDR、FXAA、Blur、DOF
- **环境** - 基于图像的光照、反射探针

### Lume_3D 中的渲染算法

#### PBR（基于物理的渲染）

##### BRDF 模型

引擎实现了基于微表面的 BRDF，具有多种分布函数：

| NDF 类型 | 函数 | 用途 |
|----------|------|------|
| **GGX/Trowbridge-Reitz** | `dGGX()` | 主要分布，粗糙度控制 |
| **Charlie** | `dCharlie()` | 织物/光泽材质 |
| **Ashikhmin** | `dAshikhmin()` | 光泽 BRDF 替代方案 |
| **Lambert** | `dLambert()` | 漫反射（返回 1/π） |

**关键近似**：
- **菲涅尔**：带有 f0/f90 参数的 Schlick 近似
- **几何衰减**：带组合分母的 GGX（`vGGXWithCombinedDenominator`）
- **最小粗糙度**：0.089 以防止除零

**标准 PBR 方程**：
```
f_L = (F · D · G) / (4 · NoV · NoL) + (1 - F) · diffuseColor
```

##### 材质工作流

**金属-粗糙度**（主要）：
- 基础颜色（RGBA）
- 金属度因子（0-1）
- 粗糙度因子（0-1）
- 法向入射反射率（默认 0.04）

**高光-光泽度**（转换）：
- 高光颜色（RGB）
- 光泽度（转换为粗糙度）

**高级材质层**：
- **清漆** - 多层涂层，默认 f0=0.04
- **光泽** - 基础层之上的织物/纤维外观
- **透射** - 半透明/玻璃材质
- **次表面散射** - 用于皮肤、蜡、大理石
- **各向异性** - 拉丝金属的方向性粗糙度
- **高光** - 自定义高光颜色和强度

##### IBL（基于图像的光照）

**环境贴图格式**：
- 立方体贴图辐射率（镜面反射）
- 辐照度贴图（漫反射光照）
- 等距圆柱格式
- RGBD 压缩格式

**球谐函数**：
- 用于漫反射辐照度的 3 阶 SH（9 个系数）
- 函数：`unpackIblIrradianceSH()`

**BRDF LUT 集成**：
- 预积分环境 BRDF
- 近似：`EnvBRDFApprox()`、`EnvBRDFApproxNonmetal()`

**背景类型**：无、图像、立方体贴图、等距圆柱、天空

#### 阴影算法

| 技术 | 描述 |
|------|------|
| **标准阴影映射** | 带可配置偏移的深度比较 |
| **VSM（方差阴影映射）** | 基于矩的柔和阴影，带光泄漏减少 |
| **PCF 3×3** | 9 样本内核，双线性权重（归一化到 144.0） |
| **PCF Medium** | 4 样本优化版本 |
| **PCF Simple** | 单样本快速版本 |

**阴影功能**：
- 深度偏移（可配置近平面和深度偏移）
- 法线偏移（防止阴影痤疮）
- 接收平面偏移（PCF 的透视校正）
- 光泄漏减少（`ReduceLightBleedingVsm()`）

#### 光源类型

| 光源类型 | 描述 | 衰减 |
|----------|------|------|
| **方向光** | 平行光线（太阳/月亮） | 无（无限距离） |
| **点光源** | 全向发射 | 平方反比衰减 |
| **聚光灯** | 带内/外角的方向锥 | 距离 + 角度（GLTF 标准） |
| **矩形区域光** | 物理矩形区域光 | 立体角计算 |

**光源聚类**：平铺/聚类前向渲染中多光源的高效剔除。

#### 色调映射

| 算法 | 描述 |
|------|------|
| **ACES** | 学院色彩编码系统（行业标准） |
| **Filmic** | 电影胶片曲线 |
| **Gamma** | 标准 sRGB 转换 |

可配置曝光和白点。

#### 抗锯齿

| 方法 | 描述 |
|------|------|
| **FXAA** | 快速近似 AA - 后处理边缘平滑，可配置阈值 |
| **TAA** | 时间 AA，基于运动的累积、样本拒绝、速度缓冲区 |

#### 后处理效果

| 效果 | 描述 |
|------|------|
| **Bloom** | 多 mip 级别模糊，带阈值和强度控制 |
| **DOF（景深）** | 散焦模拟，带近/远焦点平面、圆形模糊 |
| **运动模糊** | 时间累积、基于速度、可配置样本数 |
| **暗角** | 径向变暗，带平滑衰减和色调 |
| **镜头光晕** | 带重影伪影的光散射 |

#### 雾

**高度雾**：
- 基于高度的密度衰减
- 双层支持
- 基于距离的淡出
- 最大不透明度控制
- 环境贴图集成以实现真实散射

#### 反射技术

| 技术 | 描述 |
|------|------|
| **平面反射** | 镜面反射，带水波纹模拟 |

#### 环境效果

- **环境光遮蔽** - 材质系统支持烘焙 AO 纹理（非屏幕空间/SSAO）
  - AO 因子与纹理值相乘（0.0 = 无 AO，1.0 = 完全 AO）
  - 镜面 AO 集成：`EnvSpecularAo()` 用于基于环境的遮蔽
- **天空大气** - 程序化天空，带云集成和多次散射

#### 性能功能

- **GPU 实例化** - 相同网格的批量渲染
- **光源聚类** - 多光源的平铺/聚类前向渲染
- **基于层的渲染** - 有组织的渲染通道以提高效率
- **多视图** - VR/立体渲染支持