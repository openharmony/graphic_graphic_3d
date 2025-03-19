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

#include <ComponentTools/base_manager.h>
#include <ComponentTools/base_manager.inl>
#include <ComponentTools/component_query.h>
#include <algorithm>
#include <chrono>
#include <thread>

#include <gtest/gtest.h>

#include <base/util/uid.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_component_manager.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_system.h>
#include <core/ecs/intf_system_graph_loader.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/property_types.h>
#include <core/property_tools/property_api_impl.h>
#include <core/property_tools/property_api_impl.inl>

// Definition of the component manager.

// Declare a component and managerusing Component Helper macros. This should be in a header.
BEGIN_COMPONENT(ITestComponentManager, TestComponent)
DEFINE_PROPERTY(uint32_t, value, "Value", 0, VALUE(0))
END_COMPONENT(ITestComponentManager, TestComponent, "b8d0c322-228e-4a77-94f6-ac530e7f4e3c")

BEGIN_COMPONENT(ITestComponent2Manager, TestComponent2)
DEFINE_PROPERTY(float, value, "Value", 0, VALUE(0))
DEFINE_PROPERTY(CORE_NS::EntityReference, ref, "Reference", 0,)
END_COMPONENT(ITestComponent2Manager, TestComponent2, "b8d0c322-228e-4a77-94f6-ac530e7f4e3d")

// Definition of the component manager.

#define IMPLEMENT_MANAGER
#include <core/property_tools/property_macros.h>

#include "TestRunner.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace testing::ext;

namespace {
struct FuzzContext {
    std::shared_ptr<ISceneInit> sceneInit_ = nullptr;
    CORE_NS::IEcs::Ptr ecs_;
};
static FuzzContext g_context;

using IntfPtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using IntfWeakPtr = BASE_NS::weak_ptr<CORE_NS::IInterface>;
static constexpr BASE_NS::Uid ENGINE_THREAD{"2070e705-d061-40e4-bfb7-90fad2c280af"};
static constexpr BASE_NS::Uid APP_THREAD{"b2e8cef3-453a-4651-b564-5190f8b5190d"};
static constexpr BASE_NS::Uid IO_QUEUE{"be88e9a0-9cd8-45ab-be48-937953dc258f"};
static constexpr BASE_NS::Uid JS_RELEASE_THREAD{"3784fa96-b25b-4e9c-bbf1-e897d36f73af"};

bool SceneDispose(FuzzContext &context)
{
    context.sceneInit_ = nullptr;
    return true;
}

bool SceneCreate(FuzzContext &context)
{
    context.sceneInit_ = CreateTestScene();
    context.sceneInit_->LoadPluginsAndInit();
    if (!context.sceneInit_->GetEngineInstance().engine_) {
        WIDGET_LOGE("fail to get engine");
        return false;
    }

    using namespace SCENE_NS;
#if SCENE_META_FUZZ
    auto fun = [&context]() {
        auto &obr = META_NS::GetObjectRegistry();

        context.params_ = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
        if (!context.params_) {
            CORE_LOG_E("default obj null");
        }
        context.scene_ =
            interface_pointer_cast<SCENE_NS::IScene>(obr.Create(SCENE_NS::ClassId::Scene, context.params_));
        
        auto onLoaded = META_NS::MakeCallback<META_NS::IOnChanged>([&context]() {
            bool complete = false;
            auto status = context.scene_->Status()->GetValue();
            if (status == SCENE_NS::IScene::SCENE_STATUS_READY) {
                // still in engine thread
                complete = true;
            } else if (status == SCENE_NS::IScene::SCENE_STATUS_LOADING_FAILED) {
                // make sure we don't have anything in result if error
                complete = true;
            }

            if (complete) {
                if (context.scene_) {
                    auto &obr = META_NS::GetObjectRegistry();
                    // make sure we have renderconfig
                    auto rc = context.scene_->RenderConfiguration()->GetValue();
                    if (!rc) {
                        // Create renderconfig
                        rc = obr.Create<SCENE_NS::IRenderConfiguration>(SCENE_NS::ClassId::RenderConfiguration);
                        context.scene_->RenderConfiguration()->SetValue(rc);
                    }

                    interface_cast<IEcsScene>(context.scene_)
                        ->RenderMode()
                        ->SetValue(IEcsScene::RenderMode::RENDER_ALWAYS);
                    auto duh = context.params_->GetArrayPropertyByName<IntfWeakPtr>("Scenes");
                    if (!duh) {
                        return ;
                    }
                    duh->AddValue(interface_pointer_cast<CORE_NS::IInterface>(context.scene_));
                }
            }
        });
        context.scene_->Asynchronous()->SetValue(false);
        context.scene_->Uri()->SetValue("scene://empty");
        return META_NS::IAny::Ptr{};
    };
    // Should it be possible to cancel? (ie. do we need to store the token for something ..)
    META_NS::GetTaskQueueRegistry()
        .GetTaskQueue(ENGINE_THREAD)
        ->AddWaitableTask(META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>(BASE_NS::move(fun)))
        ->Wait();
#endif
    return true;
}
} // namespace

