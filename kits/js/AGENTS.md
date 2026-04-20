---
layer: binding
deps: [3d_scene_adapter, 3d_widget_adapter, LumeScene, LumeMeta]
summary: NAPI绑定层，30+包装类桥接JS到原生引擎对象；禁止在非JS线程调用NAPI，引擎线程操作必须通过ExecSyncTask调度
---

# AGENTS.md — kits/js

本文件指导大模型（LLM）在修改本模块时遵循的规范和约束。

## 关键约束

### 语言与编译

- **C++17**，与 NAPI（Node-API for OpenHarmony）交互
- 宏定义：`__OHOS_PLATFORM__`、`__SCENE_ADAPTER__`、`CORE_HAS_GLES_BACKEND=1`、`CORE_HAS_VULKAN_BACKEND=1`

### 命名空间

- **NapiApi** — NAPI 包装类（Env、Object、Function、Value 等）
- **JS 包装类** — 全局作用域（无命名空间）
- 别名：`META_NS`、`BASE_NS`、`CORE_NS`、`SCENE_NS`

### 命名规范

| 元素 | 规范 | 示例 |
|------|------|------|
| JS 包装类 | `<Domain>JS` 后缀 | `CameraJS`、`SceneJS`、`MeshJS`、`MaterialJS` |
| 代理类 | `<Type>Proxy` 后缀 | `Vec3Proxy`、`ColorProxy`、`ObjectProxy`、`PropertyProxy` |
| 文件名 | 与类名一致 | `CameraJS.h`/`CameraJS.cpp` |
| JS 属性方法 | `Get<Prop>()`/`Set<Prop>()` | `GetFov()`/`SetFov()` |
| JS 方法 | 以 JS 方法名命名 | `Raycast()`、`WorldToScreen()` |
| 静态初始化 | `static void Init(napi_env, napi_value)` | 每个类都有 |
| 类 ID | `static constexpr uint32_t ID` | 运行时类型识别 |

### Include 路径

- NAPI 抽象：`#include <napi_api.h>`
- 场景接口：`#include <scene/interface/intf_*.h>`
- Meta 接口：`#include <meta/interface/intf_*.h>`
- 内部：`#include "BaseObjectJS.h"`、`#include "SceneJS.h"`

## 核心架构

### 对象继承层次

```
TrueRootObject                → 持有 META_NS::IObject::Ptr，管理 GC 生命周期
  └── BaseObject              → NAPI 包装（napi_wrap），ctor 模板，属性/方法宏
        ├── SceneJS           → 场景根：管理所有场景对象
        ├── NodeJS            → 场景节点 + NodeImpl
        │     ├── CameraJS    → 相机：FOV、near/far、后处理、raycast
        │     ├── GeometryJS  → 几何节点：mesh、morpher
        │     └── BaseLight   → 光源基类
        │           ├── DirectionalLightJS
        │           ├── PointLightJS
        │           └── SpotLightJS
        ├── MeshJS            → 网格
        ├── MaterialJS        → 材质
        ├── AnimationJS       → 动画
        ├── EnvironmentJS     → 环境
        ├── PostProcJS        → 后处理
        └── ...               → 其它 30+ 包装类
```

### NAPI 类注册模式

```cpp
void CameraJS::Init(napi_env env, napi_value exports) {
    BASE_NS::vector<napi_property_descriptor> node_props;
    NodeImpl::GetPropertyDescs(node_props);
    node_props.push_back(NapiApi::GetSetProperty<float, CameraJS,
        &CameraJS::GetFov, &CameraJS::SetFov>("fov"));
    node_props.push_back(NapiApi::Method<...,
        CameraJS, &CameraJS::Raycast>("raycast"));
    napi_define_class(env, "Camera", NAPI_AUTO_LENGTH,
        BaseObject::ctor<CameraJS>(), nullptr,
        node_props.size(), node_props.data(), &func);
    mis->StoreCtor("Camera", func);
}
```

### Getter/Setter 模式

```cpp
napi_value CameraJS::GetFov(NapiApi::FunctionContext<>& ctx) {
    auto cam = GetNativeObject<SCENE_NS::ICamera>();
    return ctx.GetNumber(cam->Fov()->GetValue());
}
void CameraJS::SetFov(NapiApi::FunctionContext<float>& ctx) {
    auto cam = GetNativeObject<SCENE_NS::ICamera>();
    cam->Fov()->SetValue(ctx.Arg<0>());
}
```

### 属性声明辅助宏

