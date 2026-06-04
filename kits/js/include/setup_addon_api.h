/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#ifndef SETUP_ADDON_API_H
#define SETUP_ADDON_API_H

#include <core/plugin/intf_plugin_register.h>

#include "export.h"

SCENE_ADDON_PUBLIC void InitSceneAddonApi(CORE_NS::IPluginRegister& (*getPluginRegister)(),
    void (*createPluginRegistry)(const struct CORE_NS::PlatformCreateInfo& platformCreateInfo));

#endif  // SETUP_ADDON_API_H