namespace {
class TestComponentManager final : public BaseManager<TestComponent, ITestComponentManager> {
public:
    explicit TestComponentManager(IEcs& ecs)
        : BaseManager<TestComponent, ITestComponentManager>(ecs, CORE_NS::GetName<TestComponent>())
    {}

    ~TestComponentManager() = default;

    size_t PropertyCount() const override
    {
        return ComponentMetaData.size();
    }

    const Property* MetaData(size_t index) const override
    {
        if (index < ComponentMetaData.size()) {
            return &ComponentMetaData[index];
        }
        return nullptr;
    }

    array_view<const Property> MetaData() const override
    {
        return ComponentMetaData;
    }

private:
    BEGIN_PROPERTY(TestComponent, ComponentMetadata)
    DEFINE_PROPERTY(uint32_t, value, "Value", 0,)
    END_PROPERTY();
    const array_view<const Property> ComponentMetaData { ComponentMetadata, countof(ComponentMetadata) };
};

class TestComponent2Manager final : public BaseManager<TestComponent2, ITestComponent2Manager> {
public:
    explicit TestComponent2Manager(IEcs& ecs)
        : BaseManager<TestComponent2, ITestComponent2Manager>(ecs, CORE_NS::GetName<TestComponent2>())
    {}

    ~TestComponent2Manager() = default;

    size_t PropertyCount() const override
    {
        return ComponentMetaData.size();
    }

    const Property* MetaData(size_t index) const override
    {
        if (index < ComponentMetaData.size()) {
            return &ComponentMetaData[index];
        }
        return nullptr;
    }

    array_view<const Property> MetaData() const override
    {
        return ComponentMetaData;
    }

private:
    BEGIN_PROPERTY(TestComponent2, ComponentMetadata)
    DEFINE_PROPERTY(float, value, "Value", 0,)
    END_PROPERTY();
    const array_view<const Property> ComponentMetaData { ComponentMetadata, countof(ComponentMetadata) };
};

constexpr ComponentManagerTypeInfo testComponentInfo {
    ComponentManagerTypeInfo::UID,
    ITestComponentManager::UID,
    CORE_NS::GetName<ITestComponentManager>().data(),
    [](IEcs& ecs) -> IComponentManager* { return new TestComponentManager(ecs); },
    [](IComponentManager* instance) { delete static_cast<TestComponentManager*>(instance); },
};

constexpr ComponentManagerTypeInfo testComponent2Info {
    ComponentManagerTypeInfo::UID,
    ITestComponent2Manager::UID,
    CORE_NS::GetName<ITestComponent2Manager>().data(),
    [](IEcs& ecs) -> IComponentManager* { return new TestComponent2Manager(ecs); },
    [](IComponentManager* instance) { delete static_cast<TestComponent2Manager*>(instance); },
};

constexpr ComponentManagerTypeInfo duplicatedTestComponentInfo {
    ComponentManagerTypeInfo::UID,
    ComponentManagerTypeInfo::UID,
    CORE_NS::GetName<ITestComponentManager>().data(),
    [](IEcs& ecs) -> IComponentManager* { return new TestComponentManager(ecs); },
    [](IComponentManager* instance) { delete static_cast<TestComponentManager*>(instance); },
};

// Definition of a system
class TestSystem final : public ISystem {
public:
    static constexpr Uid UID { "03700b48-4ff3-44ff-8bbf-3b0effe7e25d" };

    explicit TestSystem(IEcs& ecs) : ecs_(ecs), testManager_(GetManager<ITestComponentManager>(ecs_)) {}

    string_view GetName() const override
    {
        return "TestSystem";
    }

    Uid GetUid() const override
    {
        return UID;
    }

    IPropertyHandle* GetProperties() override
    {
        return properties_.GetData();
    }

    const IPropertyHandle* GetProperties() const override
    {
        return properties_.GetData();
    }

    void SetProperties(const IPropertyHandle& dataHandle) override
    {
        if (dataHandle.Owner() != &properties_) {
            return;
        }
        if (auto data = ScopedHandle<const PropertyData>(&dataHandle); data) {
            props_ = *data;
        }
    }

    bool IsActive() const override
    {
        return active_;
    }

