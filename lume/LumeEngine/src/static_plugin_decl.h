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

#ifndef API_CORE_PLUGIN_INTF_STATIC_PLUGIN_DECL_H
#define API_CORE_PLUGIN_INTF_STATIC_PLUGIN_DECL_H

#include <core/namespace.h>
#include <core/plugin/intf_plugin.h>

// clang-format off
#if !defined(CORE_PLUGIN) || (CORE_PLUGIN == 0)
// Static plugin
#if defined(CORE_USE_COMPILER_GENERATED_STATIC_LIST) && (CORE_USE_COMPILER_GENERATED_STATIC_LIST == 1)
// linker/compiler generated initialized list..
#if _MSC_VER
#define PLUGIN_DATA(NAME) static constexpr const CORE_NS::IPlugin NAME##_pluginData
#define DEFINE_STATIC_PLUGIN(NAME)                                                              \
    __pragma(section("spd$d", long, read))                                                      \
    __declspec(allocate("spd$d"))                                                               \
    extern constexpr CORE_NS::IPlugin const* const NAME##_StaticPlugin = &NAME##_pluginData;
#else
#define PLUGIN_DATA(NAME) __attribute__((used)) constexpr const CORE_NS::IPlugin NAME##_pluginData
#if __aarch64__
#define SECTION(NAME) #NAME",\"wa\"\n .align 3\n"
#elif __x86_64__
#define SECTION(NAME) #NAME",\"wa\"\n .align 8\n"
#elif __arm__
#define SECTION(NAME) #NAME",\"wa\"\n .align 2\n"
#elif __i386__
#define SECTION(NAME) #NAME",\"wa\"\n .align 4\n"
#endif
#define DEFINE_STATIC_PLUGIN(NAME)                                                                                    \
    __asm__(                                                                                                          \
        " .pushsection " SECTION(spl.1)                                                                               \
        " .weak static_plugin_list\n"                                                                                 \
        " static_plugin_list:\n"                                                                                      \
        ".section " SECTION(spl.2)                                                                                    \
        " .global "#NAME"_StaticPlugin\n"                                                                             \
        ""#NAME"_StaticPlugin:\n"                                                                                     \
        " .dc.a "#NAME"_pluginData\n"                                                                                 \
        ".section " SECTION(spl.3)                                                                                    \
        " .weak static_plugin_list_end\n"                                                                             \
        " static_plugin_list_end:\n"                                                                                  \
        " .popsection\n");                                                                                            \
    extern const CORE_NS::IPlugin* const NAME##_StaticPlugin;                                                         \
    __attribute__((visibility("hidden"), used)) CORE_NS::IPlugin const* const NAME##_DATA_ref = NAME##_StaticPlugin;  \
    extern CORE_NS::IPlugin const* const static_plugin_list;                                                          \
    extern __attribute__((visibility("hidden"), used, weak)) CORE_NS::IPlugin const* const static_plugin_list_ref =   \
        static_plugin_list;                                                                                           \
    extern CORE_NS::IPlugin const* const static_plugin_list_end;                                                      \
    extern __attribute__((visibility("hidden"), used, weak))                                                          \
        CORE_NS::IPlugin const* const static_plugin_list_end_ref = static_plugin_list_end;
#endif
#else

namespace CORE_NS {
namespace StaticPluginRegistry {
void RegisterStaticPlugin(const CORE_NS::IPlugin& plugin);
} // namespace StaticPluginRegistry
} // namespace CORE_NS

#define PLUGIN_DATA(NAME) static constexpr const CORE_NS::IPlugin NAME##_pluginData

#if _MSC_VER
    // Use static class "constructor" to call a function during initialization.
    // ("safer", should work with ANY compiler, but with dynamic runtime init/memory cost)
#define DEFINE_STATIC_PLUGIN(NAME)                                                   \
    static struct magic {                                                            \
        magic()                                                                      \
        {                                                                            \
            CORE_NS::StaticPluginRegistry::RegisterStaticPlugin(NAME##_pluginData); \
        }                                                                            \
    } magic;
#else
    // Use "constructor" attribute to call a function during initialization.
    // ("safer", but with dynamic runtime init/memory cost)
#define DEFINE_STATIC_PLUGIN(NAME)                                                                  \
    __attribute__((visibility("hidden"))) __attribute__((constructor)) static void registerStatic() \
    {                                                                                               \
        CORE_NS::StaticPluginRegistry::RegisterStaticPlugin(NAME##_pluginData);                    \
    }
#endif
#endif
#endif
// clang-format on
#endif // API_CORE_PLUGIN_INTF_STATIC_PLUGIN_DECL_H
