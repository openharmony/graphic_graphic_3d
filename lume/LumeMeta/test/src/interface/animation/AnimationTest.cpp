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
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <meta/api/animation.h>
#include <meta/interface/animation/intf_animation_controller.h>
#include <meta/interface/property/construct_property.h>
#include <meta/interface/property/property.h>

#include "TestRunner.h"
#include "helpers/animation_test_base.h"
#include "helpers/serialisation_utils.h"
#include "helpers/testing_objects.h"

using namespace testing;
using namespace testing::ext;

META_BEGIN_NAMESPACE()

class AnimationTest : public AnimationTestBase {
protected:
    static void SetUpTestSuite()
    {
        SetTest();
    }
    static void TearDownTestSuite()
    {
        TearDownTest();
    }
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

HWTEST_F(AnimationTest, KeyframeAnimation, TestSize.Level1)
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

HWTEST_F(AnimationTest, KeyframeAnimationSerialization, TestSize.Level1)
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

HWTEST_F(AnimationTest, ImplicitAnimation, TestSize.Level1)
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

HWTEST_F(AnimationTest, ImplicitAnimationNoEvaluate, TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");
    auto pani = CreateAnimation(ClassId::PropertyAnimation);
    auto ani = interface_pointer_cast<ITimedAnimation>(pani);
    ASSERT_TRUE(ani && pani);

    EXPECT_TRUE(pani->Property()->SetValue(p));
    EXPECT_TRUE(ani->Duration()->SetValue(TimeSpan::Milliseconds(100)));

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
}

HWTEST_F(AnimationTest, ImplicitAnimationSerialization, TestSize.Level1)
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

HWTEST_F(AnimationTest, ImplicitAnimationSerializationWithAttachment, TestSize.Level1)
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
        ser.Dump("file://./test.json");
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
 * @tc.desc: test ImplicitAnimationOnChanged
 * @tc.type: FUNC
 */
HWTEST_F(AnimationTest, ImplicitAnimationOnChanged, TestSize.Level1)
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
META_END_NAMESPACE()
