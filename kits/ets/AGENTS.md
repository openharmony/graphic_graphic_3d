---
layer: binding
deps: [kits/js, 3d_scene_adapter, 3d_widget_adapter, LumeScene, LumeMeta]
summary: ArkTS/ANI绑定层，通过Taihe IDL生成胶水代码桥接ArkTS到原生引擎；新增API必须先定义.taihe IDL再实现*Impl，禁止绕过Taihe直接暴露C++接口
---
# AGENTS.md — kits/ets

本文件指导大模型（LLM）在修改本模块时遵循的规范和约束。

## 关键约束

### 语言与编译

- **C++17**，与 ANI（ArkTS Native Interface）交互
- 宏定义：`__OHOS_PLATFORM__`、`__SCENE_ADAPTER__`
- Taihe IDL 编译器从 `.taihe` 文件生成 `.abi.c`、`.ani.cpp`、`.ets`、`.proj.hpp`、`.impl.hpp`、`.user.hpp`

### 对象构造禁忌

- **禁止**：禁忌通过 `({nullptr, nullptr})` 构造无效对象作为返回值，会导致未定义行为（如 SIGSEGV 崩溃）

### 命名空间

- ETS 包装类：`OHOS::Render3D`（`include/` + `src/`）
- Taihe 实现：`OHOS::Render3D::KITETS`（`taihe/include/` + `taihe/src/`）
- 别名：`META_NS`、`BASE_NS`、`CORE_NS`、`SCENE_NS`

### 命名规范

| 元素         | 规范                         | 示例                                         |
| ------------ | ---------------------------- | -------------------------------------------- |
| ETS 包装类   | `<Domain>ETS` 后缀         | `SceneETS`、`CameraETS`、`MaterialETS` |
| Taihe 实现类 | `<Domain>Impl` 后缀        | `SceneImpl`、`NodeImpl`、`CameraImpl`  |
| 属性代理     | `<Type>Proxy` 后缀         | `Vec3Proxy`、`ColorProxy`、`QuatProxy` |
| 几何定义     | `CubeETS`/`SphereETS` 等 | 在 `geometry_definition/` 子目录           |
| Taihe IDL    | `<Namespace>.taihe`        | `SceneTH.taihe`、`SceneNodes.taihe`      |
| 文件名       | 与类名一一对应               | `CameraETS.h`/`CameraETS.cpp`            |
| 成员变量     | 尾下划线                     | `sceneAdapter_`、`nodeETS_`、`camera_` |

### Include 路径

- ETS 头文件：`#include "SceneETS.h"`、`#include "CameraETS.h"`
- Taihe 实现头：`#include "SceneImpl.h"`、`#include "ANIUtils.h"`
- 引擎接口：`#include <scene/interface/intf_scene.h>`
- JS 层复用：`#include "BaseObjectJS.h"`、`#include <napi_api.h>`（来自 `kits/js`）

## 核心架构

### 三层结构

```
taihe/idl/*.taihe              → IDL 定义（接口、枚举、联合体）
    ↓ Taihe 编译器
taihe/include/*Impl.h          → 手动实现 IDL 接口
taihe/src/*Impl.cpp            → 持有 shared_ptr<XxxETS>，委托调用
    ↓ 委托
include/*ETS.h + src/*ETS.cpp  → C++ 业务逻辑包装层
    ↓ 包装
LumeScene / LumeMeta           → 原生引擎接口
```

### Taihe IDL 模式

* [ ] IDL 文件定义公共 API，编译器自动生成 ArkTS 侧接口和 C++ 侧胶水代码：

```taihe
@!namespace("Scene")
interface Camera {
    @get float getFov();
    @set void setFov(float fov);
    // ...
}
```

`@gen_promise("name")` 注解将同步方法自动包装为 ArkTS Promise。

### ETS 包装类模式

每个 `*ETS` 类持有 `SCENE_NS` 接口的弱/强引用：

```cpp
class CameraETS : public NodeETS {
    SCENE_NS::ICamera::Ptr camera_;
public:
    float GetFov();
    void SetFov(float fov);
    // ...
};
```

### Taihe Impl 模式

每个 `*Impl` 类持有 `shared_ptr<XxxETS>` 并委托调用：

```cpp
class CameraImpl : public taihe::Camera {
    std::shared_ptr<CameraETS> cameraETS_;
public:
    float getFov() override { return cameraETS_->GetFov(); }
    void setFov(float fov) override { cameraETS_->SetFov(fov); }
};
```

### 属性代理

`PropertyProxy<T>` 模板包装 `META_NS::IProperty::Ptr`，子类提供分量访问：

- `Vec3Proxy`（x, y, z）、`Vec4Proxy`（x, y, z, w）
- `QuatProxy`（x, y, z, w）、`ColorProxy`（r, g, b, a）

### 线程安全

- 复用 `kits/js` 的 `NodeJSTaskQueue` 和 `ExecSyncTask()`
- `CheckNapiEnv.h` 确保模块已加载且 JS 任务队列可用
- 引擎线程操作必须通过 `ExecSyncTask()` 调度

