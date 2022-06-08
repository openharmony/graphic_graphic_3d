/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class IPluginRegister;
CORE_END_NAMESPACE()

namespace {
static CORE_NS::IPluginRegister* gPluginRegistry { nullptr };
} // namespace

CORE_BEGIN_NAMESPACE()
class IPluginRegister;
IPluginRegister& GetPluginRegister()
{
    return *gPluginRegistry;
}
CORE_END_NAMESPACE()

extern "C" void InitRegistry(CORE_NS::IPluginRegister& pluginRegistry)
{
    // Initializing dynamic plugin.
    // Pluginregistry access via the provided registry instance which is saved here.
    gPluginRegistry = &pluginRegistry;
}