    void SetActive(bool isActive) override
    {
        active_ = isActive;
    }

    void Initialize() override
    {
        SetActive(true);
    }

    bool Update(bool isFrameRenderingQueued, uint64_t time, uint64_t delta) override
    {
        ++updateCalls_;
        delta_ = delta;
        if (!active_) {
            return false;
        }
        if (testManager_) {
            const auto count = testManager_->GetComponentCount();
            for (IComponentManager::ComponentId id = 0; id < count; ++id) {
                const auto value = testManager_->Read(id)->value;
                testManager_->Write(id)->value = value + 1;
            }
            return count > 0;
        }

        return false;
    }

    void Uninitialize() override
    {
        SetActive(false);
    }

    const IEcs& GetECS() const override
    {
        return ecs_;
    }

    size_t UpdateCalls() const
    {
        return updateCalls_;
    }

    uint64_t Delta() const
    {
        return delta_;
    }

private:
    struct PropertyData {
        int id;
        char name[150];
    };
    IEcs& ecs_;
    PropertyData props_ { 1,
        { 't', 'e', 's', 't', 's', 't', 'e', 's', 't', 's', 't', 'e', 's', 't', 's', 't', 'e', 's', 't', 's', 't', 'e',
            's', 't', 's', 't', 'e', 's', 't', 's', 't', 'e', 's', 't', 's', 't', 'e', 's', 't', 's', 't', 'e', 's',
            't', 's', 't', 'e', 's', 't', 's', 't', 'e', 's', 't', 's', 't', 'e', 's', 't', 's', 't', 'e', 's', 't',
            's', 't', 'e', 's', 't', 's', 't', 'e', 's', 't', 's', 't', 'e', 's', 't', 's', 't', 'e', 's', 't', 's',
            't', 'e', 's', 't', 's', 't', 'e', 's', 't', 's', 't', 'e', 's', 't', 's', 't', 'e', 's', 't', 's', 't',
            'e', 's', 't', 's', 't', 'e', 's', 't', 's', 't', 'e', 's', 't', 's', 't', 'e', 's', 't', 's', 't', 'e',
            's', 't', 's', 't', 'e', 's', 't', 's', 't', 'e', 's', 't', 's', 't', 'e', 's', 't', 's', 't', 'e', 's',
            't', 's' } };
    MetaData tmp;

