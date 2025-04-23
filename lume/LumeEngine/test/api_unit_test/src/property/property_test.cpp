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

#include <gtest/gtest.h>

#include <base/math/vector.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_handle_util.h>
#include <core/property_tools/core_metadata.inl>
#include <core/property_tools/property_api_impl.h>
#include <core/property_tools/property_api_impl.inl>

#include "TestRunner.h"
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;
using namespace testing::ext;

struct Thing {
    int intVal;
    BASE_NS::Math::Vec2 vec2Val;
    int otherIntVal;
};

namespace {
struct TestContext {
    std::shared_ptr<ISceneInit> sceneInit_ = nullptr;
    CORE_NS::IEcs::Ptr ecs_;
};
static TestContext g_context;

using IntfPtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using IntfWeakPtr = BASE_NS::weak_ptr<CORE_NS::IInterface>;
static constexpr BASE_NS::Uid ENGINE_THREAD{"2070e705-d061-40e4-bfb7-90fad2c280af"};
static constexpr BASE_NS::Uid APP_THREAD{"b2e8cef3-453a-4651-b564-5190f8b5190d"};
static constexpr BASE_NS::Uid IO_QUEUE{"be88e9a0-9cd8-45ab-be48-937953dc258f"};
static constexpr BASE_NS::Uid JS_RELEASE_THREAD{"3784fa96-b25b-4e9c-bbf1-e897d36f73af"};

bool SceneDispose(TestContext &context)
{
    context.ecs_ = nullptr;
    context.sceneInit_ = nullptr;
    return true;
}

bool SceneCreate(TestContext &context)
{
    context.sceneInit_ = CreateTestScene();
    context.sceneInit_->LoadPluginsAndInit();
    if (!context.sceneInit_->GetEngineInstance().engine_) {
        WIDGET_LOGE("fail to get engine");
        return false;
    }
    context.ecs_ = context.sceneInit_->GetEngineInstance().engine_->CreateEcs();
    if (!context.ecs_) {
        WIDGET_LOGE("fail to get ecs");
        return false;
    }
    auto factory = GetInstance<Core::ISystemGraphLoaderFactory>(UID_SYSTEM_GRAPH_LOADER);
    auto systemGraphLoader = factory->Create(context.sceneInit_->GetEngineInstance().engine_->GetFileManager());
    systemGraphLoader->Load("rofs3D://systemGraph.json", *(context.ecs_));
    auto& ecs = *(context.ecs_);
    ecs.Initialize();

    using namespace SCENE_NS;
#if SCENE_META_TEST
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

CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(Thing)

DATA_TYPE_METADATA(Thing, MEMBER_PROPERTY(intVal, "intValue", 0), MEMBER_PROPERTY(otherIntVal, "otherIntValue", 0),
    MEMBER_PROPERTY(vec2Val, "vec2Value", 0))
CORE_END_NAMESPACE()

PROPERTY_LIST(Thing, THING_METADATA, MEMBER_PROPERTY(intVal, "intValue", 0),
    MEMBER_PROPERTY(otherIntVal, "otherIntValue", 0), MEMBER_PROPERTY(vec2Val, "vec2Value", 0))

struct Owner : public CORE_NS::IPropertyApi {
    PROPERTY_LIST(
        Thing, componentMetadata_, MEMBER_PROPERTY(intVal, "intValue", 0), MEMBER_PROPERTY(vec2Val, "vec2Value", 0))

    size_t PropertyCount() const override
    {
        return BASE_NS::countof(componentMetadata_);
    }

    const CORE_NS::Property* MetaData(size_t index) const override
    {
        if (index < BASE_NS::countof(componentMetadata_)) {
            return &componentMetadata_[index];
        }
        return nullptr;
    }

    BASE_NS::array_view<const CORE_NS::Property> MetaData() const override
    {
        return componentMetadata_;
    }

    CORE_NS::IPropertyHandle* Create() const override
    {
        return nullptr;
    }

    CORE_NS::IPropertyHandle* Clone(const CORE_NS::IPropertyHandle* srcHandle) const override
    {
        return nullptr;
    }

    void Release(CORE_NS::IPropertyHandle* handle) const override {}

    uint64_t Type() const override
    {
        return {};
    }

    struct Impl : public CORE_NS::IPropertyHandle {
        const IPropertyApi* Owner() const override
        {
            return nullptr;
        }

        size_t Size() const override
        {
            return {};
        }

        const void* RLock() const override
        {
            return nullptr;
        }

        void RUnlock() const override {}

        void* WLock() override
        {
            return nullptr;
        }

        void WUnlock() override {}
    };
};

struct ThingProperty final : public CORE_NS::PropertyApiImpl<Thing> {
    ThingProperty(Thing* data, BASE_NS::array_view<const CORE_NS::Property> props)
        : CORE_NS::PropertyApiImpl<Thing>(data, props)
    {}
};

class PropertyTest : public testing::Test {
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

/**
 * @tc.name: MakeScopedHandle
 * @tc.desc: test MakeScopedHandle
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, MakeScopedHandle, TestSize.Level1)
{
    Thing thing;
    ThingProperty thingProperty(&thing, THING_METADATA);

    // non-const types (writable)
    {
        auto handle = CORE_NS::MakeScopedHandle<int>(*thingProperty.GetData(), "intVal");
        EXPECT_TRUE(handle);
        if (handle) {
            *handle = 42;
        }
    }
    {
        auto handle = CORE_NS::MakeScopedHandle<BASE_NS::Math::Vec2>(*thingProperty.GetData(), "vec2Val");
        EXPECT_TRUE(handle);
        if (handle) {
            *handle = BASE_NS::Math::Vec2(1.f, 2.f);
        }
    }

    // unknown propertyName
    {
        auto handle = CORE_NS::MakeScopedHandle<int>(*thingProperty.GetData(), "foo");
        EXPECT_FALSE(handle);
    }

    // propertyName doesn't match propertyType (propertyType doesn't match any of the properties)
    {
        auto handle = CORE_NS::MakeScopedHandle<BASE_NS::Math::UVec2>(*thingProperty.GetData(), "vec2Val");
        EXPECT_FALSE(handle);
    }

    // propertyName doesn't match type
    {
        auto handle = CORE_NS::MakeScopedHandle<int>(*thingProperty.GetData(), "vec2Val");
        EXPECT_FALSE(handle);
    }

    // const types (readonly)
    {
        auto handle = CORE_NS::MakeScopedHandle<const int>(*thingProperty.GetData(), "intVal");
        EXPECT_TRUE(handle);
        if (handle) {
            EXPECT_EQ(*handle, 42);
        }
    }
    {
        auto handle = CORE_NS::MakeScopedHandle<const BASE_NS::Math::Vec2>(*thingProperty.GetData(), "vec2Val");
        EXPECT_TRUE(handle);
        if (handle) {
            EXPECT_FLOAT_EQ(handle->x, 1.f);
            EXPECT_FLOAT_EQ(handle->y, 2.f);
        }
    }
}

/**
 * @tc.name: GetSetPropertyValue
 * @tc.desc: test GetSetPropertyValue
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, GetSetPropertyValue, TestSize.Level1)
{
    Thing thing;
    ThingProperty thingProperty(&thing, THING_METADATA);

    EXPECT_TRUE(CORE_NS::SetPropertyValue(thingProperty.GetData(), "vec2Val", BASE_NS::Math::Vec2(2.f, 3.f)));

    // unknown propertyName
    EXPECT_FALSE(CORE_NS::SetPropertyValue(thingProperty.GetData(), "foo", BASE_NS::Math::Vec2(1.f, 2.f)));

    // propertyName doesn't match propertyType (propertyType doesn't match any of the properties)
    EXPECT_FALSE(CORE_NS::SetPropertyValue(thingProperty.GetData(), "vec2Val", BASE_NS::Math::UVec2(2, 3)));

    // propertyName doesn't match propertyType
    EXPECT_FALSE(CORE_NS::SetPropertyValue(thingProperty.GetData(), "intVal", BASE_NS::Math::Vec2(1.f, 2.f)));

    auto vec2Val = CORE_NS::GetPropertyValue<BASE_NS::Math::Vec2>(thingProperty.GetData(), "vec2Val");
    EXPECT_EQ(vec2Val.x, 2.f);
    EXPECT_EQ(vec2Val.y, 3.f);

    // set/get multiple properties with same type
    EXPECT_TRUE(CORE_NS::SetPropertyValue(thingProperty.GetData(), "intVal", 1));
    EXPECT_TRUE(CORE_NS::SetPropertyValue(thingProperty.GetData(), "otherIntVal", 2));
    EXPECT_EQ(CORE_NS::GetPropertyValue<int>(thingProperty.GetData(), "intVal"), 1);
    EXPECT_EQ(CORE_NS::GetPropertyValue<int>(thingProperty.GetData(), "otherIntVal"), 2);
}
