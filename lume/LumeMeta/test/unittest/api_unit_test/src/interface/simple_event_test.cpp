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

#include <meta/api/make_callback.h>
#include <meta/ext/event_impl.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_cloneable.h>
#include <meta/interface/intf_container.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/property/property_events.h>
#include <meta/interface/simple_event.h>

META_BEGIN_NAMESPACE()

namespace UTest {
namespace {

constexpr BASE_NS::Uid uid1 { "234b88b1-84db-46c2-b837-d294d9f806a2" };
constexpr BASE_NS::Uid uid2 { "637b87b2-845b-4652-b857-d29459f806a2" };

struct test1Info {
    constexpr static BASE_NS::Uid UID { uid1 };
    constexpr static char const* NAME { "Test" };
};

struct test2Info {
    constexpr static BASE_NS::Uid UID { uid2 };
    constexpr static char const* NAME { "Test" };
};

using Event1 = SimpleEvent<test1Info>;
using Event2 = SimpleEvent<test2Info, void(int, int)>;

} // namespace

/**
 * @tc.name: CreateSimpleEvent
 * @tc.desc: Tests for Create Simple Event. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EventTest, CreateSimpleEvent, testing::ext::TestSize.Level1)
{
    EXPECT_EQ(Event1::UID, test1Info::UID);
    EXPECT_EQ(Event2::UID, test2Info::UID);
}

/**
 * @tc.name: Methods
 * @tc.desc: Tests for Methods. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EventTest, Methods, testing::ext::TestSize.Level1)
{
    auto c = GetObjectRegistry().Create<IContainer>(ClassId::ObjectFlatContainer);
    ASSERT_TRUE(c);
    auto event = c->OnContainerChanged();
    EXPECT_EQ(event->GetHandlers().size(), 0);
    ChildChangedInfo info;
    META_NS::Invoke<META_NS::IOnChildChanged>(event, info);
    META_NS::Invoke<META_NS::IOnChanged>(event); // Wrong type
    event->AddHandler(META_NS::MakeCallback<META_NS::IOnChildChanged>([](const ChildChangedInfo&) {}));
    EXPECT_EQ(event->GetHandlers().size(), 1);
    EXPECT_TRUE(event->HasHandlers());
    EXPECT_FALSE(event->GetEventTypeName().empty());
    EXPECT_NE(event->GetCallableUid(), BASE_NS::Uid {});
    auto object = interface_pointer_cast<META_NS::IObject>(event.GetEvent());
    ASSERT_TRUE(object);
    EXPECT_EQ(object->GetClassId(), BASE_NS::Uid {});
    EXPECT_EQ(object->GetClassName(), "Event");
    EXPECT_FALSE(object->GetInterfaces().empty());
    auto cloneable = interface_pointer_cast<ICloneable>(object);
    ASSERT_TRUE(cloneable);
    EXPECT_TRUE(cloneable->GetClone());
}

/**
 * @tc.name: DestroyAtInvoke
 * @tc.desc: Tests for Destroy At Invoke. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EventTest, DestroyAtInvoke, testing::ext::TestSize.Level1)
{
    constexpr auto name = "DestroyedEvent";
    size_t callCount = 0;
    BASE_NS::shared_ptr<EventImpl<IOnChanged>> event { new EventImpl<IOnChanged>(name) };
    auto* ptr = event.get();
    event->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([&]() {
        event.reset();
        callCount++;
    }));
    event->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([&]() {
        callCount++;
        EXPECT_EQ(ptr->GetName(), name);
    }));
    event->Invoke();
    EXPECT_EQ(callCount, 2);
    EXPECT_FALSE(event);
}

} // namespace UTest

META_END_NAMESPACE()