    Property propsArray[21] = { Property { BASE_NS::string_view { "prop 1" }, 1, PropertyType::BOOL_T, 1, 1, 1,
                                           BASE_NS::string_view { "property 1" }, 1, tmp },
        Property { BASE_NS::string_view { "prop 2" }, 2, PropertyType::CHAR_T, 1, 4, 5,
            BASE_NS::string_view { "property 2" }, 1, tmp },
        Property { BASE_NS::string_view { "prop 3" }, 3, PropertyType::INT8_T, 1, 1, 6,
            BASE_NS::string_view { "property 3" }, 1, tmp },
        Property { BASE_NS::string_view { "prop 4" }, 4, PropertyType::INT16_T, 1, 2, 8,
            BASE_NS::string_view { "property 4" }, 1, tmp },
        Property { BASE_NS::string_view { "prop 5" }, 5, PropertyType::INT32_T, 1, 4, 12,
            BASE_NS::string_view { "property 5" }, 1, tmp },
        Property { BASE_NS::string_view { "prop 6" }, 6, PropertyType::INT64_T, 1, 8, 20,
            BASE_NS::string_view { "property 6" }, 1, tmp },
        Property { BASE_NS::string_view { "prop 7" }, 7, PropertyType::UINT8_T, 1, 1, 21,
            BASE_NS::string_view { "property 7" }, 1, tmp },
        Property { BASE_NS::string_view { "prop 8" }, 8, PropertyType::UINT16_T, 1, 2, 23,
            BASE_NS::string_view { "property 8" }, 1, tmp },
        Property { BASE_NS::string_view { "prop 9" }, 9, PropertyType::UINT32_T, 1, 4, 27,
            BASE_NS::string_view { "property 9" }, 1, tmp },
        Property { BASE_NS::string_view { "prop 10" }, 10, PropertyType::UINT64_T, 1, 8, 35,
            BASE_NS::string_view { "property 10" }, 1, tmp },
        Property { BASE_NS::string_view { "prop 11" }, 11, PropertyType::DOUBLE_T, 1, 8, 43,
            BASE_NS::string_view { "property 11" }, 1, tmp },
        Property { BASE_NS::string_view { "prop 12" }, 12, PropertyType::ENTITY_T, 1, 8, 51,
            BASE_NS::string_view { "property 12" }, 1, tmp },
        Property { BASE_NS::string_view { "prop 13" }, 13, PropertyType::VEC2_T, 1, 8, 59,
            BASE_NS::string_view { "property 13" }, 1, tmp },
        Property { BASE_NS::string_view { "prop 14" }, 14, PropertyType::VEC3_T, 1, 12, 71,
            BASE_NS::string_view { "property 14" }, 1, tmp },
        Property { BASE_NS::string_view { "prop 15" }, 15, PropertyType::VEC4_T, 1, 16, 87,
            BASE_NS::string_view { "property 15" }, 1, tmp },
        Property { BASE_NS::string_view { "prop 16" }, 16, PropertyType::QUAT_T, 1, 16, 103,
            BASE_NS::string_view { "property 16" }, 1, tmp },
        Property { BASE_NS::string_view { "prop 17" }, 17, PropertyType::FLOAT_T, 1, 8, 111,
            BASE_NS::string_view { "property 17" }, 1, tmp },
        Property { BASE_NS::string_view { "prop 18" }, 18, PropertyType::STRING_T, 1, 4, 115,
            BASE_NS::string_view { "property 18" }, 1, tmp },
        Property { BASE_NS::string_view { "prop 19" }, 1, PropertyType::BOOL_ARRAY_T, 2, 2, 117,
            BASE_NS::string_view { "property 19" }, 1, tmp },
        Property { BASE_NS::string_view { "prop 20" }, 1, PropertyType::CHAR_ARRAY_T, 2, 8, 125,
            BASE_NS::string_view { "property 20" }, 1, tmp },
        Property { BASE_NS::string_view { "prop 21" }, 1, PropertyType::CHAR_T, 2, 8, 127,
            BASE_NS::string_view { "property 21" }, 1, tmp } };
    PropertyApiImpl<PropertyData> properties_ { &props_, propsArray };
    ITestComponentManager* testManager_;
    bool active_ { false };
    size_t updateCalls_ { 0U };
    uint64_t delta_ { 0U };
};

inline constexpr const string_view GetName(const TestSystem*)
{
    return "TestSystem";
}

constexpr Uid testSystemDependencies[] = { ITestComponentManager::UID };

constexpr SystemTypeInfo testSystemInfo {
    SystemTypeInfo::UID,
    TestSystem::UID,
    CORE_NS::GetName<TestSystem>().data(),
    [](IEcs& ecs) -> ISystem* { return new TestSystem(ecs); },
    [](ISystem* instance) { delete static_cast<TestSystem*>(instance); },
    testSystemDependencies,
};

constexpr SystemTypeInfo duplicateTestSystemInfo {
    SystemTypeInfo::UID,
    SystemTypeInfo::UID,
    CORE_NS::GetName<TestSystem>().data(),
    [](IEcs& ecs) -> ISystem* { return new TestSystem(ecs); },
    [](ISystem* instance) { delete static_cast<TestSystem*>(instance); },
    testSystemDependencies,
};

PluginToken CreateTestEcsPlugin(IEcs& ecs)
{
    return {};
}

void DestroyTestEcsPlugin3D(PluginToken token) {}

constexpr IEcsPlugin EcsInfo { CreateTestEcsPlugin, DestroyTestEcsPlugin3D };

class EntityListener final : IEcs::EntityListener {
public:
    explicit EntityListener(IEcs& ecs) : ecs_ { ecs }
    {
        ecs.AddListener(*this);
    }

    ~EntityListener()
    {
        ecs_.RemoveListener(*this);
    }

    void OnEntityEvent(EventType type, const array_view<const Entity> entities)
    {
        events_.push_back(Event { type, BASE_NS::vector<Entity> { entities.begin(), entities.end() } });
    }

    IEcs& ecs_;
    struct Event {
        EventType type;
        BASE_NS::vector<Entity> entities;
    };
    BASE_NS::vector<Event> events_;
};

bool operator==(const EntityListener::Event& lhs, const EntityListener::Event& rhs)
{
    return lhs.type == rhs.type && lhs.entities.size() == rhs.entities.size() &&
        std::all_of(std::begin(lhs.entities), std::end(lhs.entities), [&re = rhs.entities](const Entity& le) {
            return std::any_of(std::begin(re), std::end(re), [&le](const Entity& re) { return le == re; });
        });
}

class ComponentListener final : IEcs::ComponentListener {
public:
    ComponentListener(IEcs& ecs, IComponentManager* manager) : ecs_ { ecs }, manager_ { manager }
    {
        if (manager) {
            ecs.AddListener(*manager, *this);
        } else {
            ecs.AddListener(*this);
        }
    }

