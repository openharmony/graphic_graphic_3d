/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <3d/namespace.h>
#include <core/namespace.h>

#include "util/log.h"

#if (CORE_PERF_ENABLED == 1)
// Workaround for performance trace macros from LumeEngine.
CORE_BEGIN_NAMESPACE() class IPluginRegister;
IPluginRegister& GetPluginRegister()
{
    return CORE3D_NS::GetPluginRegister();
}
CORE_END_NAMESPACE()
#endif
