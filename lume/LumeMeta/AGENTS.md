---
layer: meta
deps: [LumeBase, LumeEngine, LumeRender]
summary: 反射和属性系统层，提供META_INTERFACE/META_OBJECT宏、动画和序列化；对象必须用IntroduceInterfaces模式，属性变更通过IBind传播
---

# AGENTS.md — LumeMeta

本文件指导大模型（LLM）在修改本模块时遵循的规范和约束。

## 关键约束

### 语言与编译

- **C++17**，`-fno-exceptions`，`-fno-rtti`，`-fvisibility=hidden`
- 导出宏：`CORE3D_PUBLIC`
- 宏定义：`CORE_USE_COMPILER_GENERATED_STATIC_LIST=1`、`CORE3D_SHARED_LIBRARY=1`

### 命名空间

- 主命名空间：`Meta`（通过 `META_BEGIN_NAMESPACE()` / `META_END_NAMESPACE()`）
- 内部命名空间：`META_NS::Internal`
- 别名：`META_NS`、`BASE_NS`、`CORE_NS`

### 命名规范

| 元素 | 规范 | 示例 |
|------|------|------|
| 接口类 | `I` 前缀 | `IObject`、`IProperty`、`IMetadata`、`IContainer` |
| 接口注册 | `META_REGISTER_INTERFACE(IName, "uuid")` | `META_REGISTER_INTERFACE(IProperty, "772086d4-...")` |
| 类注册 | `META_REGISTER_CLASS(Name, "uuid", category)` | `META_REGISTER_CLASS(AnimationController, "32ccf293-...", NO_CATEGORY)` |
| 智能指针 | `Ptr`、`ConstPtr`、`WeakPtr` | `IObject::Ptr`、`IProperty::ConstPtr` |
| 属性访问器 | `Property##name()` | `PropertyVisible()`、`PropertyName()` |
| 事件访问器 | `Event##name()` | `EventOnChanged()` |
| 模板包装 | `Property<T>`、`ConstProperty<T>`、`ArrayProperty<T>` | `Property<float>` |
| ID 类型 | `TypeId`、`ObjectId`、`InstanceId` | 包装 `BASE_NS::Uid` |
| 宏前缀 | `META_` | `META_INTERFACE`、`META_NO_COPY_MOVE`、`META_PROPERTY` |

### Include 路径

- 公共：`#include <meta/interface/intf_object.h>`
- 内部：`#include "meta_object_lib.h"`
- Base/Core：`#include <base/containers/string.h>`、`#include <core/plugin/intf_interface.h>`

## 核心架构

### 接口声明模式

每个接口使用 `META_INTERFACE` 宏：

```cpp
class IMyInterface : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMyInterface)
public:
    using Ptr = BASE_NS::shared_ptr<IMyInterface>;
    using ConstPtr = BASE_NS::shared_ptr<const IMyInterface>;
    // ... virtual methods ...
};
```

### 对象实现模式

对象使用 `IntroduceInterfaces<...>` 构建：

```cpp
class MetaObject : public IntroduceInterfaces<BaseObject, IMetadata, IOwner, IAttach> {
    // 自动获得引用计数、GetInterface() 分发、编译时接口注册
};
```

### 属性系统

- `IProperty` — 类型擦除属性，`GetValue()`/`SetValue()` 通过 `IAny`
- `Property<T>` — 类型安全包装
- `ConstProperty<T>` — 只读包装
- `ArrayProperty<T>` — 数组属性
- `IAny` / `IArrayAny` — 类型擦除值容器
- `IStackProperty` — 分层属性（值层叠、覆盖）
- `IBind` / `IModifier` — 属性绑定和修改

### 事件系统

- `IEvent` — 事件处理器管理
- `SimpleEvent<...>` — 类型化事件模板
- `Event<T>` — 类型化事件包装

### 容器与层级

- `IContainer` — 子对象容器（add/remove/move/find）
- `IContainerObserver` — 容器变更观察
- `IObjectHierarchyObserver` — 全层级变更观察

### 附件系统

- `IAttach` — 接受附件的对象
- `IAttachment` — 附件标记
- `IAttachmentContainer` — 附件管理