    ~ComponentListener()
    {
        if (manager_) {
            ecs_.RemoveListener(*manager_, *this);
        } else {
            ecs_.RemoveListener(*this);
        }
    }

    void OnComponentEvent(
        EventType type, const IComponentManager& componentManager, const array_view<const Entity> entities)
    {
        events_.push_back(
            Event { type, componentManager, BASE_NS::vector<Entity> { entities.begin(), entities.end() } });
    }

    IEcs& ecs_;
    IComponentManager* manager_;
    struct Event {
        EventType type;
        const IComponentManager& manager;
        BASE_NS::vector<Entity> entities;
    };
    BASE_NS::vector<Event> events_;
};

bool operator==(const ComponentListener::Event& lhs, const ComponentListener::Event& rhs)
{
    return lhs.type == rhs.type && &lhs.manager == &rhs.manager && lhs.entities.size() == rhs.entities.size() &&
    std::all_of(std::begin(lhs.entities), std::end(lhs.entities), [&re = rhs.entities](const Entity& le) {
        return std::any_of(std::begin(re), std::end(re), [&le](const Entity& re) { return le == re; });
    });
}

constexpr const string_view systemGraph = "rofs3D://systemGraph.json";
} // namespace


class EcsTest : public testing::Test {
public:
    static void SetUpTestSuite()
    {
        SceneCreate(g_context);
    }
    static void TearDownTestSuite()
    {
        SceneDispose(g_context);
    }
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(EcsTest, create, TestSize.Level1)
{
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    IInterface* instance = static_cast<ITaskQueueFactory*>(
        CORE_NS::GetPluginRegister().GetClassRegister().GetInstance(UID_TASK_QUEUE_FACTORY));
    if (instance) {
        const auto& consIns = *instance;
        EXPECT_TRUE(consIns.GetInterface(ITaskQueueFactory::UID) != nullptr);
        EXPECT_FALSE(consIns.GetInterface(Uid { "00000000-0000-0000-0000-000000000000" }) != nullptr);
        auto& norIns = *instance;
        EXPECT_FALSE(norIns.GetInterface(Uid { "00000000-0000-0000-0000-000000000000" }) != nullptr);
        norIns.Ref();
        norIns.Unref();
    }
    const auto factory = GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY);
    {
        IEcs::Ptr ecs = engine->CreateEcs();
        ASSERT_TRUE(ecs);
        ASSERT_TRUE(ecs->GetThreadPool());
        EXPECT_EQ(ecs->GetThreadPool()->GetNumberOfThreads(), factory->GetNumberOfCores() / 2);
    }
    {
        auto threadPool = factory->CreateThreadPool(4);
        IEcs::Ptr ecs = engine->CreateEcs(*threadPool);
        ASSERT_TRUE(ecs);
        ASSERT_TRUE(ecs->GetThreadPool());
        EXPECT_EQ(ecs->GetThreadPool(), threadPool);
    }
}

