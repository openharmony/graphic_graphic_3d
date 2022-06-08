/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class IPluginRegister;
CORE_END_NAMESPACE()

extern "C" void InitRegistry(CORE_NS::IPluginRegister&)
{
    // Initializing static plugin. (registry is available directly, nothing to do here.)
}
