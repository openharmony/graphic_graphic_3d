---
layer: foundation
deps: []
summary: 引擎最底层纯头文件库，提供STL替代容器和数学库；禁止使用C++ STL，禁止依赖其它Lume模块
---

# AGENTS.md — LumeBase

本文件指导大模型（LLM）在修改本模块时遵循的规范和约束。

## 关键约束

### 语言与编译

- **C++17** 标准，`-fno-exceptions`，`-fno-rtti`，`-fvisibility=hidden`
- **禁止使用 C++ STL**（`<vector>`、`<string>`、`<memory>` 等），只允许 C 标准库头文件（`<cassert>`、`<cstddef>` 等）
- 使用 `memcpy_s`/`memmove_s`/`memset_s`（来自 OpenHarmony `securec.h`）替代不安全的 C 函数
- 使用 `BASE_ASSERT`（debug 模式下等同于 `assert()`，release 模式下为空操作）替代 `throw`/异常

### 命名空间

- 主命名空间：`Base`（通过宏 `BASE_BEGIN_NAMESPACE()` / `BASE_END_NAMESPACE()`）
- 数学子命名空间：`Base::Math`
- 命名空间别名：`BASE_NS`、`CORE_NS`

### 命名规范

| 元素 | 规范 | 示例 |
|------|------|------|
| 类 | PascalCase | `Vec3`、`Mat4X4`、`array_view` |
| 成员函数 | snake_case | `push_back()`、`size()` |
| 数学自由函数 | PascalCase | `Dot()`、`Normalize()`、`Cross()` |
| 容器自由函数 | snake_case | `FNV1aHash()` |
| 成员变量 | 尾下划线 | `ptr_`、`data_`、`size_` |
| 宏 | ALL_CAPS + 前缀 | `BASE_ASSERT`、`BASE_LOG_E`、`BASE_PUBLIC` |
| Include guards | `API_BASE_<SECTION>_<FILE>_H` | `API_BASE_MATH_VECTOR_H` |

### Include 路径

- 公共头文件使用尖括号：`#include <base/containers/vector.h>`
- 模块内部引用：`#include <base/math/vector.h>`（从 `matrix.h` 引用）
- **禁止**引入任何其它 Lume 模块的头文件（LumeBase 零依赖）

## 核心组件

### 容器（`api/base/containers/`）

提供完整的 STL 替代：`vector<T>`、`basic_string<CharT>`、`basic_string_view<CharT>`、`array_view<T>`、`unique_ptr<T,D>`、`shared_ptr<T>`、`refcnt_ptr<T>`、`unordered_map<K,V>`、`flat_map<K,V>`、`byte_array`、`fixed_string`。

所有容器支持自定义 `allocator`（函数指针结构体）。

### 数学库（`api/base/math/`）

- 向量：`Vec2`/`Vec3`/`Vec4`、`UVec2`/`UVec3`/`UVec4`、`IVec2`/`IVec3`/`IVec4`
- 矩阵：`Mat3X3`、`Mat4X4`、`Mat4X3`（列主序，SIMD 优化）
- 四元数：`Quat`（x, y, z, w）
- 工具函数：`vector_util`（Dot、Cross、Normalize）、`matrix_util`（LookAt、Perspective、Inverse）、`quaternion_util`（Slerp、FromEuler）

SIMD 支持平台：x64（SSE/FMA）和 ARM64（NEON）。

### 工具（`api/base/util/`）

`Color`（线性/sRGB 转换）、`Uid`（128-bit UUID）、FNV-1a 哈希、Base64 编解码、UTF-8 解码、`log.h`（BASE_ASSERT/BASE_LOG_E 宏）。

## 构建目标

| 目标 | 类型 | 说明 |
|------|------|------|
| `lume_base_api_config` | config | 导出 `api/` 的 include 路径 |
| `lume_unit_test_config` | config | 单元测试编译配置 |
| `lume_base_api_test` | ohos_unittest | 单元测试（GTest） |
| `unittest` | group | 测试聚合 |

## 测试

测试位于 `test/unittest/`，使用 GTest 框架。涵盖容器（8 文件）、数学（1 文件）、工具（6 文件）。

运行测试：
```bash
./build.sh --product-name rk3568 --build-target lume_base_api_test
```

## 修改指南

### 新增容器/工具类

1. 在 `api/base/<section>/` 下创建头文件
2. Include guard 格式：`API_BASE_<SECTION>_<NAME>_H`
3. 使用 `BASE_BEGIN_NAMESPACE()` / `BASE_END_NAMESPACE()` 包裹
4. 不依赖任何 LumeBase 以外的模块
5. 在 `test/unittest/api_unit_test/src/<section>/` 下添加对应测试
6. 在测试 BUILD.gn 中添加测试源文件

### 修改现有容器

1. 保持与 STL 行为兼容的公共接口
2. 维持 allocator 感知设计
3. 确保 SIMD 优化路径不被破坏（向量/矩阵操作）
4. 运行已有单元测试验证回归

### 修改数学类型

1. 矩阵为列主序存储，此约定不可变更
2. SIMD 平台特定代码使用 `#if` 守卫
3. 所有浮点常量使用 `float` 后缀（如 `1.0f`）
4. 最小粗糙度常量 `0.089` 用于防止除零

## 依赖关系

```
LumeBase（本模块，零依赖）
  ↑ 被所有其它 Lume 模块依赖
  ├── LumeEngine
  ├── LumeRender
  ├── Lume_3D
  ├── LumeMeta
  ├── LumeScene
  └── LumeDotfield 等
```
