/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * Boilerplate code for using the hello world easily in multiple places.
 */

#ifndef CORE3D_TEST_API_GFX_HELLO_WORLD_BOILERPLATE_H
#define CORE3D_TEST_API_GFX_HELLO_WORLD_BOILERPLATE_H

#include <3d/namespace.h>
#include <base/containers/string_view.h>
#include <core/perf/intf_performance_data_manager.h>

#include "gfx_common.h"

namespace HelloWorldBoilerplate {
/*  Similar test case as we have the Hello world application
    but in this case we do not rotate the cubes but render
    only n number of frames for validation.
    In addition has material render sorting.
*/
void HelloWorldTest(CORE3D_NS::UTest::TestResources& res);

void GetPerformanceDataCounters(BASE_NS::string_view name, CORE_NS::IPerformanceDataManager::CounterPairView data);
CORE_NS::IPerformanceDataManager::ComparisonData ComparePerformanceDataCounters(BASE_NS::string_view name,
    CORE_NS::IPerformanceDataManager::ConstCounterPairView lhs,
    CORE_NS::IPerformanceDataManager::ConstCounterPairView rhs);
} // namespace HelloWorldBoilerplate

#endif // CORE3D_TEST_API_GFX_HELLO_WORLD_BOILERPLATE_H
