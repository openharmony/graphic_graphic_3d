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

#ifndef CORE_OS_PLATFORM_H
#define CORE_OS_PLATFORM_H

#include <base/containers/unique_ptr.h>
#include <core/namespace.h>
#include <core/os/intf_platform.h>

CORE_BEGIN_NAMESPACE()
struct PlatformCreateInfo;
class Platform {
public:
    static CORE_NS::IPlatform::Ptr Create(CORE_NS::PlatformCreateInfo const& createInfo);
};
CORE_END_NAMESPACE()

#endif // CORE_OS_PLATFORM_H
