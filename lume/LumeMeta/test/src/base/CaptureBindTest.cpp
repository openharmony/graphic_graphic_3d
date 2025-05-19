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
#include <functional>

#include <gtest/gtest.h>

#include <meta/api/make_callback.h>
#include <meta/api/object.h>
#include <meta/base/capture.h>
#include <meta/ext/attachment/attachment.h>
#include <meta/interface/intf_object_registry.h>

#include "TestRunner.h"
#include "src/testing_objects.h"
#include "src/util.h"

using namespace testing::ext;

META_BEGIN_NAMESPACE()

class CaptureBindTest : public testing::Test {
public:
    static void SetUpTestSuite()
    {
        SetTest();
    }
    static void TearDownTestSuite()
    {
        TearDownTest();
    }
    void SetUp() {}
    void TearDown() {}
};

HWTEST_F(CaptureBindTest, LambdaAutoLocksCapturedSharedPtr, TestSize.Level1)
{
    {
        BASE_NS::shared_ptr<int> sp(new int { 6 });
        auto f = CaptureSafe([](auto p) { *p = 1; }, sp);

        EXPECT_EQ(*sp, 6);
        EXPECT_NO_THROW(f());
        EXPECT_EQ(*sp, 1);
    }

    {
        const BASE_NS::shared_ptr<int> sp(new int { 6 });
        auto f = CaptureSafe([](auto p) { *p = 1; }, sp);

        EXPECT_EQ(*sp, 6);
        EXPECT_NO_THROW(f());
        EXPECT_EQ(*sp, 1);
    }
}

HWTEST_F(CaptureBindTest, LambdaAutoLocksCapturedWeakPtr, TestSize.Level1)
{
    {
        BASE_NS::shared_ptr<int> sp(new int { 6 });
        auto wp = BASE_NS::weak_ptr(sp);
        auto f = CaptureSafe([](auto p) { *p = 1; }, wp);

        EXPECT_EQ(*sp, 6);
        EXPECT_NO_THROW(f());
        EXPECT_EQ(*sp, 1);
    }

    {
        BASE_NS::shared_ptr<int> sp(new int { 6 });
        const auto wp = BASE_NS::weak_ptr(sp);
        auto f = CaptureSafe([](auto p) { *p = 1; }, wp);

        EXPECT_EQ(*sp, 6);
        EXPECT_NO_THROW(f());
        EXPECT_EQ(*sp, 1);
    }
}

HWTEST_F(CaptureBindTest, LambdaNotThrowsWhenCapturedSharedPointerIsReleased, TestSize.Level1)
{
    {
        BASE_NS::shared_ptr<int> p(new int {});

        auto f = CaptureSafe([](auto p) { *p = 1; }, p);
        p.reset();

        EXPECT_NO_THROW(f());
    }
}

HWTEST_F(CaptureBindTest, LambdaNotThrowsWhenCapturedWeakPointerPointsToInvalidResource, TestSize.Level1)
{
    {
        BASE_NS::shared_ptr<int> sp(new int {});

        auto f = CaptureSafe([](auto p) { *p = 1; }, BASE_NS::weak_ptr(sp));
        sp.reset();

        EXPECT_NO_THROW(f());
    }

    {
        BASE_NS::shared_ptr<int> sp(new int {});
        BASE_NS::weak_ptr<int> wp = BASE_NS::weak_ptr(sp);

        auto f = CaptureSafe([](auto p) { *p = 1; }, wp);
        sp.reset();

        EXPECT_NO_THROW(f());
    }
}

HWTEST_F(CaptureBindTest, SimpleLambda, TestSize.Level1)
{
    BASE_NS::shared_ptr<int> p(new int {});

    {
        auto f = Capture([](auto pp) { *pp = 1; }, p);
        f();
        EXPECT_EQ(*p, 1);
    }
    {
        auto f = Capture([](auto p, int v) { *p = v; }, p);
        f(2);
        EXPECT_EQ(*p, 2);
    }
    {
        auto f = Capture([]() { return true; });
        EXPECT_TRUE(f());
    }
    {
        auto f = Capture([](int v) { v = 1; });
        int i = 0;
        f(i);
        EXPECT_EQ(i, 0);
    }
    {
        auto f = Capture([](int& v) { v = 1; });
        int i = 0;
        f(i);
        EXPECT_EQ(i, 1);
    }
    {
        BASE_NS::shared_ptr<int> p(new int {});
        auto f = Capture([](auto p) { return (bool)p; }, p);
        EXPECT_TRUE(f());
        p.reset();
        EXPECT_FALSE(f());
    }
    {
        std::function<bool()> func;
        {
            const BASE_NS::shared_ptr<int> p(new int {});
            func = Capture([](auto p) { return (bool)p; }, p);
            EXPECT_TRUE(func());
        }
        EXPECT_FALSE(func());
    }
    {
        BASE_NS::shared_ptr<int> p(new int {});
        auto other = p;
        auto f = Capture([](auto p) { return (bool)p; }, BASE_NS::move(p));
        EXPECT_TRUE(f());
        other.reset();
        EXPECT_FALSE(f());
    }
}

HWTEST_F(CaptureBindTest, MakeCallback, TestSize.Level1)
{
    auto& registry = GetObjectRegistry();
    auto p = META_NS::ConstructProperty<int>(registry, "P1", 0);

    BASE_NS::shared_ptr<int> count(new int {});
    p->OnChanged()->AddHandler(MakeCallback<IOnChanged>(
        [](auto i) {
            if (i) {
                ++*i;
            }
        },
        count));

    p->SetValue(2);
    EXPECT_EQ(*count, 1);
}

HWTEST_F(CaptureBindTest, MakeCallable, TestSize.Level1)
{
    auto& registry = GetObjectRegistry();
    auto c = CreateTestContainer<IContainer>("Test");

    BASE_NS::shared_ptr<int> count(new int {});
    c->OnContainerChanged()->AddHandler(MakeCallback<IOnChildChanged>(
        [](auto i, const ChildChangedInfo& info) {
            if (i) {
                ++*i;
            }
        },
        count));

    c->Add(Object {});
    EXPECT_EQ(*count, 1);
}

namespace {
META_REGISTER_CLASS(TestAttachment, "d4e854a2-b169-4b90-ae31-76e78cdf07b0", META_NS::ObjectCategoryBits::APPLICATION)

class TestAttachment : public META_NS::AttachmentFwd {
    META_OBJECT(TestAttachment, ClassId::TestAttachment, AttachmentFwd)
    bool AttachTo(const META_NS::IAttach::Ptr& target, const IObject::Ptr& dataContext) override
    {
        return true;
    }
    bool DetachFrom(const META_NS::IAttach::Ptr& target) override
    {
        return true;
    }
};
} // namespace

HWTEST_F(CaptureBindTest, IObject, TestSize.Level1)
{
    RegisterObjectType<TestAttachment>();
    IObject::Ptr obj = Object {}.GetIObject();
    auto func = Capture(
        [](IObject::Ptr p) {
            if (auto a = interface_cast<IAttach>(p)) {
                a->Attach(GetObjectRegistry().Create<IAttachment>(Meta::ClassId::TestAttachment));
                return true;
            }
            return false;
        },
        obj);

    EXPECT_TRUE(func());
    EXPECT_EQ(interface_cast<IAttach>(obj)->GetAttachments().size(), 1);
    obj.reset();
    EXPECT_FALSE(func());
    UnregisterObjectType<TestAttachment>();
}

META_END_NAMESPACE()
