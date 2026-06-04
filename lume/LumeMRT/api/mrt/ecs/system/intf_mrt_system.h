/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef PLUGIN_API_MRT_SYSTEM_H
#define PLUGIN_API_MRT_SYSTEM_H

#include <core/ecs/intf_system.h>
#include <core/property/property_types.h>

#include "implementation_uids.h"

MRT_BEGIN_NAMESPACE()
class IMRTSystem : public CORE_NS::ISystem {
public:
    static constexpr BASE_NS::Uid UID{UID_MRT_SYSTEM};

protected:
    IMRTSystem() = default;
    virtual ~IMRTSystem() = default;
    IMRTSystem(const IMRTSystem&) = delete;
    IMRTSystem(const IMRTSystem&&) = delete;
    IMRTSystem& operator=(const IMRTSystem&) = delete;
};

inline constexpr const char* GetName(const IMRTSystem*)
{
    return "MRTSystem";
}
MRT_END_NAMESPACE()
#endif