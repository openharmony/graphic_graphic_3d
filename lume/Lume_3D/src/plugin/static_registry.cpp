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

CORE_BEGIN_NAMESPACE()
class IPluginRegister;
CORE_END_NAMESPACE()

namespace {
CORE_NS::IPluginRegister* gPluginRegistry { nullptr };
} // namespace

CORE3D_BEGIN_NAMESPACE()
CORE_NS::IPluginRegister& GetPluginRegister()
{
    return *gPluginRegistry;
}
CORE3D_END_NAMESPACE()

// Bridge for engine headers that reference CORE_NS::GetPluginRegister().
CORE_BEGIN_NAMESPACE()
IPluginRegister& GetPluginRegister()
{
    return CORE3D_NS::GetPluginRegister();
}
CORE_END_NAMESPACE()

extern "C" void InitRegistry(CORE_NS::IPluginRegister& pluginRegistry)
{
    // Initializing dynamic plugin.
    // Pluginregistry access via the provided registry instance which is saved here.
    gPluginRegistry = &pluginRegistry;
}
