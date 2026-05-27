/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef API_3D_ECS_SYSTEMS_ILIGHT_PROBE_BAKER_SYSTEM_H
#define API_3D_ECS_SYSTEMS_ILIGHT_PROBE_BAKER_SYSTEM_H

#include <3d/namespace.h>
#include <base/containers/array_view.h>
#include <base/util/uid.h>
#include <core/ecs/intf_system.h>
#include <core/plugin/intf_interface.h>
#include <render/namespace.h>

CORE_BEGIN_NAMESPACE()
class IEcs;
struct Entity;
CORE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
class RenderHandleReference;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class ILightProbeBakerSystem : public CORE_NS::ISystem {
public:
    static constexpr BASE_NS::Uid UID{"69D065EF-F3B0-4307-977D-AF5C79212A7C"};

    enum class LightProbeBakeStatus {
        IDLE = 0,
        BAKE_IN_PROGRESS,
    };

    struct State {
        LightProbeBakeStatus status;
        // Between [0, 1]
        float progress{0.0f};
    };

protected:
    ILightProbeBakerSystem() = default;
    virtual ~ILightProbeBakerSystem() = default;
    ILightProbeBakerSystem(const ILightProbeBakerSystem&) = delete;
    ILightProbeBakerSystem(const ILightProbeBakerSystem&&) = delete;
    ILightProbeBakerSystem& operator=(const ILightProbeBakerSystem&) = delete;
};

inline constexpr const char* GetName(const ILightProbeBakerSystem*)
{
    return "LightProbeBakerSystem";
}

class ILightProbeBaker : public CORE_NS::IInterface {
public:
    static constexpr auto UID = BASE_NS::Uid("6836EB08-6457-4A3E-906D-2307D8957A46");

    using Ptr = BASE_NS::refcnt_ptr<ILightProbeBaker>;

    virtual ~ILightProbeBaker() = default;

    virtual void Initialize(CORE_NS::IEcs& ecs) = 0;
    virtual bool StartBake(BASE_NS::array_view<CORE_NS::Entity> lightProbeGroups) = 0;
    virtual bool CancelBake() = 0;
    virtual ILightProbeBakerSystem::State GetStatus() const = 0;
    virtual RENDER_NS::RenderHandleReference GetRenderNodeGraph() const = 0;
};

inline constexpr BASE_NS::string_view GetName(const ILightProbeBaker*)
{
    return "ILightProbeBaker";
}
CORE3D_END_NAMESPACE()

#endif  // API_3D_ECS_SYSTEMS_ILIGHT_PROBE_BAKER_SYSTEM_H