HWTEST_F(EcsTest, createAndDestroyEntity, TestSize.Level1)
{
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    IEcs::Ptr ecs = engine->CreateEcs();
    auto entityListener = EntityListener { *ecs };
    ecs->Initialize();
    auto& entityManager = ecs->GetEntityManager();

    // expect empty at the moment
    EXPECT_EQ(begin(entityManager), end(entityManager));
    {
        // after creating an entity, iterators are not equal
        auto entity = entityManager.Create();
        EXPECT_TRUE(entityManager.IsAlive(entity));
        EXPECT_FALSE(begin(entityManager) == end(entityManager));

        ecs->ProcessEvents();

        ASSERT_EQ(entityListener.events_.size(), 1U);
        EXPECT_EQ(entityListener.events_[0U].type, IEcs::EntityListener::EventType::CREATED);
        ASSERT_EQ(entityListener.events_[0U].entities.size(), 1U);
        EXPECT_EQ(entityListener.events_[0U].entities[0U], entity);
        entityListener.events_.clear();

        // after destroy, should be empty again
        entityManager.Destroy(entity);
        EXPECT_FALSE(entityManager.IsAlive(entity));
        EXPECT_EQ(begin(entityManager), end(entityManager));

        ecs->ProcessEvents();

        ASSERT_EQ(entityListener.events_.size(), 1U);
        EXPECT_EQ(entityListener.events_[0U].type, IEcs::EntityListener::EventType::DESTROYED);
        ASSERT_EQ(entityListener.events_[0U].entities.size(), 1U);
        EXPECT_EQ(entityListener.events_[0U].entities[0U], entity);
        entityListener.events_.clear();
    }

    {
        // after creating an entity, iterators are not equal
        auto entity = entityManager.Create();
        EXPECT_TRUE(entityManager.IsAlive(entity));

        ecs->ProcessEvents();

        ASSERT_EQ(entityListener.events_.size(), 1U);
        EXPECT_EQ(entityListener.events_[0U].type, IEcs::EntityListener::EventType::CREATED);
        ASSERT_EQ(entityListener.events_[0U].entities.size(), 1U);
        EXPECT_EQ(entityListener.events_[0U].entities[0U], entity);
        entityListener.events_.clear();

        // inactive entity is not alive and not reachable by the
        entityManager.SetActive(entity, false);
        EXPECT_FALSE(entityManager.IsAlive(entity));
        EXPECT_EQ(begin(entityManager), end(entityManager));

        ecs->ProcessEvents();

        ASSERT_EQ(entityListener.events_.size(), 1U);
        EXPECT_EQ(entityListener.events_[0U].type, IEcs::EntityListener::EventType::DEACTIVATED);
        ASSERT_EQ(entityListener.events_[0U].entities.size(), 1U);
        EXPECT_EQ(entityListener.events_[0U].entities[0U], entity);
        entityListener.events_.clear();

        // setting back to active makes the entity alive and found by iterators
        entityManager.SetActive(entity, true);
        EXPECT_TRUE(entityManager.IsAlive(entity));
        EXPECT_FALSE(begin(entityManager) == end(entityManager));

        ecs->ProcessEvents();

        ASSERT_EQ(entityListener.events_.size(), 1U);
        EXPECT_EQ(entityListener.events_[0U].type, IEcs::EntityListener::EventType::ACTIVATED);
        ASSERT_EQ(entityListener.events_[0U].entities.size(), 1U);
        EXPECT_EQ(entityListener.events_[0U].entities[0U], entity);
        entityListener.events_.clear();

        entityManager.SetActive(entity, false);

        // inactive entity can be destroyed.
        entityManager.Destroy(entity);

        ecs->ProcessEvents();

        ASSERT_EQ(entityListener.events_.size(), 2U);
        EXPECT_EQ(entityListener.events_[0U].type, IEcs::EntityListener::EventType::DEACTIVATED);
        ASSERT_EQ(entityListener.events_[0U].entities.size(), 1U);
        EXPECT_EQ(entityListener.events_[0U].entities[0U], entity);
        EXPECT_EQ(entityListener.events_[1U].type, IEcs::EntityListener::EventType::DESTROYED);
        ASSERT_EQ(entityListener.events_[1U].entities.size(), 1U);
        EXPECT_EQ(entityListener.events_[1U].entities[0U], entity);
        entityListener.events_.clear();

        EXPECT_FALSE(entityManager.IsAlive(entity));
        EXPECT_EQ(begin(entityManager), end(entityManager));

        // should be safe to call with dead entity
        entityManager.SetActive(entity, true);
        EXPECT_FALSE(entityManager.IsAlive(entity));
    }
}

HWTEST_F(EcsTest, timeScale, TestSize.Level1)
{
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    IEcs::Ptr ecs = engine->CreateEcs();
    EXPECT_EQ(&ecs->GetClassFactory(), engine->GetInterface<IClassFactory>());

    EXPECT_TRUE(ecs->CreateComponentManager(testComponentInfo) != nullptr);
    EXPECT_TRUE(ecs->CreateSystem(testSystemInfo) != nullptr);
    ecs->Initialize();

    EXPECT_TRUE(ecs->CreateComponentManager(duplicatedTestComponentInfo) != nullptr);
    EXPECT_TRUE(ecs->CreateComponentManager(duplicatedTestComponentInfo) != nullptr);
    EXPECT_TRUE(ecs->CreateSystem(duplicateTestSystemInfo) != nullptr);
    EXPECT_TRUE(ecs->CreateSystem(duplicateTestSystemInfo) != nullptr);

    TestSystem* testSystem = GetSystem<TestSystem>(*ecs);
    ASSERT_TRUE(testSystem);

    auto* testManager = GetManager<ITestComponentManager>(*ecs);
    ASSERT_TRUE(testManager);

    IEcs* ecsArr[] = { ecs.get() };
    EXPECT_FLOAT_EQ(ecs->GetTimeScale(), 1.f);
    EXPECT_EQ(engine->TickFrame(ecsArr), true);

    static constexpr const float scales[] = { 1.f, 0.5f, 0.25f, 1.25f, 2.f, 1.f };
    for (auto scale : scales) {
        ecs->SetTimeScale(scale);
        EXPECT_FLOAT_EQ(ecs->GetTimeScale(), scale);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        EXPECT_EQ(engine->TickFrame(ecsArr), true);
        const auto time = engine->GetEngineTime();
        const auto delta = testSystem->Delta();
        EXPECT_EQ(static_cast<uint64_t>(time.deltaTimeUs * scale), delta);
    }
}

