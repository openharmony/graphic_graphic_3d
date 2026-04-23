---
layer: core
deps: [LumeBase]
summary: 核心引擎层，提供ECS框架、插件系统和线程池；所有接口继承IInterface并使用refcnt_ptr，全局禁用异常和RTTI
---

# AGENTS.md — LumeEngine

本文件指导大模型（LLM）在修改本模块时遵循的规范和约束。

## 关键约束

### 语言与编译

- **C++17**，`-fno-exceptions`，`-fno-rtti`，`-fvisibility=hidden`
- 导出宏：`CORE_PUBLIC`
- 宏定义：`CORE_BUILD_BASE=1`、`CORE_HIDE_SYMBOLS=1`、`CORE_PERF_ENABLED=0`、`__OHOS_PLATFORM__`

### 命名空间

- 主命名空间：`Core`（通过 `CORE_BEGIN_NAMESPACE()` / `CORE_END_NAMESPACE()`）
- 别名：`CORE_NS`、`BASE_NS`

### 命名规范

| 元素 | 规范 | 示例 |
|------|------|------|
| 接口类 | `I` 前缀 | `IEngine`、`IEcs`、`IComponentManager`、`ISystem`、`IPlugin` |
| 实现类 | 无 `I` 前缀 | `Engine`、`EngineFactory`、`PluginRegistry` |
| 类型信息 | `*TypeInfo` 后缀 | `ComponentManagerTypeInfo`、`SystemTypeInfo` |
| 智能指针 | `using Ptr = BASE_NS::refcnt_ptr<T>` | 接口内定义 |
| 成员变量 | 尾下划线 | `fileManager_`、`plugins_`、`platform_` |
| 常量 | UPPER_SNAKE_CASE | `INVALID_ENTITY`、`INVALID_COMPONENT_ID` |
| 日志宏 | `CORE_LOG_I/E/W/D/F(...)` | `CORE_ASSERT(...)` |
| 方法 | PascalCase | `Init()`、`TickFrame()`、`CreateEcs()` |

### Include 路径

- API 头文件：`#include <core/ecs/entity.h>`、`#include <core/plugin/intf_plugin.h>`
- Base：`#include <base/containers/vector.h>`
- 内部：`#include "engine.h"`、`#include "plugin_registry.h"`

## 核心架构

### 引擎生命周期

1. `CreatePluginRegistry(PlatformCreateInfo)` → 创建全局 `PluginRegistry`
2. `IEngineFactory::Create(EngineCreateInfo)` → 创建 `Engine` 实例
3. `IEngine::Init()` → 创建 `ImageLoaderManager`、加载插件
4. `IEngine::CreateEcs()` → 创建 ECS 实例
5. `IEcs::Initialize()` → 自动发现并创建组件管理器和系统
6. `IEngine::TickFrame()` → 驱动 `IEcs::Update()`
7. `IEcs::Uninitialize()` → 销毁实体和系统

### ECS 帧循环

1. `IEcs::ProcessEvents()` — GC 实体、分发实体/组件事件
2. `IEcs::Update(time, delta)` — 更新所有系统、清除修改标记
3. `IEcs::ProcessEvents()` — 更新后 GC

### 插件生命周期

1. `registerInterfaces`（全局，一次）— 注册接口和类型
2. `IEnginePlugin::createPlugin`（每个引擎实例）
3. `IEcsPlugin::createPlugin`（每个 ECS 实例）
4. 反序 `destroy` 调用
5. `unregisterInterfaces`（全局，一次）

**静态插件**：使用链接器段（`spl.1/spl.2/spl.3`）或构造函数属性自注册。
**动态插件**：从 `.so` 文件运行时加载。

### 引用计数

- `IInterface` 使用 `Ref()/Unref()`（通过 `BASE_NS::refcnt_ptr<T>`）
- `EntityReference` 为实体添加引用计数
- 线程安全（原子操作 `AtomicIncrement`/`AtomicDecrementRelease`）

## 目录结构要点

