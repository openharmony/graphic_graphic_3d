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

#include <meta/api/container/find_containers.h>
#include <meta/api/object.h>
#include <meta/base/ref_uri.h>

#include "helpers/testing_objects.h"

META_BEGIN_NAMESPACE()

namespace UTest {

/**
 * @tc.name: Self
 * @tc.desc: Tests for Self. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainerInfoTest, Self, testing::ext::TestSize.Level1)
{
    /*Object object;
    auto info = interface_pointer_cast<IContainerQuery>(object);
    ASSERT_NE(info, nullptr);
    EXPECT_THAT(info->FindAllContainers(), testing::SizeIs(1)); // attachments container
    */

    Object object(CreateInstance(ClassId::Object));
    EXPECT_THAT(FindAllContainers(object), testing::SizeIs(1)); // attachments container
}

// TEST(API_ContainerInfoTest, Container)
//{
//     ParallelAnimation object;
//     auto info = interface_pointer_cast<IContainerQuery>(object);
//     ASSERT_NE(info, nullptr);
//     auto attach = interface_pointer_cast<IAttach>(object);
//     ASSERT_NE(attach, nullptr);
//     auto containers = info->FindAllContainers<IAnimation>();
//     EXPECT_THAT(containers, UnorderedElementsAre(object));
//     containers = info->FindAllContainers();
//     EXPECT_THAT(containers, UnorderedElementsAre(object, attach->GetAttachmentContainer()));
// }

// TEST(API_ContainerInfoTest, Attachments)
//{
//     auto animation = ParallelAnimation();
//     auto container = GetObjectRegistry().Create<IContainer>(ClassId::ObjectContainer);
//     auto object = Object(interface_pointer_cast<IObject>(container)).Attach(animation).Attach(PropertyAnimation());
//     auto info = interface_pointer_cast<IContainerQuery>(object);
//     ASSERT_NE(info, nullptr);
//     auto attach = interface_pointer_cast<IAttach>(object);
//     ASSERT_NE(attach, nullptr);
//     // Get all containers which are compatible with IAnimation (i.e. our ParallelAnimation)
//     auto containers = info->FindAllContainers<IAnimation>();
//     EXPECT_THAT(containers, UnorderedElementsAre(container, animation));
//     // Get all containers which are compatible with IAttachment (i.e. the AttachmentContainer within the object)
//     containers = info->FindAllContainers<IAttachment>();
//     EXPECT_THAT(containers, UnorderedElementsAre(container, attach->GetAttachmentContainer()));
//     // Should return all containers, i.e. the attachment container and all of the attachments
//     // which are themselves a container
//     containers = info->FindAllContainers();
//     EXPECT_THAT(containers, UnorderedElementsAre(container, animation, attach->GetAttachmentContainer()));
// }

// TEST(API_ContainerInfoTest, MultiInterface)
//{
//     auto animation = ParallelAnimation();
//     auto container = GetObjectRegistry().Create<IContainer>(ClassId::ObjectContainer);
//     auto req = interface_cast<IRequiredInterfaces>(container);
//     ASSERT_TRUE(req);
//     EXPECT_TRUE(req->SetRequiredInterfaces({ IAnimation::UID, IAttachment::UID }));
//     auto object = Object(interface_pointer_cast<IObject>(container)).Attach(animation).Attach(PropertyAnimation());
//     auto info = interface_pointer_cast<IContainerQuery>(object);
//     ASSERT_NE(info, nullptr);
//     auto attach = interface_pointer_cast<IAttach>(object);
//     // Only our own container matches the search criteria
//     auto containers = info->FindAllContainers({ IAnimation::UID, IAttachment::UID });
//     EXPECT_THAT(containers, UnorderedElementsAre(container));
// }

// TEST(API_ContainerInfoTest, Invalid)
//{
//     auto animation = ParallelAnimation();
//     auto container = GetObjectRegistry().Create<IContainer>(ClassId::ObjectContainer);
//     auto req = interface_cast<IRequiredInterfaces>(container);
//     ASSERT_TRUE(req);
//     EXPECT_TRUE(req->SetRequiredInterfaces({ IAttach::UID }));
//     auto object = Object(interface_pointer_cast<IObject>(container)).Attach(animation).Attach(PropertyAnimation());
//     auto info = interface_pointer_cast<IContainerQuery>(object);
//     ASSERT_NE(info, nullptr);
//     auto attach = interface_pointer_cast<IAttach>(object);
//     // No containers should match the criteria
//     auto containers = info->FindAllContainers({ IAnimation::UID, IAttachment::UID });
//     EXPECT_THAT(containers, IsEmpty());
// }

// TEST(API_ContainerInfoTest, MaxCount)
//{
//     auto animation = ParallelAnimation();
//     auto container = GetObjectRegistry().Create<IContainer>(ClassId::ObjectContainer);
//     auto req = interface_cast<IRequiredInterfaces>(container);
//     ASSERT_TRUE(req);
//     EXPECT_TRUE(req->SetRequiredInterfaces({ IAttach::UID }));
//     auto object = Object(interface_pointer_cast<IObject>(container)).Attach(animation).Attach(PropertyAnimation());
//     auto info = interface_pointer_cast<IContainerQuery>(object);
//     ASSERT_NE(info, nullptr);
//     auto containers = info->FindAllContainers({ {}, 1 });
//     EXPECT_THAT(containers, UnorderedElementsAre(container));
//     containers = info->FindAllContainers({ { IAnimation::UID }, 1 });
//     EXPECT_THAT(containers, UnorderedElementsAre(animation));
//     EXPECT_TRUE(req->SetRequiredInterfaces({ IAnimation::UID }));
//     containers = info->FindAllContainers({ { IAnimation::UID }, 1 });
//     EXPECT_THAT(containers, UnorderedElementsAre(container));
//     containers = info->FindAllContainers({ { IAnimation::UID }, 0 });
//     EXPECT_THAT(containers, UnorderedElementsAre(container, animation));
//     EXPECT_EQ(info->FindAnyContainer<IAnimation>(), container);
// }
} // namespace UTest
META_END_NAMESPACE()
