/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_OS_PLATFORM_H
#define CORE_OS_PLATFORM_H

#include <base/containers/unique_ptr.h>
#include <core/os/intf_platform.h>
#include <core/os/platform_create_info.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()

class Platform
{
public:
    static CORE_NS::IPlatform::Ptr Create(CORE_NS::PlatformCreateInfo const& createInfo);
};

CORE_END_NAMESPACE()

#endif // CORE_OS_PLATFORM_H