```cpp
NapiApi::GetProperty<Type, Class, &Class::Getter>("propName")
NapiApi::SetProperty<Type, Class, &Class::Setter>("propName")
NapiApi::GetSetProperty<Type, Class, &Class::Getter, &Class::Setter>("propName")
NapiApi::Method<FunctionContext<Args...>, Class, &Class::Method>("methodName")
```

### 属性代理系统

- `PropertyProxy` — 双向 JS-to-Native 属性绑定基类
- `Vec3Proxy`/`Vec2Proxy`/`Vec4Proxy` — 数学向量（x/y/z/w）
- `ColorProxy` — 颜色（r/g/b/a）
- `QuatProxy` — 四元数（x/y/z/w）
- `ObjectProxy` — 通用属性映射

### 线程安全

- `ExecSyncTask()` — 在引擎线程执行同步任务
- `NodeJSTaskQueue` — 自定义任务队列，通过 `napi_threadsafe_function` 在 JS 线程执行
- `ThreadSafeCallback` — 跨线程回调（用于 AnimationJS 的 onFinished/onStarted）

### 对象缓存

`JsObjectCache` 防止为同一原生对象创建重复包装器：`FetchJsObj`/`StoreJsObj`/`DetachJsObj`。

## 目录结构要点

```
include/
  napi/                 → NAPI 抽象层（11 头文件）
  geometry_definition/  → 几何图元构建器
  *.h                   → 30+ JS 包装类头文件
src/
  napi/                 → NAPI 抽象实现
  geometry_definition/  → 几何构建器实现
  native_module_export.cpp → NAPI 模块入口
  register_module.cpp   → 所有 JS 类注册
  nodejstaskqueue.cpp   → JS 线程任务队列
  *.cpp                 → 对应包装类实现
```

## 构建目标

| 目标 | 类型 | 说明 |
|------|------|------|
| `libscene` | ohos_shared_library | 最终 NAPI 模块（`libscene.z.so`，注册为 `"scene.napi"`） |
| `libKitHelper` | ohos_shared_library | 绑定代码共享库 |
| `napi_config` | config | 公共 include 路径 |

## 依赖

- **内部**：`3d_widget_adapter:lib3dWidgetAdapter`、`3d_scene_adapter:scene_adapter_config`
- **外部**：`c_utils`、`graphic_2d:EGL/GLESv3`、`graphic_surface:surface`、`hilog:libhilog`、`hitrace:hitrace_meter`、`init:libbegetutil`、`napi:ace_napi`

## 测试

本模块无独立测试。NAPI 绑定层通过 `kits/ets/test/` 的 ETS 级别集成测试验证。

## 修改指南

### 新增 JS 包装类

1. 在 `include/` 创建 `<Name>JS.h`，继承 `BaseObject`
2. 在 `src/` 创建 `<Name>JS.cpp`
3. 实现 `static void Init(napi_env, napi_value)` 注册 JS 类
4. 添加 `static constexpr uint32_t ID` 用于类型识别
5. 实现 Getter/Setter 方法（`Get<Prop>`/`Set<Prop>`）
6. 在 `src/register_module.cpp` 的 `RegisterClasses()` 中添加注册调用
7. 在 `native_module_export.cpp` 中确保模块加载时初始化

### 新增属性代理

1. 在 `include/` 创建 `<Type>Proxy.h`
2. 继承 `PropertyProxy` 或 `ObjectPropertyProxy`
3. 实现 JS 对象的 getter/setter 创建
4. 支持双向同步（JS → Native、Native → JS）

### 修改 NAPI 抽象层

1. NAPI 抽象位于 `include/napi/` 和 `src/napi/`
2. `FunctionContext<Args...>` 提供类型安全的参数访问
3. `MyInstanceState` 存储 JS 构造函数，每个环境一个
4. `ValidateType<T>` 将 C++ 类型映射到 `napi_valuetype`

### 修改线程模型

1. 引擎线程操作必须通过 `ExecSyncTask()` 调度
2. JS 回调必须通过 `NodeJSTaskQueue` 在 JS 线程执行
3. `ThreadSafeCallback` 使用 `napi_threadsafe_function` 确保线程安全
4. 不要在非 JS 线程直接调用 NAPI 函数

### 模块入口

`native_module_export.cpp` 是 NAPI 模块入口：
- 创建 `SceneAdapter` 并初始化引擎
- 创建 `MyInstanceState` 存储构造函数
- 调用 `RegisterClasses()` 注册所有 JS 类
- 模块注册为 `"scene.napi"`
