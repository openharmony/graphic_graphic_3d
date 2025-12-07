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

#include <meta/api/animation.h>
#include <meta/interface/animation/intf_animation_controller.h>
#include <meta/interface/property/construct_property.h>
#include <meta/interface/property/property.h>

#include "helpers/animation_test_base.h"
#include "helpers/serialisation_utils.h"
#include "helpers/testing_objects.h"

META_BEGIN_NAMESPACE()

namespace UTest {

class API_AnimationTest : public API_AnimationTestBase {
protected:
    IPropertyAnimation::Ptr CreateAnimation(const META_NS::ClassInfo& classId)
    {
        auto pani = GetObjectRegistry().Create<IPropertyAnimation>(classId);
        if (auto ani = interface_cast<IAnimation>(pani)) {
            ani->OnStarted()->AddHandler(MakeCallback<IOnChanged>([this]() { startedCalled_++; }));
            ani->OnFinished()->AddHandler(MakeCallback<IOnChanged>([this]() { finishedCalled_++; }));
        }
        return pani;
    }

    auto GetStartedCalledCount() const
    {
        return startedCalled_;
    }
    auto GetFinishedCalledCount() const
    {
        return finishedCalled_;
    }

private:
    size_t startedCalled_ {};
    size_t finishedCalled_ {};
};

/**
 * @tc.name: KeyframeAnimation
 * @tc.desc: Tests for Keyframe Animation. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AnimationTest, KeyframeAnimation, testing::ext::TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");
    auto pani = CreateAnimation(ClassId::KeyframeAnimation);
    ASSERT_TRUE(pani);
    auto pp = pani->Property();
    ASSERT_TRUE(pp);
    pp->SetValue(p);

    auto ani = interface_pointer_cast<IKeyframeAnimation>(pani);
    ASSERT_TRUE(ani);
    ani->From()->SetValue(IAny::Ptr(new Any<int>(0)));
    ani->To()->SetValue(IAny::Ptr(new Any<int>(10)));
    ani->Duration()->SetValue(TimeSpan::Milliseconds(100));

    auto st = interface_pointer_cast<IStartableAnimation>(pani);

    st->Start();

    EXPECT_EQ(GetStartedCalledCount(), 1);
    EXPECT_EQ(GetFinishedCalledCount(), 0);

    EXPECT_TRUE(ani->Running()->GetValue());

    this->RunFrames(11, [&](uint32_t frame) {
        auto f = frame - 1;
        EXPECT_EQ(f, p->GetValue());
        EXPECT_EQ(ani->Running()->GetValue(), f < 10);
        EXPECT_FLOAT_EQ(ani->Progress()->GetValue(), f / 10.f);
    });

    EXPECT_FALSE(ani->Running()->GetValue());

    EXPECT_EQ(GetStartedCalledCount(), 1);
    EXPECT_EQ(GetFinishedCalledCount(), 1);
}

/**
 * @tc.name: KeyframeAnimationFinish
 * @tc.desc: Tests for Keyframe Animation Finish. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AnimationTest, KeyframeAnimationFinish, testing::ext::TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");
    auto pani = CreateAnimation(ClassId::KeyframeAnimation);
    META_NS::SetValue(pani->Property(), p);

    auto ani = interface_cast<IKeyframeAnimation>(pani);
    ASSERT_TRUE(ani);
    SetValue(ani->From(), IAny::Ptr(new Any<int>(0)));
    SetValue(ani->To(), IAny::Ptr(new Any<int>(10)));
    SetValue(ani->Duration(), TimeSpan::Milliseconds(100));

    auto st = interface_cast<IStartableAnimation>(pani);

    st->Start();
    EXPECT_TRUE(GetValue(ani->Running()));
    st->Finish();
    EXPECT_FALSE(GetValue(ani->Running()));

    EXPECT_EQ(GetStartedCalledCount(), 1);
    EXPECT_EQ(GetFinishedCalledCount(), 1);
}

/**
 * @tc.name: KeyframeAnimationChangeProperty
 * @tc.desc: Tests for Keyframe Animation Change Property. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AnimationTest, KeyframeAnimationChangeProperty, testing::ext::TestSize.Level1)
{
    auto p1 = ConstructProperty<int>("test");
    auto p2 = ConstructProperty<int>("test2");
    auto pani = CreateAnimation(ClassId::KeyframeAnimation);
    META_NS::SetValue(pani->Property(), p1);
    auto ani = interface_cast<IKeyframeAnimation>(pani);
    auto st = interface_cast<IStartableAnimation>(pani);
    ASSERT_TRUE(ani);
    ASSERT_TRUE(st);
    SetValue(ani->From(), IAny::Ptr(new Any<int>(0)));
    SetValue(ani->To(), IAny::Ptr(new Any<int>(10)));
    SetValue(ani->Duration(), TimeSpan::Milliseconds(100));
    META_NS::SetValue(pani->Property(), p2);
    META_NS::SetValue(pani->Property(), nullptr);
    META_NS::SetValue(pani->Property(), p1);
    st->Start();
    META_NS::SetValue(pani->Property(), p2);
    META_NS::SetValue(pani->Property(), nullptr);
    META_NS::SetValue(pani->Property(), p1);
    st->Stop();
}

/**
 * @tc.name: KeyframeAnimationSerialization
 * @tc.desc: Tests for Keyframe Animation Serialization. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AnimationTest, KeyframeAnimationSerialization, testing::ext::TestSize.Level1)
{
    TestSerialiser ser;
    {
        auto p = ConstructProperty<int>("test");
        auto pani = CreateAnimation(ClassId::KeyframeAnimation);
        ASSERT_TRUE(pani);
        auto pp = pani->Property();
        ASSERT_TRUE(pp);
        pp->SetValue(p);

        auto ani = interface_pointer_cast<IKeyframeAnimation>(pani);
        ASSERT_TRUE(ani);
        ani->From()->SetValue(IAny::Ptr(new Any<int>(0)));
        ani->To()->SetValue(IAny::Ptr(new Any<int>(10)));
        ani->Duration()->SetValue(TimeSpan::Milliseconds(100));

        Object obj(CreateInstance(ClassId::Object));
        Metadata(obj).AddProperty(p);
        AttachmentContainer(obj).Attach(pani);
        ASSERT_TRUE(ser.Export(obj));
        ser.Dump("app://test.json");
    }

    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);
    auto p = obj->GetProperty<int>("test");
    ASSERT_TRUE(p);

    auto att = interface_cast<IAttach>(obj);
    ASSERT_TRUE(att);
    auto attvec = att->GetAttachments<IKeyframeAnimation>();
    ASSERT_EQ(attvec.size(), 1);

    auto pani = interface_pointer_cast<IPropertyAnimation>(attvec.front());
    auto st = interface_pointer_cast<IStartableAnimation>(pani);
    auto ani = interface_pointer_cast<IKeyframeAnimation>(pani);
    ASSERT_TRUE(ani);
    ASSERT_EQ(pani->Property()->GetValue().lock(), p.GetProperty());
    ASSERT_TRUE(ani->Valid()->GetValue());

    st->Start();

    EXPECT_TRUE(ani->Running()->GetValue());

    this->RunFrames(11, [&](uint32_t frame) {
        auto f = frame - 1;
        EXPECT_EQ(f, p->GetValue());
        EXPECT_EQ(ani->Running()->GetValue(), f < 10);
        EXPECT_FLOAT_EQ(ani->Progress()->GetValue(), f / 10.f);
    });

    EXPECT_FALSE(ani->Running()->GetValue());
}

/**
 * @tc.name: ImplicitAnimation
 * @tc.desc: Tests for Implicit Animation. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AnimationTest, DISABLED_ImplicitAnimation, testing::ext::TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");
    auto pani = CreateAnimation(ClassId::PropertyAnimation);
    auto ani = interface_pointer_cast<ITimedAnimation>(pani);
    ASSERT_TRUE(ani && pani);

    EXPECT_TRUE(p->SetValue(10));
    EXPECT_EQ(p->GetValue(), 10);

    EXPECT_TRUE(p->SetValue(0));
    EXPECT_EQ(p->GetValue(), 0);

    EXPECT_TRUE(pani->Property()->SetValue(p));
    EXPECT_TRUE(ani->Duration()->SetValue(TimeSpan::Milliseconds(100)));

    EXPECT_EQ(p->GetValue(), 0);

    // Set the value, it should not immediately change but rather start animating as we
    // run some frames
    EXPECT_TRUE(p->SetValue(10));

    EXPECT_TRUE(ani->Running()->GetValue());
    EXPECT_EQ(p->GetValue(), 0);
    EXPECT_EQ(GetStartedCalledCount(), 1);
    EXPECT_EQ(GetFinishedCalledCount(), 0);

    // Check that property value changes to target gradually
    this->RunFrames(11, [&](uint32_t frame) {
        auto f = frame - 1;
        EXPECT_EQ(f, p->GetValue());
        EXPECT_EQ(ani->Running()->GetValue(), f < 10);
        EXPECT_FLOAT_EQ(ani->Progress()->GetValue(), f / 10.f);
    });

    EXPECT_EQ(GetStartedCalledCount(), 1);
    EXPECT_EQ(GetFinishedCalledCount(), 1);

    EXPECT_FALSE(ani->Running()->GetValue());
    EXPECT_EQ(p->GetValue(), 10);

    // Change the value again and verify that it still changes gradually
    EXPECT_TRUE(p->SetValue(0));
    EXPECT_TRUE(ani->Running()->GetValue());
    EXPECT_EQ(p->GetValue(), 10);

    EXPECT_EQ(GetStartedCalledCount(), 2);
    EXPECT_EQ(GetFinishedCalledCount(), 1);

    this->RunFrames(11, [&](uint32_t frame) {
        auto f = frame - 1;
        EXPECT_FLOAT_EQ(11 - frame, p->GetValue()) << "Frame: " << frame;
        EXPECT_EQ(ani->Running()->GetValue(), f < 10) << "Frame: " << frame;
        EXPECT_FLOAT_EQ(ani->Progress()->GetValue(), f / 10.f) << "Frame: " << frame;
    });

    EXPECT_FALSE(ani->Running()->GetValue());
    EXPECT_EQ(p->GetValue(), 0);

    EXPECT_EQ(GetStartedCalledCount(), 2);
    EXPECT_EQ(GetFinishedCalledCount(), 2);

    // Remove target property, now any changes to property value should be immediate
    pani->Property()->SetValue({});

    EXPECT_TRUE(p->SetValue(10));
    EXPECT_FALSE(ani->Running()->GetValue());
    EXPECT_EQ(p->GetValue(), 10);

    EXPECT_EQ(GetStartedCalledCount(), 2);
    EXPECT_EQ(GetFinishedCalledCount(), 2);
}

/**
 * @tc.name: ImplicitAnimationNoEvaluate
 * @tc.desc: Tests for Implicit Animation No Evaluate. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AnimationTest, ImplicitAnimationNoEvaluate, testing::ext::TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");
    auto pani = CreateAnimation(ClassId::PropertyAnimation);
    auto ani = interface_pointer_cast<ITimedAnimation>(pani);
    ASSERT_TRUE(ani && pani);

    EXPECT_TRUE(pani->Property()->SetValue(p));
    EXPECT_TRUE(ani->Duration()->SetValue(TimeSpan::Milliseconds(100)));

    std::size_t changes = 0;
    p->OnChanged()->AddHandler(MakeCallback<IOnChanged>([&]() { changes++; }));

    // Set the value, it should not immediately change but rather start animating as we
    // run some frames
    EXPECT_TRUE(p->SetValue(10));

    EXPECT_TRUE(ani->Running()->GetValue());

    this->RunFrames(11, [&](uint32_t frame) {
        auto f = frame - 1;
        EXPECT_EQ(ani->Running()->GetValue(), f < 10);
        EXPECT_FLOAT_EQ(ani->Progress()->GetValue(), f / 10.f);
    });

    EXPECT_TRUE(p->SetValue(0));
    EXPECT_EQ(p->GetValue(), 10);
    EXPECT_EQ(changes, 11);
}

/**
 * @tc.name: ImplicitAnimationSerialization
 * @tc.desc: Tests for Implicit Animation Serialization. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AnimationTest, ImplicitAnimationSerialization, testing::ext::TestSize.Level1)
{
    TestSerialiser ser;
    {
        auto p = ConstructProperty<int>("test");
        auto pani = CreateAnimation(ClassId::PropertyAnimation);
        auto ani = interface_pointer_cast<ITimedAnimation>(pani);
        ASSERT_TRUE(ani && pani);

        EXPECT_TRUE(pani->Property()->SetValue(p));
        EXPECT_TRUE(ani->Duration()->SetValue(TimeSpan::Milliseconds(100)));

        Object obj(CreateInstance(ClassId::Object));
        Metadata(obj).AddProperty(p);
        ASSERT_TRUE(ser.Export(obj));
        ser.Dump("app://test.json");
    }

    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);
    auto p = obj->GetProperty<int>("test");
    ASSERT_TRUE(p);

    auto vec = p->GetModifiers<ITimedAnimation>();
    ASSERT_EQ(vec.size(), 1);
    auto ani = vec.front();
    ASSERT_TRUE(ani);
    auto pani = interface_pointer_cast<IPropertyAnimation>(ani);
    ASSERT_TRUE(pani);

    EXPECT_EQ(ani->Duration()->GetValue(), TimeSpan::Milliseconds(100));
    EXPECT_EQ(pani->Property()->GetValue().lock(), p.GetProperty());

    // Set the value, it should not immediately change but rather start animating as we
    // run some frames
    EXPECT_TRUE(p->SetValue(10));

    EXPECT_TRUE(ani->Running()->GetValue());
    EXPECT_EQ(p->GetValue(), 0);

    // Check that property value changes to target gradually
    this->RunFrames(11, [&](uint32_t frame) {
        auto f = frame - 1;
        EXPECT_EQ(f, p->GetValue());
        EXPECT_EQ(ani->Running()->GetValue(), f < 10);
        EXPECT_FLOAT_EQ(ani->Progress()->GetValue(), f / 10.f);
    });

    EXPECT_FALSE(ani->Running()->GetValue());
    EXPECT_EQ(p->GetValue(), 10);
}

/**
 * @tc.name: ImplicitAnimationSerializationWithAttachment
 * @tc.desc: Tests for Implicit Animation Serialization With Attachment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AnimationTest, ImplicitAnimationSerializationWithAttachment, testing::ext::TestSize.Level1)
{
    TestSerialiser ser;
    {
        auto p = ConstructProperty<int>("test");
        auto pani = CreateAnimation(ClassId::PropertyAnimation);
        auto ani = interface_pointer_cast<ITimedAnimation>(pani);
        ASSERT_TRUE(ani && pani);

        EXPECT_TRUE(pani->Property()->SetValue(p));
        EXPECT_TRUE(ani->Duration()->SetValue(TimeSpan::Milliseconds(100)));

        Object obj(CreateInstance(ClassId::Object));
        Metadata(obj).AddProperty(p);
        AttachmentContainer(obj).Attach(interface_pointer_cast<IAttachment>(pani));
        ASSERT_TRUE(ser.Export(obj));
        ser.Dump("app://test.json");
    }

    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);
    auto p = obj->GetProperty<int>("test");
    ASSERT_TRUE(p);

    auto att = interface_cast<IAttach>(obj);
    ASSERT_TRUE(att);
    auto attvec = att->GetAttachments<IPropertyAnimation>();
    EXPECT_EQ(attvec.size(), 1);

    auto vec = p->GetModifiers<ITimedAnimation>();
    ASSERT_EQ(vec.size(), 1);
    auto ani = vec.front();
    ASSERT_TRUE(ani);
    auto pani = interface_pointer_cast<IPropertyAnimation>(ani);
    ASSERT_TRUE(pani);

    EXPECT_EQ(ani->Duration()->GetValue(), TimeSpan::Milliseconds(100));
    EXPECT_EQ(pani->Property()->GetValue().lock(), p.GetProperty());

    // Set the value, it should not immediately change but rather start animating as we
    // run some frames
    EXPECT_TRUE(p->SetValue(10));

    EXPECT_TRUE(ani->Running()->GetValue());
    EXPECT_EQ(p->GetValue(), 0);

    // Check that property value changes to target gradually
    this->RunFrames(11, [&](uint32_t frame) {
        auto f = frame - 1;
        EXPECT_EQ(f, p->GetValue());
        EXPECT_EQ(ani->Running()->GetValue(), f < 10);
        EXPECT_FLOAT_EQ(ani->Progress()->GetValue(), f / 10.f);
    });

    EXPECT_FALSE(ani->Running()->GetValue());
    EXPECT_EQ(p->GetValue(), 10);
}

/**
 * @tc.name: ImplicitAnimationOnChanged
 * @tc.desc: Tests for Implicit Animation On Changed. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AnimationTest, ImplicitAnimationOnChanged, testing::ext::TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");
    auto pani = CreateAnimation(ClassId::PropertyAnimation);
    auto ani = interface_pointer_cast<ITimedAnimation>(pani);
    ASSERT_TRUE(ani && pani);

    int count = 0;
    p->OnChanged()->AddHandler(MakeCallback<IOnChanged>([&] {
        ++count;
        p->GetValue();
    }));

    EXPECT_TRUE(pani->Property()->SetValue(p));
    EXPECT_TRUE(ani->Duration()->SetValue(TimeSpan::Milliseconds(100)));

    // Set the value, it should not immediately change but rather start animating as we
    // run some frames
    EXPECT_TRUE(p->SetValue(10));

    EXPECT_TRUE(ani->Running()->GetValue());
    EXPECT_EQ(p->GetValue(), 0);
    EXPECT_EQ(GetStartedCalledCount(), 1);
    EXPECT_EQ(GetFinishedCalledCount(), 0);

    // Check that property value changes to target gradually
    this->RunFrames(11, [&](uint32_t frame) {
        auto f = frame - 1;
        EXPECT_EQ(f, p->GetValue());
        EXPECT_EQ(ani->Running()->GetValue(), f < 10);
        EXPECT_FLOAT_EQ(ani->Progress()->GetValue(), f / 10.f);
    });

    EXPECT_EQ(GetStartedCalledCount(), 1);
    EXPECT_EQ(GetFinishedCalledCount(), 1);

    EXPECT_FALSE(ani->Running()->GetValue());
    EXPECT_EQ(p->GetValue(), 10);
    EXPECT_TRUE(count >= 10);
}

} // namespace UTest

META_END_NAMESPACE()
