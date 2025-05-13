/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef API_3D_ECS_SYSTEMS_IWEATHER_SYSTEM_H
#define API_3D_ECS_SYSTEMS_IWEATHER_SYSTEM_H

#include <3d/namespace.h>
#include <base/util/uid.h>
#include <core/ecs/intf_system.h>

CORE3D_BEGIN_NAMESPACE()
class IWeatherSystem : public CORE_NS::ISystem {
public:
    static constexpr BASE_NS::Uid UID { "12345678-4ebc-2d3d-b93d-3cef38fc5045" };

protected:
    IWeatherSystem() = default;
    virtual ~IWeatherSystem() = default;
    IWeatherSystem(const IWeatherSystem&) = delete;
    IWeatherSystem(const IWeatherSystem&&) = delete;
    IWeatherSystem& operator=(const IWeatherSystem&) = delete;
};

inline constexpr const char* GetName(const IWeatherSystem*)
{
    return "WeatherSystem";
}
CORE3D_END_NAMESPACE()

#endif // API_3D_ECS_SYSTEMS_IWEATHER_SYSTEM_H