```
api/core/           → 公共 API 头文件
  ecs/              → ECS 接口（Entity、IEcs、IComponentManager、ISystem）
  plugin/           → 插件系统（IInterface、IClassFactory、IClassRegister、IPluginRegister）
  property/         → 属性系统（IPropertyApi、IPropertyHandle、Property）
  property_tools/   → 属性实现工具（property_data、property_macros）
  io/               → 文件 I/O（IFileManager、IFilesystem、IFile）
  image/            → 图像加载（IImageLoaderManager、IImageContainer）
  threading/        → 线程池（IThreadPool、ITaskQueue）
  perf/             → 性能分析（IPerformanceDataManager、CpuPerfScope）
  resources/        → 资源管理（IResourceManager、IResourceType）
api/platform/       → 平台抽象（common/ + ohos/）
src/                → 实现代码
  ecs/              → ECS 核心（ecs.cpp、entity_manager）
  io/               → 文件系统实现（std、memory、rofs、proxy）
  image/            → 图像加载器（ASTC、KTX、STB）
  threading/        → 线程池实现
  os/               → 平台抽象（ohos/）
ecshelper/          → ECS 辅助工具（BaseManager 模板、ComponentQuery）
DLL/                → 共享库包装（libAGPDLL.z.so）
```

## 构建目标

| 目标 | 类型 | 说明 |
|------|------|------|
| `libAGPEngine` | ohos_static_library | 静态库（引擎核心） |
| `libAGPDLL` (LIB_ENGINE_CORE) | ohos_shared_library | 共享库 `libAGPDLL.z.so` |
| `AGPBaseApi` | ohos_shared_library | 导出 lume_base_api 配置 |
| `AGPEngineApi` | ohos_shared_library | 导出 lume_engine_api 配置 |
| `AGPEcshelperApi` | ohos_shared_library | 导出组件辅助配置 |
| `libAGPEcshelper` | ohos_static_library | ECS 辅助静态库 |
| `libComponentHelper` | ohos_static_library | 组件辅助静态库 |

## 依赖

- **内部**：LumeBase（零依赖基础库）
- **外部**：`c_utils`、`resource_management:global_resmgr`、`qos_manager:qos`、`resource_schedule_service:ressched_client`、`bounds_checking_function:libsec_shared`、`hilog:libhilog`

## 测试

- `lume_engine_api_test` — API 级别测试（ECS、插件、属性、工具）
- `lume_engine_src_test` — 实现级别测试（图像、IO、日志、插件、线程、工具）

## 修改指南

### 新增 ECS 接口

1. 在 `api/core/ecs/` 创建接口头文件
2. 接口继承 `CORE_NS::IInterface`
3. 添加 `using Ptr = BASE_NS::refcnt_ptr<T>`
4. 在 `implementation_uids.h` 添加 UID 常量
5. 在 `src/ecs/` 实现并在插件注册中注册

### 新增文件系统实现

1. 在 `api/core/io/` 定义接口（如已存在则跳过）
2. 在 `src/io/` 实现文件系统类
3. 在 `src/io/filesystem_api.cpp` 注册新文件系统
4. 支持路径协议注册模式（`IFileManager::RegisterProtocol`）

### 新增图像加载器

1. 在 `src/image/loaders/` 创建加载器类
2. 实现 `IImageLoaderManager::IImageLoader` 接口
3. 在 `ImageLoaderManager` 构造函数中注册
4. 添加对应测试

### 修改插件系统

1. 插件描述符类型：`IPlugin`、`IEnginePlugin`、`IEcsPlugin`
2. 静态插件使用 `PLUGIN_DATA(NAME)` + `DEFINE_STATIC_PLUGIN(NAME)` 宏
3. `PluginRegistry` 同时实现 `IPluginRegister`、`IClassRegister`、`IClassFactory`
4. 全局单例：Logger、EngineFactory、SystemGraphLoader、FrustumUtil、TaskQueueFactory

### 使用 ECS Helper（BaseManager 模板）

`ecshelper/ComponentTools/base_manager.h` 提供组件管理器模板基类：
- 实现 `IComponentManager` 和 `IPropertyApi`
- 提供 CRUD、GC、修改跟踪、代数计数器
- `ComponentQuery` 支持多组件查询（REQUIRED/OPTIONAL）
