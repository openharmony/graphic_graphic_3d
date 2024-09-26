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

#ifndef PLUGIN_API_DOTFIELD_SYSTEM_H
#define PLUGIN_API_DOTFIELD_SYSTEM_H

#include <core/ecs/intf_system.h>
#include <core/property/property_types.h>

namespace Dotfield {
class IDotfieldSystem : public CORE_NS::ISystem {
public:
    static constexpr BASE_NS::Uid UID{ "3288bb8a-4d6f-479c-b1b5-aeca5b26522d" };

protected:
    IDotfieldSystem() = default;
    virtual ~IDotfieldSystem() = default;
    IDotfieldSystem(const IDotfieldSystem&) = delete;
    IDotfieldSystem(const IDotfieldSystem&&) = delete;
    IDotfieldSystem& operator=(const IDotfieldSystem&) = delete;
};

inline constexpr const char* GetName(const IDotfieldSystem*)
{
    return "DotfieldSystem";
}
} // namespace Dotfield
#endif