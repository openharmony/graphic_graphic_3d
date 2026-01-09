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

#include <test_framework.h>

#include <core/resources/intf_resource_manager.h>

#include <meta/api/animation.h>
#include <meta/api/object.h>
#include <meta/base/meta_types.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/animation/intf_animation_controller.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/intf_manual_clock.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/property/construct_property.h>
#include <meta/interface/resource/intf_object_resource.h>
#include <meta/interface/resource/intf_object_template.h>
#include <meta/interface/resource/intf_resource.h>
#include <meta/interface/serialization/intf_refuri_builder.h>

#include "helpers/serialisation_utils.h"
#include "helpers/testing_objects.h"

#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

META_BEGIN_NAMESPACE()
namespace UTest {

class API_ResourceSerializationTest : public ::testing::Test {
public:
protected:
    void SetUp() override
    {
        resources_ = GetObjectRegistry().Create<CORE_NS::IResourceManager>(ClassId::FileResourceManager);
        ASSERT_TRUE(resources_);

        ASSERT_TRUE(resources_->AddResourceType(
            GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::ObjectTemplateType)));

        ASSERT_TRUE(resources_->AddResourceType(
            GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::ObjectResourceType)));

        auto animRes = GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::AnimationResourceType);
        if (auto i = interface_cast<IRefUriBuilderAnchorType>(animRes)) {
            i->AddAnchorType(ClassId::TestType.Id());
        }
        ASSERT_TRUE(resources_->AddResourceType(animRes));

        if (auto res = GetObjectRegistry().Create<IObjectResource>(ClassId::ObjectResourceType)) {
            res->SetResourceType(ClassId::AnimationResourceTemplate);
            resources_->AddResourceType(interface_pointer_cast<CORE_NS::IResourceType>(res));
        }

        auto res = GetObjectRegistry().Create<CORE_NS::IResource>(ClassId::ObjectResource);
        if (auto m = interface_cast<IMetadata>(res)) {
            m->AddProperty(ConstructProperty<int>("prop", 1));
        }
        if (auto i = interface_cast<CORE_NS::ISetResourceId>(res)) {
            i->SetResourceId("app://test_resource.json");
        }
        ASSERT_TRUE(resources_->AddResource(res));

        resources_->SetFileManager(CORE_NS::IFileManager::Ptr(&GetTestEnv()->engine->GetFileManager()));
    }

    void TearDown() override {}

    void BasicAnimationTest(BASE_NS::string_view animPath);
    void BasicAnimationTemplateTest(BASE_NS::string_view animPath);

protected:
    CORE_NS::IResourceManager::Ptr resources_;
};

