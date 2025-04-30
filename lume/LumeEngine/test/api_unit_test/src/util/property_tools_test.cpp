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

#include <core/property_tools/core_metadata.inl>
#include <core/property_tools/property_data.h>

CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(BASE_NS::vector<BASE_NS::Math::Vec2>);
CORE_END_NAMESPACE()

#include "TestRunner.h"
using namespace CORE_NS;
using namespace testing::ext;

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

namespace {
struct Test {
    float fValue;
    BASE_NS::Math::Vec4 vec4;
    BASE_NS::Math::Vec2 vec2Array[4];
    BASE_NS::vector<float> fVector;
    BASE_NS::vector<BASE_NS::Math::Vec2> vec2Vector;
};

// clang-format off

PROPERTY_LIST(Test, TestProperties,
    MEMBER_PROPERTY(vec4, "Factor", 0U),
    MEMBER_PROPERTY(vec2Array, "Factor array", 0U),
    MEMBER_PROPERTY(fVector, "Vector of floats", 0U),
    MEMBER_PROPERTY(fValue, "Float Value", 0U),
    MEMBER_PROPERTY(vec2Vector, "Vector of Vec2 Value", 0U))
// clang-format on
} // namespace

template<typename To, typename From>
auto Cast(From* f)
{
    if constexpr (BASE_NS::is_const_v<From>) {
        return static_cast<const To*>(static_cast<const void*>(f));
    } else {
        return static_cast<To*>(static_cast<void*>(f));
    }
}

