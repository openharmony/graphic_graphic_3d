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

/**
 */

#include <base/containers/fixed_string.h>
#include <core/ecs/intf_ecs.h>

#include "../gfx_common.h"
#include "../gfx_hello_world_boilerplate.h"
#include "test_framework.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

namespace {
// adds the entity ids into a string
string PrintEntityFailInfo(array_view<const Entity> entities)
{
    BASE_NS::string ret {};
    for (auto e : entities) {
        ret.append(BASE_NS::to_string(e.id));
        ret.append(" ");
    }
    return ret;
}

// adds only the manager names into a string
string PrintComponentFailInfo(array_view<const Entity> entities, const IComponentManager& manager)
{
    BASE_NS::string ret {};
    for (auto e : entities) {
        if (manager.HasComponent(e)) {
            ret.append(manager.GetName());
            ret.append(" ");
        }
    }
    return ret;
}
} // namespace

/**
    Ensure there are no ECS events in simple 3D scenery when camera is not moved.
*/
UNIT_TEST(ECS, DefaultEventCycle, testing::ext::TestSize.Level1)
{
    UTest::TestResources res(360u, 260u, DeviceBackendType::VULKAN);
    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));

    auto& ecs = res.GetEcs();
    const auto& managers = ecs.GetComponentManagers();
    auto managerCount = managers.size();

    // helper for catching entity events.
    class EntityListeningChecker : public IEcs::EntityListener {
    public:
        EntityListeningChecker(IEcs& e) : ecs(e) {}
        void OnEntityEvent(EventType type, BASE_NS::array_view<const Entity> entities) override
        {
            auto managers = ecs.GetComponentManagers();
            switch (type) {
                case IEntityManager::EventType::ACTIVATED:
                    ADD_FAILURE() << "IEntityManager::EventType::ACTIVATED " << PrintEntityFailInfo(entities).c_str();
                    break;
                case IEntityManager::EventType::CREATED:
                    ADD_FAILURE() << "IEntityManager::EventType::CREATED " << PrintEntityFailInfo(entities).c_str();
                    break;
                case IEntityManager::EventType::DEACTIVATED:
                    ADD_FAILURE() << "IEntityManager::EventType::DEACTIVATED " << PrintEntityFailInfo(entities).c_str();
                    break;
                case IEntityManager::EventType::DESTROYED:
                    ADD_FAILURE() << "IEntityManager::EventType::DESTROYED " << PrintEntityFailInfo(entities).c_str();
                    break;
                default:
                    break;
            }
        }

        IEcs& ecs;
    };
    EntityListeningChecker elistener(ecs);

    // helper for catching component events.
    class ComponentListeningChecker : public IEcs::ComponentListener {
    public:
        void OnComponentEvent(EventType type, const IComponentManager& componentManager,
            BASE_NS::array_view<const Entity> entities) override
        {
            switch (type) {
                case CORE_NS::IEcs::ComponentListener::EventType::CREATED:
                    ADD_FAILURE() << "ComponentListener::EventType::CREATED "
                                  << PrintComponentFailInfo(entities, componentManager).c_str();
                    break;
                case CORE_NS::IEcs::ComponentListener::EventType::MODIFIED:
                    ADD_FAILURE() << "ComponentListener::EventType::MODIFIED "
                                  << PrintComponentFailInfo(entities, componentManager).c_str();
                    break;
                case CORE_NS::IEcs::ComponentListener::EventType::DESTROYED:
                    ADD_FAILURE() << "ComponentListener::EventType::DESTROYED "
                                  << PrintComponentFailInfo(entities, componentManager).c_str();
                    break;
                default:
                    break;
            }
        }
    };
    ComponentListeningChecker clistener;

    HelloWorldBoilerplate::HelloWorldTest(res);
    res.TickTest(3);

    ecs.AddListener(elistener);

    for (auto* manager : managers) {
        ecs.AddListener(*manager, clistener);
    }

    // Tick the test to have advances in ECS.
    res.TickTest(1);

    // remove the managers listeners, but before that, ensure that the manager count has remained the same.
    ASSERT_EQ(managerCount, managers.size());
    for (auto* manager : managers) {
        ecs.RemoveListener(*manager, clistener);
    }

    ecs.RemoveListener(elistener);

    res.ShutdownTest();
}