/**
 * @tc.name: Basic
 * @tc.desc: Tests for Basic. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceSerializationTest, Basic, testing::ext::TestSize.Level1)
{
    TestSerialiser ser;
    ser.SetResourceManager(resources_);
    {
        Object obj(CreateObjectInstance(ClassId::Object));
        auto prop = ConstructProperty<IObject::ConstPtr>(
            "test", interface_pointer_cast<IObject>(resources_->GetResource("app://test_resource.json")));
        ASSERT_TRUE(prop);
        ASSERT_TRUE(prop->GetValue());
        Metadata(obj).AddProperty(prop);
        ser.Export(obj);
    }
    ser.Dump("app://resource.json");
    auto m = ser.Import<IMetadata>();
    ASSERT_TRUE(m);
    auto p = m->GetProperty<IObject::ConstPtr>("test");
    ASSERT_TRUE(p);
    auto res = interface_pointer_cast<CORE_NS::IResource>(p->GetValue());
    ASSERT_TRUE(res);
    EXPECT_EQ(res, resources_->GetResource("app://test_resource.json"));
    EXPECT_EQ(res->GetResourceId(), "app://test_resource.json");
    auto resm = interface_cast<IMetadata>(res);
    ASSERT_TRUE(resm);
    auto rp = resm->GetProperty<int>("prop");
    ASSERT_TRUE(rp);
    EXPECT_EQ(rp->GetValue(), 1);
}

/**
 * @tc.name: ResourceManagerSerialisation
 * @tc.desc: Tests for Resource Manager Serialisation. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceSerializationTest, ResourceManagerSerialisation, testing::ext::TestSize.Level1)
{
    ASSERT_EQ(resources_->Export("app://test.resources"), CORE_NS::IResourceManager::Result::OK);

    auto resman = GetObjectRegistry().Create<CORE_NS::IResourceManager>(ClassId::FileResourceManager);
    ASSERT_TRUE(resman);
    ASSERT_TRUE(
        resman->AddResourceType(GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::ObjectResourceType)));

    resman->SetFileManager(CORE_NS::IFileManager::Ptr(&GetTestEnv()->engine->GetFileManager()));
    ASSERT_EQ(resman->Import("app://test.resources"), CORE_NS::IResourceManager::Result::OK);

    auto res = resman->GetResource("app://test_resource.json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->GetResourceId(), "app://test_resource.json");
    auto resm = interface_cast<IMetadata>(res);
    ASSERT_TRUE(resm);
    auto rp = resm->GetProperty<int>("prop");
    ASSERT_TRUE(rp);
    EXPECT_EQ(rp->GetValue(), 1);
}

META_REGISTER_CLASS(MyInstor, "627e2fa8-b8d4-45c5-92a7-a86a87711e05", META_NS::ObjectCategoryBits::NO_CATEGORY)

class MyInstor : public IntroduceInterfaces<MetaObject, IObjectInstantiator> {
    META_OBJECT(MyInstor, ClassId::MyInstor, IntroduceInterfaces)
public:
    IObject::Ptr Instantiate(const IObjectTemplate& templ, const SharedPtrIInterface& context) override
    {
        Object gen(CreateNew);
        return Update(*gen.GetPtr(), templ, context);
    }
    META_NS::IObject::Ptr Update(
        IObject& gen, const IObjectTemplate& templ, const SharedPtrIInterface& context) override
    {
        META_NS::IObject::Ptr res;
        if (auto obj = GetValue(templ.Content())) {
            Metadata md(obj);
            if (auto count = md.GetProperty<int>("count")) {
                if (auto genmd = interface_cast<IMetadata>(&gen)) {
                    for (int i = 0; i != GetValue(count); ++i) {
                        BASE_NS::string name { BASE_NS::to_string(i) };
                        if (!genmd->GetProperty(name)) {
                            genmd->AddProperty(ConstructProperty<BASE_NS::string>(BASE_NS::to_string(i)));
                        }
                    }
                    res = gen.GetSelf();
                }
            }
        }
        return res;
    }
};

/**
 * @tc.name: ObjectTemplate
 * @tc.desc: Tests for Object Template. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceSerializationTest, ObjectTemplate, testing::ext::TestSize.Level1)
{
    GetObjectRegistry().RegisterObjectType<MyInstor>();
    {
        auto res = GetObjectRegistry().Create<CORE_NS::IResource>(META_NS::ClassId::ObjectTemplate);
        ASSERT_TRUE(res);
        if (auto i = interface_cast<CORE_NS::ISetResourceId>(res)) {
            i->SetResourceId("myobject");
        }

        Object obj(CreateNew);
        Metadata md(obj);
        md.AddProperty<int>("count", 6, ObjectFlagBits::SERIALIZE);

        auto ot = interface_cast<IObjectTemplate>(res);
        ASSERT_TRUE(ot);

        ot->Instantiator()->SetValue(ClassId::MyInstor);
        ot->SetContent(obj);

        ASSERT_TRUE(resources_->AddResource(res, "app://test_template.json"));

        ASSERT_EQ(resources_->Export("app://test_template.resources"), CORE_NS::IResourceManager::Result::OK);
        resources_->RemoveAllResources();
    }

    ASSERT_EQ(resources_->Import("app://test_template.resources"), CORE_NS::IResourceManager::Result::OK);
    auto res = interface_pointer_cast<IObjectTemplate>(resources_->GetResource("myobject"));
    ASSERT_TRUE(res);

    auto obj = res->Instantiate(nullptr);
    ASSERT_TRUE(obj);
    Metadata md(obj);
    EXPECT_EQ(md.GetProperties().size(), 6);

    auto to = res->Content()->GetValue();
    Metadata tomd(to);
    auto p = tomd.GetProperty<int>("count");
    ASSERT_TRUE(p);
    p->SetValue(8);
    EXPECT_TRUE(res->Update(*obj, nullptr));
    EXPECT_EQ(md.GetProperties().size(), 8);

    GetObjectRegistry().UnregisterObjectType<MyInstor>();
}

void API_ResourceSerializationTest::BasicAnimationTest(BASE_NS::string_view animPath)
{
    {
        auto object = CreateTestType();
        ASSERT_TRUE(object);

        auto property = object->First();

        BASE_NS::vector<float> timestamps = { 0.0f, 0.5f, 1.f };
        BASE_NS::vector<int> keyframes = { 10, 50, 100 };

        META_NS::TrackAnimation<int> anim(META_NS::CreateNew);
        anim.SetKeyframes(keyframes)
            .SetTimestamps(timestamps)
            .SetProperty(property)
            .SetDuration(META_NS::TimeSpan::Milliseconds(100));

        ASSERT_TRUE(anim.GetValid());

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(anim)) {
            i->SetResourceId("animation");
        }

        ASSERT_TRUE(resources_->AddResource(interface_pointer_cast<CORE_NS::IResource>(anim), animPath));
        ASSERT_EQ(resources_->Export("app://test_anim_resource.resources"), CORE_NS::IResourceManager::Result::OK);

        resources_->RemoveAllResources();
    }

    auto clock = GetObjectRegistry().Create<IManualClock>(META_NS::ClassId::ManualClock);

    ASSERT_EQ(resources_->Import("app://test_anim_resource.resources"), CORE_NS::IResourceManager::Result::OK);

    auto object = CreateTestType();
    ASSERT_TRUE(object);

    auto anim = interface_pointer_cast<IAnimation>(resources_->GetResource("animation", object));
    ASSERT_TRUE(anim);
    EXPECT_TRUE(anim->Valid()->GetValue());
    interface_cast<META_NS::IStartableAnimation>(anim)->Start();

    clock->IncrementTime(TimeSpan::Milliseconds(1));
    GetAnimationController()->Step(clock);
    clock->IncrementTime(TimeSpan::Milliseconds(101));
    GetAnimationController()->Step(clock);

    EXPECT_EQ(object->First()->GetValue(), 100);
}

TEST_F(API_ResourceSerializationTest, Animation)
{
    SCOPED_TRACE("anim path = 'app://test_anim_resource.anim'");
    BasicAnimationTest("app://test_anim_resource.anim");
}
TEST_F(API_ResourceSerializationTest, AnimationWithoutPath)
{
    SCOPED_TRACE("anim path = ''");
    BasicAnimationTest("");
}

void API_ResourceSerializationTest::BasicAnimationTemplateTest(BASE_NS::string_view animPath)
{
    BASE_NS::vector<float> timestamps = { 0.0f, 0.5f, 1.f };
    BASE_NS::vector<int> keyframes = { 10, 50, 100 };
    auto duration = TimeSpan::Milliseconds(100);
    {
        auto acc = GetObjectRegistry().Create<IResourceTemplateAccess>(META_NS::ClassId::TrackAnimationTemplateAccess);
        ASSERT_TRUE(acc);

        auto templ = acc->CreateEmptyTemplate();

        auto m = interface_cast<META_NS::IMetadata>(templ);
        ASSERT_TRUE(m);
        {
            auto p1 = m->GetArrayProperty<float>("Timestamps", META_NS::MetadataQuery::EXISTING);
            ASSERT_TRUE(p1);
            p1->SetValue(timestamps);
            auto p2 = m->GetProperty("Keyframes", META_NS::MetadataQuery::EXISTING);
            ASSERT_TRUE(p2);
            p2->SetValue(ArrayAny<int>(keyframes));
            auto p3 = m->GetProperty<TimeSpan>("Duration", META_NS::MetadataQuery::EXISTING);
            ASSERT_TRUE(p3);
            p3->SetValue(duration);
        }

        auto object = CreateTestType();
        ASSERT_TRUE(object);

        auto property = object->First();

        META_NS::TrackAnimation<int> anim(META_NS::CreateNew);

        auto resource = interface_pointer_cast<CORE_NS::IResource>(anim);
        auto dt = interface_pointer_cast<IDerivedFromTemplate>(anim);
        ASSERT_TRUE(dt);
        ASSERT_TRUE(dt->SetTemplate(templ));
        // set property must be after setting values from template as it has the property too
        anim.SetProperty(property);

        ASSERT_TRUE(anim.GetValid());

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(templ)) {
            i->SetResourceId("template");
        }
        if (auto i = interface_cast<CORE_NS::ISetResourceId>(anim)) {
            i->SetResourceId("animation");
        }

        ASSERT_TRUE(resources_->AddResource(templ, "app://test_templ_anim_resource.template"));
        ASSERT_TRUE(resources_->AddResource(resource, animPath));
        ASSERT_EQ(
            resources_->Export("app://test_templ_anim_resource.resources"), CORE_NS::IResourceManager::Result::OK);

        resources_->RemoveAllResources();
    }

    auto clock = GetObjectRegistry().Create<IManualClock>(META_NS::ClassId::ManualClock);

    ASSERT_EQ(resources_->Import("app://test_templ_anim_resource.resources"), CORE_NS::IResourceManager::Result::OK);

    auto object = CreateTestType();
    ASSERT_TRUE(object);

    auto anim = interface_pointer_cast<IAnimation>(resources_->GetResource("animation", object));
    ASSERT_TRUE(anim);
    EXPECT_TRUE(anim->Valid()->GetValue());
    interface_cast<META_NS::IStartableAnimation>(anim)->Start();

    clock->IncrementTime(TimeSpan::Milliseconds(1));
    GetAnimationController()->Step(clock);
    clock->IncrementTime(TimeSpan::Milliseconds(101));
    GetAnimationController()->Step(clock);

    EXPECT_EQ(object->First()->GetValue(), 100);
}

TEST_F(API_ResourceSerializationTest, CreateAnimationTemplate)
{
    SCOPED_TRACE("anim path = 'app://test_templ_anim_resource.anim'");
    BasicAnimationTemplateTest("app://test_templ_anim_resource.anim");
}

TEST_F(API_ResourceSerializationTest, CreateAnimationTemplateWithoutPath)
{
    SCOPED_TRACE("anim path = ''");
    BasicAnimationTemplateTest("");
}

} // namespace UTest
META_END_NAMESPACE()