class PropertyToolsTest : public testing::Test {
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
 * @tc.name: FindProperty
 * @tc.desc: test FindProperty
 * @tc.type: FUNC
 */
HWTEST_F(PropertyToolsTest, FindProperty, TestSize.Level1)
{
    ::Test t { 42.f, { 1.f, 2.f, 3.f, 4.f }, { { 5.f, 6.f }, { 7.f, 8.f }, { 9.f, 10.f }, { 11.f, 12.f } } };
    t.fVector.push_back(13.f);
    const auto* tStart = Cast<const uint8_t>(&t);
    // direct member
    {
        const auto fValueProperty = CORE_NS::PropertyData::FindProperty(TestProperties, "fValue", 0);
        EXPECT_TRUE((fValueProperty));
        EXPECT_EQ(fValueProperty.offset, offsetof(::Test, fValue));
        EXPECT_EQ(*Cast<float>(tStart + fValueProperty.offset), t.fValue);
    }
    {
        const auto vec4Property = CORE_NS::PropertyData::FindProperty(TestProperties, "vec4", 0);
        EXPECT_TRUE((vec4Property));
        EXPECT_EQ(vec4Property.offset, offsetof(::Test, vec4));
        EXPECT_EQ(*Cast<BASE_NS::Math::Vec4>(tStart + vec4Property.offset), t.vec4);
    }
    {
        // try giving the address of the variable. returned offset should then point directly to the member and e.g. can
        // be used with the properties containerMethods.
        const auto fVectorProperty =
            CORE_NS::PropertyData::FindProperty(TestProperties, "fVector", reinterpret_cast<uintptr_t>(tStart));
        ASSERT_TRUE((fVectorProperty));
        EXPECT_EQ(fVectorProperty.offset, reinterpret_cast<uintptr_t>(tStart) + offsetof(::Test, fVector));
        const auto* containerMethods = fVectorProperty.property->metaData.containerMethods;
        ASSERT_TRUE(containerMethods);
        EXPECT_EQ(containerMethods->size(fVectorProperty.offset), t.fVector.size());
        EXPECT_EQ(*reinterpret_cast<const float*>(containerMethods->get(fVectorProperty.offset, 0)), t.fVector[0]);
    }

    // missing properties
    {
        const auto vecProperty = CORE_NS::PropertyData::FindProperty(TestProperties, "vec", 0);
        EXPECT_FALSE((vecProperty));
    }
}

/**
 * @tc.name: FindPropertyMember
 * @tc.desc: test FindPropertyMember
 * @tc.type: FUNC
 */
HWTEST_F(PropertyToolsTest, FindPropertyMember, TestSize.Level1)
{
    ::Test t { 42.f, { 1.f, 2.f, 3.f, 4.f }, { { 5.f, 6.f }, { 7.f, 8.f }, { 9.f, 10.f }, { 11.f, 12.f } } };
    t.fVector.push_back(13.f);
    const auto* tStart = Cast<const uint8_t>(&t);

    // member's member
    {
        const auto vec4YProperty = CORE_NS::PropertyData::FindProperty(TestProperties, "vec4.y", 0);
        EXPECT_TRUE((vec4YProperty));
        EXPECT_EQ(vec4YProperty.offset, offsetof(::Test, vec4) + offsetof(BASE_NS::Math::Vec4, y));
        EXPECT_EQ(*Cast<float>(tStart + vec4YProperty.offset), t.vec4.y);
    }

    // missing properties
    {
        const auto vec4BProperty = CORE_NS::PropertyData::FindProperty(TestProperties, "vec4.b", 0);
        EXPECT_FALSE((vec4BProperty));
    }
    {
        const auto vec4Property = CORE_NS::PropertyData::FindProperty(TestProperties, "vec4.", 0);
        EXPECT_FALSE((vec4Property));
    }

    // invalid syntax
    {
        const auto property = CORE_NS::PropertyData::FindProperty(TestProperties, ".", 0);
        EXPECT_FALSE((property));
    }
}

/**
 * @tc.name: FindPropertyArray
 * @tc.desc: test FindPropertyArray
 * @tc.type: FUNC
 */
HWTEST_F(PropertyToolsTest, FindPropertyArray, TestSize.Level1)
{
    ::Test t { 42.f, { 1.f, 2.f, 3.f, 4.f }, { { 5.f, 6.f }, { 7.f, 8.f }, { 9.f, 10.f }, { 11.f, 12.f } } };
    t.fVector.push_back(13.f);
    t.vec2Vector.push_back({ 14.f, 15.f });
    t.vec2Vector.push_back({ 16.f, 17.f });

    const auto* tStart = Cast<const uint8_t>(&t);

    // array
    {
        const auto vec2ArrayProperty = CORE_NS::PropertyData::FindProperty(TestProperties, "vec2Array[1]", 0);
        EXPECT_TRUE((vec2ArrayProperty));
        EXPECT_EQ(vec2ArrayProperty.offset, offsetof(::Test, vec2Array) + sizeof(BASE_NS::Math::Vec2));
        EXPECT_EQ(*Cast<BASE_NS::Math::Vec2>(tStart + vec2ArrayProperty.offset), t.vec2Array[1]);
    }

    // member from array
    {
        const auto vec2ArrayProperty = CORE_NS::PropertyData::FindProperty(TestProperties, "vec2Array[2].x", 0);
        EXPECT_TRUE((vec2ArrayProperty));
        EXPECT_EQ(vec2ArrayProperty.offset,
            offsetof(::Test, vec2Array) + (2 * sizeof(BASE_NS::Math::Vec2)) + offsetof(BASE_NS::Math::Vec2, x));
        EXPECT_EQ(*Cast<float>(tStart + vec2ArrayProperty.offset), t.vec2Array[2].x);
    }

    // member from vector
    {
        const auto vec2VectorProperty = CORE_NS::PropertyData::FindProperty(TestProperties, "vec2Vector", 0);
        EXPECT_TRUE((vec2VectorProperty));
        EXPECT_EQ(vec2VectorProperty.offset, offsetof(::Test, vec2Vector));
        const auto vec2Property =
            CORE_NS::PropertyData::FindProperty(TestProperties, "vec2Vector[1]", reinterpret_cast<uintptr_t>(tStart));
        EXPECT_TRUE((vec2Property));
        EXPECT_EQ(vec2Property.offset, reinterpret_cast<uintptr_t>(&t.vec2Vector[1]));
        const auto floatProperty =
            CORE_NS::PropertyData::FindProperty(TestProperties, "vec2Vector[1].y", reinterpret_cast<uintptr_t>(tStart));
        EXPECT_TRUE((floatProperty));
        EXPECT_EQ(*reinterpret_cast<const float*>(floatProperty.offset), (t.vec2Vector[1].y));
    }

    // invalid syntax
    {
        const auto vec2ArrayProperty = CORE_NS::PropertyData::FindProperty(TestProperties, "vec2Array[]", 0);
        EXPECT_FALSE((vec2ArrayProperty));
    }
    {
        const auto vec2ArrayProperty = CORE_NS::PropertyData::FindProperty(TestProperties, "vec2Array[1", 0);
        EXPECT_FALSE((vec2ArrayProperty));
    }
    {
        const auto vec2ArrayProperty = CORE_NS::PropertyData::FindProperty(TestProperties, "vec2Array[", 0);
        EXPECT_FALSE((vec2ArrayProperty));
    }

    // over indexing
    {
        const auto vec2ArrayProperty = CORE_NS::PropertyData::FindProperty(TestProperties, "vec2Array[6]", 0);
        EXPECT_FALSE((vec2ArrayProperty));
    }
}