### 动画框架

- `IAnimationController` — 全局动画控制器
- `IAnimation` — 基础动画接口（时间控制、状态管理）
- `IAnimationModifier` — 动画行为修改器（循环、反转、速度）
- `IInterpolator` — 属性值插值

实现类型：关键帧、属性、并行、顺序、轨道、交错动画。

### 序列化

- JSON 导入/导出后端
- v1 向后兼容层
- `IExporter`/`IImporter`、`ISerializable`、`IValueSerializer`

### 引擎桥接

- `IEngineValue` — 同步 Meta 属性值与核心 ECS 属性
- `IEngineValueManager` — 管理引擎值实例
- `IEngineInputPropertyManager` — 管理引擎输入属性

## 目录结构要点

```
include/meta/
  api/            → 高级便利 API 包装
  base/           → 基础类型和宏（19 文件）
  ext/            → 扩展/组合头文件（实现辅助）
  interface/      → 核心接口定义（80+ 接口）
    animation/    → 动画接口（7 文件）
    curves/       → 曲线接口（5 文件）
    detail/       → 实现细节（any、property 等）
    engine/       → 引擎桥接接口（5 文件）
    loaders/      → 内容加载接口（4 文件）
    model/        → 数据模型接口（3 文件）
    property/     → 属性系统接口（12 文件）
    serialization/→ 序列化接口（12 文件）
    threading/    → 线程原语接口
src/              → 实现代码
  animation/      → 动画系统（29 文件）
  container/      → 容器实现（8 文件）
  curves/         → 曲线实现（4 文件）
  engine/         → 引擎桥接实现（4 文件）
  loaders/        → 加载器实现（6 文件）
  model/          → 数据模型实现（8 文件）
  property/       → 属性系统实现（6 文件）
  resource/       → 资源管理（9 文件）
  serialization/  → 序列化实现（12 文件）
```

## 构建目标

| 目标 | 类型 | 说明 |
|------|------|------|
| `libPluginMetaObject` | ohos_shared_library | 最终插件共享库（`libPluginMetaObject.z.so`） |
| `libMetaObject` | ohos_static_library | 静态库 |
| `AGPMetaApi` | ohos_shared_library | API 导出库 |

## 依赖

- **内部**（通过公共配置继承）：LumeBase、LumeEngine、LumeRender
- **外部**：`c_utils`、`qos_manager:qos`、`resource_schedule_service:ressched_client`

## 测试

`lume_meta_api_test` — 覆盖 API、base、ext、interface（动画、引擎值、加载器、模型、属性、资源、序列化等）。

## 修改指南

### 新增接口

1. 在 `include/meta/interface/` 创建接口头文件
2. 使用 `META_INTERFACE(Base, IName, "uuid")` 宏声明
3. 定义 `Ptr`/`ConstPtr` 类型
4. 使用 `META_REGISTER_INTERFACE(IName, "uuid")` 注册
5. 在 `src/plugin.cpp` 的注册函数中包含

### 新增对象实现

1. 在 `src/` 创建实现类
2. 使用 `META_OBJECT(ClassName, ClassId, ...)` 宏
3. 使用 `IntroduceInterfaces<BaseClass, IInterface1, IInterface2, ...>` 声明继承的接口
4. 在 `src/plugin.cpp` 注册类
5. 在 `register_default_objs.cpp` 或 `register_anys.cpp` 中注册

### 新增动画类型

1. 在 `src/animation/` 创建实现
2. 实现 `IAnimation` 接口
3. 在 `register_default_anims.cpp` 注册
4. 在 `src/animation/animation_controller.cpp` 中添加创建逻辑

### 新增序列化后端

1. 在 `src/serialization/backend/` 创建后端实现
2. 实现 `IExporter`/`IImporter` 接口
3. 在 `src/plugin.cpp` 注册

### 修改属性系统

1. 属性声明宏在 `include/meta/interface/property/` 中
2. 属性实现在 `src/property/property.cpp`
3. 属性绑定实现在 `src/property/bind.cpp`
4. 引擎属性桥接在 `src/engine/engine_value.cpp`
5. 修改后验证属性变更通知、绑定同步均正常