HWTEST_F(EcsTest, createAndDestroyEntityReference, TestSize.Level1)
{
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    IEcs::Ptr ecs = engine->CreateEcs();
    auto entityListener = EntityListener { *ecs };
    ecs->Initialize();
    auto& entityManager = ecs->GetEntityManager();
    {
        Entity entity;
        {
            // entityRef is the only reference so count is 1
            auto entityRef = entityManager.CreateReferenceCounted();
            EXPECT_EQ(entityRef.GetRefCount(), 1U);
            entity = entityRef;
            EXPECT_TRUE(entityManager.IsAlive(entity));
            {
                // two references
                auto entityRef2 = entityRef;
                // only way to check the ref count is get and release.
                EXPECT_EQ(entityRef2.GetRefCount(), 2U);
            }
            EXPECT_TRUE(entityManager.IsAlive(entity));
        }

        ecs->ProcessEvents();

        ASSERT_EQ(entityListener.events_.size(), 2U);
        EXPECT_EQ(entityListener.events_[0].type, IEntityManager::EventType::CREATED);
        ASSERT_EQ(entityListener.events_[0].entities.size(), 1U);
        EXPECT_EQ(entityListener.events_[0].entities[0], entity);
        EXPECT_EQ(entityListener.events_[1].type, IEntityManager::EventType::DESTROYED);
        ASSERT_EQ(entityListener.events_[1].entities.size(), 1U);
        EXPECT_EQ(entityListener.events_[1].entities[0], entity);
        entityListener.events_.clear();

        EXPECT_FALSE(entityManager.IsAlive(entity));
    }
    {
        Entity entity;
        {
            // Now the same with inactive entity
            auto entityRef = entityManager.CreateReferenceCounted();
            entity = entityRef;
            entityManager.SetActive(entity, false);
            {
                auto entityRef2 = entityRef;
                EXPECT_EQ(entityRef2.GetRefCount(), 2U);
            }
            // inactive entities are not considered alive
            EXPECT_FALSE(entityManager.IsAlive(entity));
        }

        ecs->ProcessEvents();

        ASSERT_EQ(entityListener.events_.size(), 3U);
        EXPECT_EQ(entityListener.events_[0].type, IEntityManager::EventType::CREATED);
        ASSERT_EQ(entityListener.events_[0].entities.size(), 1U);
        EXPECT_EQ(entityListener.events_[0].entities[0], entity);
        EXPECT_EQ(entityListener.events_[1].type, IEntityManager::EventType::DEACTIVATED);
        ASSERT_EQ(entityListener.events_[1].entities.size(), 1U);
        EXPECT_EQ(entityListener.events_[1].entities[0], entity);
        EXPECT_EQ(entityListener.events_[2].type, IEntityManager::EventType::DESTROYED);
        ASSERT_EQ(entityListener.events_[2].entities.size(), 1U);
        EXPECT_EQ(entityListener.events_[2].entities[0], entity);
        entityListener.events_.clear();

        EXPECT_FALSE(entityManager.IsAlive(entity));
    }
    {
        // create a bunch of entities
        BASE_NS::vector<EntityReference> entities;
        entities.reserve(10U);
        for (auto i = 0; i < 10U; ++i) {
            entities.push_back(entityManager.CreateReferenceCounted());
        }

        ecs->ProcessEvents();

        EXPECT_EQ(entityListener.events_.size(), 1U);
        EXPECT_EQ(entityListener.events_[0].entities.size(), entities.size());
        entityListener.events_.clear();

        // count alive entities using iterators
        auto count = 0;
        for (const auto& e : entityManager) {
            ++count;
        }
        EXPECT_EQ(count, entities.size());

        // create a few "holes" by removing entities
        entities.erase(entities.begin() + 1);
        entities.erase(entities.begin() + 2, entities.begin() + 4);

        // count alive entities again
        count = 0;
        for (const auto& e : entityManager) {
            ++count;
        }
        EXPECT_EQ(count, entities.size());

        ecs->ProcessEvents();

        ASSERT_EQ(entityListener.events_.size(), 1U);
        EXPECT_EQ(entityListener.events_[0].type, IEntityManager::EventType::DESTROYED);
        EXPECT_EQ(entityListener.events_[0].entities.size(), 3U);
        entityListener.events_.clear();

        entities.push_back(entityManager.CreateReferenceCounted());
        entities.push_back(entityManager.GetReferenceCounted(entityManager.Create()));
        // count alive entities again, could check that the new ones fill the "holes" but more of an implementation
        // detail.
        count = 0;
        for (const auto& e : entityManager) {
            ++count;
        }
        EXPECT_EQ(count, entities.size());

        ecs->ProcessEvents();

        ASSERT_EQ(entityListener.events_.size(), 1U);
        EXPECT_EQ(entityListener.events_[0].type, IEntityManager::EventType::CREATED);
        EXPECT_EQ(entityListener.events_[0].entities.size(), 2U);
        entityListener.events_.clear();

        // release references an expect to find no entities
        entities.clear();
        count = 0;
        for (const auto& e : entityManager) {
            ++count;
        }
        EXPECT_EQ(count, entities.size());
    }
}