## 目录结构要点

```
include/                → ETS C++ 包装类头文件（29 文件）
  property_proxy/       → 属性代理（8 文件）
  geometry_definition/  → 几何图元定义（7 文件）
src/                    → ETS 包装类实现（一一对应 include/）
taihe/
  idl/                  → Taihe IDL 定义（9 文件）
    *.Transfer.taihe    → 跨语言类型转换 IDL（3 文件）
  ets/                  → 纯 ETS 辅助代码
    RecordProxy.ets     → Record 迭代器
  include/              → ANI 实现头文件（38 文件）
  src/                  → ANI 实现源文件（39 文件）
    ani_constructor.cpp → ANI 模块入口
test/
  docs/                  → 测试参考文档
    UNITTEST_GUIDE.md    → 单元测试环境搭建与开发指南
  unittest/              → GTest 单元测试
    common/ets_test.h    → 测试 fixture（引擎加载/初始化）
docs/
  ARKTS_STATIC_API_GUIDE.md → ArkTS 静态接口开发指南（四层架构、Taihe IDL、属性代理）
```

### IDL 文件与命名空间映射

| IDL 文件                           | 命名空间                     | 定义内容                                                          |
| ---------------------------------- | ---------------------------- | ----------------------------------------------------------------- |
| `SceneTH.taihe`                  | `Scene`                    | Scene、SceneResourceFactory、RenderResourceFactory、RenderContext |
| `SceneNodes.taihe`               | `SceneNodes`               | Node、Camera、Light、Geometry、VariousNodes 联合体                |
| `SceneResources.taihe`           | `SceneResources`           | Material（及子类型）、Mesh、Animation、Environment、Image         |
| `SceneTypes.taihe`               | `SceneTypes`               | Vec2/3/4、Quaternion、Color、Aabb、GeometryDefinition             |
| `ScenePostProcessSettings.taihe` | `ScenePostProcessSettings` | ToneMapping、Bloom、Vignette、ColorFringe                         |
| `SceneBoidsSwarm.taihe`          | `SceneBoidsSim`            | BoidsSimWorld、BoidsSimPlugin                                     |

## 构建目标

| 目标                            | 类型                 | 说明                                        |
| ------------------------------- | -------------------- | ------------------------------------------- |
| `scene_ani`                   | taihe_shared_library | ANI 共享库（`libscene_ani.z.so`）         |
| `graphics3d_scene` 等（6 个） | generate_static_abc  | ArkTS 字节码，安装到 `/system/framework/` |
| `graphics_3d_taihe`           | group                | 顶层聚合目标                                |

## 依赖

- **内部**：`kits/js:libKitHelper`、`3d_widget_adapter:lib3dWidgetAdapter`
- **外部**：`c_utils`、`graphic_2d:EGL/GLESv3`、`graphic_surface:surface`、`hilog:libhilog`、`hitrace:hitrace_meter`、`init:libbegetutil`、`runtime_core:ani_helpers`、`napi:ace_napi`

## 测试

`SceneETSUnitTest` — 当前仅覆盖 `SceneETS::Load` 空路径测试。测试 fixture `EtsTest` 负责引擎加载和插件初始化。详见 `test/docs/UNITTEST_GUIDE.md`。

## 修改指南

### 新增 ETS API（推荐流程）

详见 `docs/ARKTS_STATIC_API_GUIDE.md`。概要步骤：

1. 在 `taihe/idl/` 对应的 `.taihe` 文件中定义接口
2. 运行 Taihe 编译器生成胶水代码
3. 在 `include/` 创建 `*ETS.h`，在 `src/` 创建 `*ETS.cpp`
4. 在 `taihe/include/` 创建 `*Impl.h`，在 `taihe/src/` 创建 `*Impl.cpp`
5. `*Impl` 类持有 `shared_ptr<*ETS>` 并委托所有方法调用
6. 在 `taihe/src/ani_constructor.cpp` 注册新类型

### 新增 Taihe IDL 接口

1. 选择正确的 `.taihe` 文件（按命名空间划分）
2. 使用 `@class`、`@get`/`@set`、`@ctor` 等注解
3. 异步方法用 `@gen_promise("name")` 注解
4. 跨模块引用用 `from X use Y` 语法
5. 修改后需重新运行 Taihe 编译器

### 新增属性代理

1. 在 `include/property_proxy/` 创建头文件，继承 `PropertyProxy<T>`
2. 在 `src/property_proxy/` 实现分量 getter/setter
3. 在对应的 `*ETS` 和 `*Impl` 类中使用

### 新增材质子类型

1. 在 `taihe/idl/SceneResources.taihe` 定义新接口
2. 在 `include/MaterialETS.h` 扩展或创建子类
3. 在 `taihe/include/` 创建对应 `*MaterialImpl.h`
4. 在 `taihe/src/SceneResourceFactoryImpl.cpp` 添加创建逻辑
