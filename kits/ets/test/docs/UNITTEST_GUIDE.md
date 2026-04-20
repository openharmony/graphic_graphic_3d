# ETS Kit 单元测试环境搭建与开发指南

## 目录

- [概述](#概述)
- [目录结构](#目录结构)
- [环境搭建](#环境搭建)
- [编译构建](#编译构建)
- [运行测试](#运行测试)
- [测试框架说明](#测试框架说明)
- [开发新测试用例](#开发新测试用例)
- [推荐实践](#推荐实践)
- [常见问题](#常见问题)

---

## 概述

本指南适用于 AGP (Ark Graphics Platform) 引擎 ETS Kit 的单元测试开发和维护。该测试框架基于 GTest (gtest/gmock) 构建，用于测试 3D 图形引擎的 ETS (Extended TypeScript) 接口层功能。

### 主要特性

- 基于 GTest 框架
- 支持多图形后端（OpenGL ES / Vulkan）
- 完整的引擎生命周期管理
- 自动化的资源管理和清理
- 模块化的测试结构

---

## 目录结构

```
kits/ets/test/unittest/
├── BUILD.gn                     # GN 编译配置文件
├── common/                      # 公共测试基础设施
│   ├── ets_test.h              # 测试基类头文件
│   └── ets_test.cpp            # 测试基类实现
└── api_unit_test/              # API 单元测试
    └── scene_ets_unit_test.cpp # SceneETS 测试用例
```

### 文件说明

#### `BUILD.gn`
GN (Generate Ninja) 构建系统的配置文件，定义了：
- 测试目标名称：`SceneETSUnitTest`
- 头文件搜索路径
- 源文件列表
- 编译选项和定义
- 依赖项（内部和外部）

#### `common/ets_test.h` & `common/ets_test.cpp`
测试基础设施，提供：
- `EtsTest` 基类：继承自 `::testing::Test`
- 完整的引擎初始化流程
- 自动化的资源清理

#### `api_unit_test/`
存放针对各个 API 的测试用例，按模块分类。

---

## 环境搭建

### 前置依赖

#### 必需组件

1. **OpenHarmony 编译环境**
   - GN 构建工具
   - Ninja 编译工具
   - LLVM 工具链

2. **系统依赖**
   - `ability_runtime`: Ability 运行时
   - `graphic_2d`: 2D 图形库（EGL/GLES）
   - `napi`: Node.js API
   - `hilog`: 日志系统
   - `googletest`: 单元测试框架

3. **引擎依赖**
   - `3d_widget_adapter`: 3D 组件适配器
   - `scene_ani`: 场景动画
   - `libKitHelper`: 辅助工具库

#### 编译器要求

- 支持 C++17 标准
- 支持 OpenGL ES 3.x 或 Vulkan

---

## 编译构建

### 1. 配置编译参数

在编译前，确保以下参数已正确配置：

```bash
# 在编译命令中设置目标 CPU 架构
target_cpu = "arm64"  # 或 "arm"
```

系统会根据 CPU 架构自动设置：
- `PLATFORM_CORE_ROOT_PATH`: 核心库路径
- `PLATFORM_CORE_PLUGIN_PATH`: 核心插件路径
- `PLATFORM_APP_ROOT_PATH`: 应用库路径
- `PLATFORM_APP_PLUGIN_PATH`: 应用插件路径

### 2. 编译命令

```bash
# 编译整个 graphic_3d 模块（包括测试）
hb build graphic_3d

# 仅编译测试模块
hb build graphic_3d_kits_ets_test_unittest
```

### 3. 输出位置

编译成功后，测试可执行文件位于：
```
out/[product_name]/tests/graphic_3d/graphic_3d/kits/ets/SceneETSUnitTest
```

---

## 运行测试

### 在设备上运行

```bash
# 推送测试到设备
hdc file send out/[product_name]/tests/.../SceneETSUnitTest /data/local/tmp/

# 添加执行权限
hdc shell chmod 755 /data/local/tmp/SceneETSUnitTest

# 运行测试
hdc shell /data/local/tmp/SceneETSUnitTest
```

### 测试输出示例

```
[==========] Running 1 test from 1 test suite.
[----------] Global test environment set-up.
[----------] 1 test from SceneETSUnitTest
[ RUN      ] SceneETSUnitTest.SceneETS_Load_001
[       OK ] SceneETSUnitTest.SceneETS_Load_001 (XXX ms)
[----------] 1 test from SceneETSUnitTest (XXX ms total)

[==========] 1 test from 1 test suite ran. (XXX ms total)
[  PASSED  ] 1 test.
```

---

## 测试框架说明

### EtsTest 基类

`EtsTest` 是所有单元测试的基类，负责初始化完整的 3D 引擎环境。

#### 类定义

```cpp
class EtsTest : public ::testing::Test {
public:
    void SetUp() override;    // 每个测试用例前执行
    void TearDown() override; // 每个测试用例后执行

private:
    void LoadEngineLib();
    void LoadPlugins(const CORE_NS::PlatformCreateInfo&);
    void CreateEngine(const CORE_NS::PlatformCreateInfo&);
    void CreateRenderContext();
    void CreateGraphicsContext();
    void CreateApplicationContext();

    void* libHandle_;                           // 引擎库句柄
    CORE_NS::IEngine::Ptr engine_;              // 引擎实例
    BASE_NS::shared_ptr<RENDER_NS::IRenderContext> renderContext_;
    CORE3D_NS::IGraphicsContext::Ptr graphicsContext3D_;
    SCENE_NS::IApplicationContext::Ptr applicationContext_;
};
```

#### 初始化流程

1. **LoadEngineLib()**: 动态加载引擎核心库 `lib_engine_core.z.so`
2. **LoadPlugins()**: 加载必要的插件（如场景插件）
3. **CreateEngine()**: 创建引擎实例并初始化文件管理器
4. **CreateRenderContext()**: 创建渲染上下文，支持 Vulkan/GLES 后端
5. **CreateGraphicsContext()**: 创建 3D 图形上下文
6. **CreateApplicationContext()**: 创建应用上下文，包括任务队列和资源管理器

#### 清理流程

`TearDown()` 会自动清理所有资源，包括：
- 注销所有任务队列
- 释放资源
- 关闭引擎库句柄

### 测试用例宏

使用 OpenHarmony 的测试宏：

```cpp
HWTEST_F(测试类名, 测试用例名, 测试级别)
```

- `HWTEST_F`: 固定测试装置（Fixture）
- 测试级别：
  - `TestSize.Level0`: 基础级别
  - `TestSize.Level1`: 中等级别（最常用）
  - `TestSize.Level2`: 高级别
  - `TestSize.Level3`: Level3

### 测试用例注释规范

每个测试用例必须包含以下注释：

```cpp
/**
 * @tc.name: 测试用例名称_编号
 * @tc.desc: 测试用例描述
 * @tc.type: FUNC (功能测试) / PERF (性能测试)
 */
```

示例：
```cpp
/**
 * @tc.name: SceneETS_Load_001
 * @tc.desc: Test SceneETS::Load with empty path
 * @tc.type: FUNC
 */
HWTEST_F(SceneETSUnitTest, SceneETS_Load_001, TestSize.Level1)
{
    // 测试代码
}
```

---

## 开发新测试用例

### 步骤 1: 创建测试文件

在 `api_unit_test/` 目录下创建新的测试文件，例如 `material_ets_unit_test.cpp`。

### 步骤 2: 包含必要的头文件

```cpp
#include <gtest/gtest.h>
#include <memory>

#include "common/ets_test.h"
#include "MaterialETS.h"  // 被测试的头文件
```

### 步骤 3: 定义测试类

```cpp
using namespace testing;
using namespace testing::ext;

namespace OHOS::Render3D {

class MaterialETSUnitTest : public EtsTest {
    // 可以添加额外的成员变量或方法
};

} // namespace OHOS::Render3D
```

### 步骤 4: 编写测试用例

```cpp
/**
 * @tc.name: MaterialETS_Create_001
 * @tc.desc: Test MaterialETS creation and initialization
 * @tc.type: FUNC
 */
HWTEST_F(MaterialETSUnitTest, MaterialETS_Create_001, TestSize.Level1)
{
    // Arrange (准备)
    auto material = std::make_shared<MaterialETS>();

    // Act (执行)
    bool result = material->IsValid();

    // Assert (断言)
    EXPECT_TRUE(result);
}

/**
 * @tc.name: MaterialETS_SetProperty_001
 * @tc.desc: Test setting material properties
 * @tc.type: FUNC
 */
HWTEST_F(MaterialETSUnitTest, MaterialETS_SetProperty_001, TestSize.Level1)
{
    auto material = std::make_shared<MaterialETS>();

    // 测试属性设置
    material->SetAlbedoColor(1.0f, 0.0f, 0.0f); // 红色
    EXPECT_EQ(material->GetAlbedoColor(), Color(1.0f, 0.0f, 0.0f));
}
```

### 步骤 5: 更新 BUILD.gn

在 `BUILD.gn` 的 `sources` 列表中添加新文件：

```gni
sources = [
    "common/ets_test.cpp",
    "api_unit_test/scene_ets_unit_test.cpp",
    "api_unit_test/material_ets_unit_test.cpp",  # 新增
]
```

### 步骤 6: 编译并运行测试

```bash
# 重新编译
hb build graphic_3d_kits_ets_test_unittest

# 运行测试（参考"运行测试"章节）
```

---

## 推荐实践

### 1. 命名规范

- **测试类名**: `<模块名>ETSUnitTest`，例如 `MaterialETSUnitTest`
- **测试用例名**: `<类名>_<方法名>_<编号>`，例如 `MaterialETS_Create_001`
- **文件名**: `<模块>_ets_unit_test.cpp`，例如 `material_ets_unit_test.cpp`

### 2. 测试组织

- 每个测试类专注于一个 API 模块
- 相关的测试用例放在同一个测试类中
- 测试用例按功能分组，使用编号区分

### 3. 断言使用

```cpp
// 布尔判断
EXPECT_TRUE(condition);
EXPECT_FALSE(condition);

// 相等性判断
EXPECT_EQ(expected, actual);
ASSERT_EQ(expected, actual);  // 失败后立即停止

// 指针判断
EXPECT_NE(nullptr, pointer);
EXPECT_NULL(pointer);

// 浮点数比较
EXPECT_FLOAT_EQ(expected, actual);
EXPECT_NEAR(val1, val2, abs_error);
```

### 4. 测试隔离

- 每个测试用例应该独立运行
- 不要依赖测试用例的执行顺序
- 使用 `SetUp()` 和 `TearDown()` 确保资源正确初始化和清理

### 5. 错误处理

```cpp
HWTEST_F(SceneETSUnitTest, SceneETS_Load_InvalidPath_001, TestSize.Level1)
{
    auto sceneETS = std::make_shared<SceneETS>();
    bool result = sceneETS->Load("/invalid/path");

    // 测试错误处理
    EXPECT_FALSE(result);
}
```

### 6. 性能测试

对于需要验证性能的场景，使用 `TestSize.Level2` 或 `TestSize.Level3`：

```cpp
/**
 * @tc.name: SceneETS_Load_LargeModel_001
 * @tc.desc: Test loading large model performance
 * @tc.type: PERF
 */
HWTEST_F(SceneETSUnitTest, SceneETS_Load_LargeModel_001, TestSize.Level2)
{
    auto start = std::chrono::high_resolution_clock::now();

    auto sceneETS = std::make_shared<SceneETS>();
    sceneETS->Load("/data/large_model.glb");

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    EXPECT_LT(duration.count(), 1000); // 期望在 1 秒内完成
}
```

---

## 常见问题

### Q1: 编译时提示找不到头文件

**原因**: 头文件路径配置不正确或缺少依赖

**解决方案**:
1. 检查 `BUILD.gn` 中的 `include_dirs` 是否包含所需路径
2. 确认相关模块已正确编译并导出头文件

### Q2: 运行时提示找不到共享库

**原因**: 动态库路径未正确设置

**解决方案**:
1. 确认 `lib_engine_core.z.so` 等库文件已部署到设备
2. 检查编译时定义的路径宏是否与设备路径匹配

### Q3: 测试用例通过但引擎崩溃

**原因**: 资源未正确释放或线程同步问题

**解决方案**:
1. 检查是否所有资源都在 `TearDown()` 前正确释放
2. 确保任务队列中的任务已完成（参考 `ets_test.cpp:195`）

### Q4: 如何调试测试用例

**解决方案**:
1. 使用日志：`#include <hilog/log.h>`
   ```cpp
   OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "Test", "Debug info: %{public}s", info);
   ```
2. 在设备上使用 GDB：
   ```bash
   hdc shell /data/local/tmp/SceneETSUnitTest --gtest_filter=SceneETSUnitTest.SceneETS_Load_001
   ```

### Q5: 如何只运行特定测试

**解决方案**:
使用 `--gtest_filter` 参数：
```bash
# 运行特定测试类
hdc shell /data/local/tmp/SceneETSUnitTest --gtest_filter=SceneETSUnitTest.*

# 运行特定测试用例
hdc shell /data/local/tmp/SceneETSUnitTest --gtest_filter=SceneETSUnitTest.SceneETS_Load_001

# 运行多个测试用例
hdc shell /data/local/tmp/SceneETSUnitTest --gtest_filter=*Load_001:*Load_002
```

### Q6: 如何添加新的编译宏定义

在 `BUILD.gn` 中添加：
```gni
defines = [
    "NEW_DEFINE=1",
    "STRING_DEFINE=\"value\"",
]
```

---

## 参考资源

- [GTest 文档](https://google.github.io/googletest/)
- [OpenHarmony 测试规范](https://gitee.com/openharmony/testfwk_developer_test)
- [AGP 引擎文档](../README.md)
- [GN 构建系统](https://gn.googlesource.com/gn/)

---

## 附录：完整的 BUILD.gn 模板

```gni
# Copyright (c) 2025 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import("//build/config/features.gni")
import("//build/test.gni")
import("//build/ohos.gni")
import("//foundation/graphic/graphic_3d/lume/lume_config.gni")

module_output_path = "graphic_3d/graphic_3d/kits/ets"

ohos_unittest("SceneETSUnitTest") {
  module_out_path = module_output_path

  include_dirs = [
    ".",
    "//foundation/graphic/graphic_3d/kits/ets/include",
    "//foundation/graphic/graphic_3d/kits/ets/include/property_proxy",
    "//foundation/graphic/graphic_3d/kits/ets/include/geometry_definition",
    "${LUME_BASE_PATH}/api",
    "${LUME_CORE_PATH}/api",
    "${LUME_RENDER_PATH}/api",
    "${LUME_CORE3D_PATH}/api",
  ]

  cflags = [
    "-g",
    "-O0",
    "-Wno-unused-variable",
    "-fno-omit-frame-pointer",
  ]

  sources = [
    "common/ets_test.cpp",
    "api_unit_test/scene_ets_unit_test.cpp",
    # 在这里添加新的测试文件
  ]

  defines = [
    "CORE_HAS_GLES_BACKEND=1",
    "CORE_HAS_VULKAN_BACKEND=1",
    "LIB_ENGINE_CORE=\"lib_engine_core.z.so\"",
  ]

  configs = [
    "//foundation/graphic/graphic_3d/kits/ets:lume3d_config",
  ]

  deps = [
    "//foundation/graphic/graphic_3d/3d_widget_adapter:lib3dWidgetAdapter",
    "//foundation/graphic/graphic_3d/kits/ets:scene_ani",
  ]

  external_deps = [
    "ability_runtime:napi_base_context",
    "c_utils:utils",
    "googletest:gmock",
    "googletest:gtest",
    "hilog:libhilog",
    "napi:ace_napi",
  ]

  part_name = "graphic_3d"
  subsystem_name = "graphic"
}

group("unittest") {
  testonly = true
  deps = [
    ":SceneETSUnitTest",
  ]
}
```

---

**版本**: 1.0
**更新日期**: 2025-01-29
**维护者**: AGP 引擎团队