namespace {
size_t GetEntityCount(IEcs& ecs)
{
    CORE_LOG_D("GetEntityCount called");
    size_t entityCount = 0;

    auto& entityManager = ecs.GetEntityManager();
    auto aliveBegin = entityManager.Begin();
    auto aliveEnd = entityManager.End();

    for (auto& it = aliveBegin; !it->Compare(aliveEnd); it->Next()) {
        entityCount++;
    }

    return entityCount;
}
} // namespace

HWTEST_F(EcsTest, destroyEntityReferenceDependencyTree, TestSize.Level1)
{
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    IEcs::Ptr ecs = engine->CreateEcs();

    GetPluginRegister().RegisterTypeInfo(testComponent2Info);
    ecs->CreateComponentManager(testComponent2Info);

    ecs->Initialize();
    auto& entityManager = ecs->GetEntityManager();

    auto* test2Manager = GetManager<ITestComponent2Manager>(*ecs);
    ASSERT_TRUE(test2Manager);

    // Check amount of entities currently in ecs.
    ASSERT_EQ(GetEntityCount(*ecs), 0);
    {
        auto entity1 = entityManager.CreateReferenceCounted();
        ASSERT_EQ(GetEntityCount(*ecs), 1);
        {
            // Make entity 1 reference entity 2
            auto entity2 = entityManager.CreateReferenceCounted();
            test2Manager->Create(entity1);
            test2Manager->Write(entity1)->ref = entity2;
            ASSERT_EQ(GetEntityCount(*ecs), 2);

            // Make entity 2 reference entity 3
            auto entity3 = entityManager.CreateReferenceCounted();
            test2Manager->Create(entity2);
            test2Manager->Write(entity2)->ref = entity3;
            ASSERT_EQ(GetEntityCount(*ecs), 3);
        }
        ecs->ProcessEvents();
        ASSERT_EQ(GetEntityCount(*ecs), 3);
    }
    ecs->ProcessEvents();
    ASSERT_EQ(GetEntityCount(*ecs), 0);
}

HWTEST_F(EcsTest, ecsPluginTest, TestSize.Level1)
{
    GetPluginRegister().RegisterTypeInfo(EcsInfo);
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    EXPECT_TRUE(engine != nullptr);
    IEcs::Ptr ecs = engine->CreateEcs();
    EXPECT_TRUE(ecs != nullptr);
}

HWTEST_F(EcsTest, entityComparison, TestSize.Level1)
{
    constexpr Entity e1 { 1 };
    constexpr Entity e2 { 2 };

    EXPECT_TRUE(e1 == e1);
    EXPECT_FALSE(e1 == e2);
    EXPECT_FALSE(e2 == e1);
    EXPECT_TRUE(e1 != e2);
    EXPECT_FALSE(e1 != e1);
    EXPECT_TRUE(e1 < e2);
    EXPECT_FALSE(e2 < e1);
    EXPECT_TRUE(e1 <= e2);
    EXPECT_FALSE(e2 <= e1);
    EXPECT_TRUE(e1 <= e1);
    EXPECT_TRUE(e2 > e1);
    EXPECT_FALSE(e1 > e2);
    EXPECT_TRUE(e2 >= e1);
    EXPECT_FALSE(e1 >= e2);
    EXPECT_TRUE(e2 >= e2);

    static_assert(e1 == e1);
    static_assert(!(e1 == e2));
    static_assert(!(e2 == e1));
    static_assert(e1 != e2);
    static_assert(!(e1 != e1));
    static_assert(e1 < e2);
    static_assert(!(e2 < e1));
    static_assert(e1 <= e2);
    static_assert(!(e2 <= e1));
    static_assert(e1 <= e1);
    static_assert(e2 > e1);
    static_assert(!(e1 > e2));
    static_assert(e2 >= e1);
    static_assert(!(e1 >= e2));
    static_assert(e2 >= e2);
}
