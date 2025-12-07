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

#ifndef CORE3D_TEST_API_GFX_CLOUD_TEST_H
#define CORE3D_TEST_API_GFX_CLOUD_TEST_H

#include <3d/namespace.h>

#include "gfx_common.h"

namespace Cloud {
/*  Similar test case as we have the Hello world application */
void CloudTest(CORE3D_NS::UTest::TestResources& res);
void TickTest(CORE3D_NS::UTest::TestResources& res, int frameCountToTick);
} // namespace Cloud

#endif // CORE3D_TEST_API_GFX_CLOUD_TEST_H
