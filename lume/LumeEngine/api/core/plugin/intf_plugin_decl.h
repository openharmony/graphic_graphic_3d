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

#ifndef API_CORE_PLUGIN_INTF_PLUGIN_DECL_H
#define API_CORE_PLUGIN_INTF_PLUGIN_DECL_H

#include <core/namespace.h>
#include <core/plugin/intf_plugin.h>

// clang-format off
#if defined(CORE_PLUGIN) && (CORE_PLUGIN == 1)
// Dynamic plugin
#if _MSC_VER
#define PLUGIN_DATA(NAME) __declspec(dllexport) CORE_NS::IPlugin gPluginData
#define DEFINE_STATIC_PLUGIN(NAME)
#else
#define PLUGIN_DATA(NAME) __attribute__((visibility("default"))) CORE_NS::IPlugin gPluginData
#define DEFINE_STATIC_PLUGIN(NAME)
#endif
#endif
// clang-format on
#endif // API_CORE_PLUGIN_INTF_PLUGIN_DECL_H
