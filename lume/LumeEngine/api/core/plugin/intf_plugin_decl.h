/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
