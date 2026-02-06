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

#include <base/containers/unordered_map.h>

#include <meta/api/object.h>
#include <meta/api/util.h>
#include <meta/ext/attachment/attachment.h>
#include <meta/interface/intf_attachment_container.h>

#include "helpers/serialisation_utils.h"
#include "helpers/testing_objects.h"
#include "test_framework.h"

constexpr auto DataContextPropertyName = "FloatProp";

META_BEGIN_NAMESPACE()
namespace UTest {

META_REGISTER_CLASS(MyAttachment, "852bd735-53d3-4701-af76-b87a94e986fe", META_NS::ObjectCategoryBits::APPLICATION)
META_REGISTER_CLASS(
    MySecondAttachment, "d4f734a2-b169-4b90-ae31-76e78cdf07b0", META_NS::ObjectCategoryBits::APPLICATION)

/** An interface that our custom attachment implements */
class IMyAttachment : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMyAttachment, "f09e17bc-7b39-494b-857d-0507abd952e5")
public:
    virtual META_NS::IProperty::Ptr Value() = 0;
    virtual uint32_t GetAttachCount() = 0;
    virtual uint32_t GetDetachCount() = 0;
};

/** An interface not implemented by anyone */
class IMyInvalidAttachment : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMyInvalidAttachment, "9dd9725d-8c55-42b6-a746-2c9e64a9974e")
public:
};

/** Custom attachment */
class MyAttachment : public IntroduceInterfaces<META_NS::AttachmentFwd, IMyAttachment> {
    META_OBJECT(MyAttachment, ClassId::MyAttachment, IntroduceInterfaces)
    using Super::Super;

    bool Build(const IMetadata::Ptr& data) override
    {
        // Add a custom property to our attachment which the test cases can read
        if (property_ = META_NS::ConstructProperty<float>("AttachmentProp", 0); property_) {
            AddProperty(property_);
        }
        return true;
    }

    META_NS::IProperty::Ptr Value() override
    {
        return property_;
    }
    uint32_t GetAttachCount() override
    {
        return attachCount_;
    }
    uint32_t GetDetachCount() override
    {
        return detachCount_;
    }

    virtual bool AttachTo(const META_NS::IAttach::Ptr& target, const IObject::Ptr& dataContext) override
    {
        attachCount_++;

        // Bind our internal property to <DataContextPropertyName> property from the data context,
        // if such a property exists in the data context
        if (auto meta = interface_cast<META_NS::IMetadata>(dataContext)) {
            if (auto prop = meta->GetProperty(DataContextPropertyName)) {
                property_->SetBind(prop);
            }
        }

        return true;
    }
    virtual bool DetachFrom(const META_NS::IAttach::Ptr& target) override
    {
        detachCount_++;
        return true;
    }

private:
    META_NS::Property<float> property_;
    uint32_t attachCount_ {};
    uint32_t detachCount_ {};
};

class MySecondAttachment : public META_NS::AttachmentFwd {
    META_OBJECT(MySecondAttachment, ClassId::MySecondAttachment, AttachmentFwd)
    using Super::Super;

    virtual bool AttachTo(const META_NS::IAttach::Ptr& target, const IObject::Ptr& dataContext) override
    {
        return true;
    }
    virtual bool DetachFrom(const META_NS::IAttach::Ptr& target) override
    {
        return true;
    }
};

class API_AttachTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // Register our custom attachment
        RegisterObjectType<MyAttachment>();
        RegisterObjectType<MySecondAttachment>();
        attachment_ = META_NS::GetObjectRegistry().Create<META_NS::IAttachment>(ClassId::MyAttachment);
        ASSERT_NE(attachment_, nullptr);
        myAttachment_ = interface_pointer_cast<IMyAttachment>(attachment_);
        ASSERT_NE(myAttachment_, nullptr);

        attachment2_ = META_NS::GetObjectRegistry().Create<META_NS::IAttachment>(ClassId::MySecondAttachment);
        ASSERT_NE(attachment2_, nullptr);

        dataContextProperty_ = META_NS::ConstructProperty<float>(DataContextPropertyName, 0);
        Metadata(dataContext_).AddProperty(dataContextProperty_);
        dataContextProperty_->SetValue(42.f);
    }
    void TearDown() override
    {
        myAttachment_.reset();
        attachment_.reset();
        attachment2_.reset();
        // Unregister the attachment
        UnregisterObjectType<MyAttachment>();
        UnregisterObjectType<MySecondAttachment>();
    }

protected:
    META_NS::IAttachment::Ptr attachment_;
    META_NS::IAttachment::Ptr attachment2_;
    META_NS::IAttachment::Ptr attachment3_;
    IMyAttachment::Ptr myAttachment_;

    META_NS::Object dataContext_ { CreateInstance(META_NS::ClassId::Object) };
    META_NS::Property<float> dataContextProperty_;
};

/**
 * @tc.name: AttachDetach
 * @tc.desc: Tests for Attach Detach. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AttachTest, AttachDetach, testing::ext::TestSize.Level1)
{
    META_NS::Object object(CreateInstance(META_NS::ClassId::Object));
    auto attach = interface_pointer_cast<META_NS::IAttach>(object);
    ASSERT_NE(attach, nullptr);
    EXPECT_EQ(myAttachment_->GetAttachCount(), 0);
    EXPECT_EQ(myAttachment_->GetDetachCount(), 0);

    // Should increase attachCount by 1
    EXPECT_TRUE(attach->Attach(attachment_));
    EXPECT_EQ(attachment_->AttachedTo()->GetValue().lock(), object.GetPtr<META_NS::IAttach>());
    EXPECT_EQ(attachment_->DataContext()->GetValue().lock(), nullptr);

    EXPECT_EQ(myAttachment_->GetAttachCount(), 1);
    EXPECT_EQ(myAttachment_->GetDetachCount(), 0);

    // Attach again, should succeed (as it is already attached), but
    // no attach calls should happen
    EXPECT_TRUE(attach->Attach(attachment_));
    EXPECT_EQ(myAttachment_->GetAttachCount(), 1);
    EXPECT_EQ(myAttachment_->GetDetachCount(), 0);

    // Attach another object
    EXPECT_TRUE(attach->Attach(attachment2_));
    EXPECT_EQ(myAttachment_->GetAttachCount(), 1);
    EXPECT_EQ(myAttachment_->GetDetachCount(), 0);

    // Should increase detachCount by 1
    EXPECT_TRUE(attach->Detach(attachment_));

    EXPECT_EQ(myAttachment_->GetAttachCount(), 1);
    EXPECT_EQ(myAttachment_->GetDetachCount(), 1);

    // Should not be able to detach again
    EXPECT_FALSE(attach->Detach(attachment_));

    EXPECT_TRUE(attach->Detach(attachment2_));
    EXPECT_EQ(myAttachment_->GetAttachCount(), 1);
    EXPECT_EQ(myAttachment_->GetDetachCount(), 1);
}

/**
 * @tc.name: GetAttachmentsTemplate
 * @tc.desc: Tests for Get Attachments Template. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AttachTest, GetAttachmentsTemplate, testing::ext::TestSize.Level1)
{
    META_NS::AttachmentContainer object(CreateInstance(META_NS::ClassId::Object));
    EXPECT_TRUE(object.Attach(attachment_));
    EXPECT_TRUE(object.Attach(attachment2_));

    auto attachmentsMy = object.GetAttachments<IMyAttachment>();
    ASSERT_EQ(attachmentsMy.size(), 1);
    auto attachment = attachmentsMy[0];
    ASSERT_NE(attachment, nullptr);
    EXPECT_EQ(interface_pointer_cast<IMyAttachment>(attachment_), attachment);
    EXPECT_EQ(attachment->GetAttachCount(), 1);
    EXPECT_EQ(attachment->GetDetachCount(), 0);
}

/**
 * @tc.name: GetAttachments
 * @tc.desc: Tests for Get Attachments. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AttachTest, GetAttachments, testing::ext::TestSize.Level1)
{
    META_NS::AttachmentContainer object(CreateInstance(META_NS::ClassId::Object));
    EXPECT_TRUE(object.Attach(attachment_));
    EXPECT_TRUE(object.Attach(attachment2_));

    // Get all attachments
    auto attachmentsAll = object.GetAttachments<IAttachment>();
    // Get attachments that implement IMyAttachment
    auto attachmentsMy = PtrArrayCast<IAttachment>(object.GetAttachments({ IMyAttachment::UID }));
    // Get attachments that implement both IMyAttachment and IMyInvalidAttachment (there are none)
    auto attachmentsStrict =
        PtrArrayCast<IAttachment>(object.GetAttachments({ IMyAttachment::UID, IMyInvalidAttachment::UID }, true));
    // Get attachments that implement either IMyAttachment or IMyInvalidAttachment
    auto attachmentsNotStrict =
        PtrArrayCast<IAttachment>(object.GetAttachments({ IMyAttachment::UID, IMyInvalidAttachment::UID }, false));
    // Get attachments that implement both IMyAttachment and IObject
    auto attachmentsObjectStrict =
        PtrArrayCast<IAttachment>(object.GetAttachments({ IMyAttachment::UID, META_NS::IObject::UID }, true));
    // Get attachments that implement any of IMyAttachment or IObject
    auto attachmentsObjectNotStrict =
        PtrArrayCast<IAttachment>(object.GetAttachments({ IMyAttachment::UID, META_NS::IObject::UID }, false));

    EXPECT_THAT(attachmentsAll, testing::UnorderedElementsAre(attachment_, attachment2_));
    EXPECT_THAT(attachmentsStrict, testing::IsEmpty());
    EXPECT_THAT(attachmentsMy, testing::UnorderedElementsAre(attachment_));
    EXPECT_THAT(attachmentsNotStrict, testing::UnorderedElementsAre(attachment_));
    EXPECT_THAT(attachmentsObjectStrict, testing::UnorderedElementsAre(attachment_));
    EXPECT_THAT(attachmentsObjectNotStrict, testing::UnorderedElementsAre(attachment_, attachment2_));

    // Detach attachment2, we should not get it anymore from the object
    EXPECT_TRUE(object.Detach(attachment2_));
    attachmentsAll = object.GetAttachments<IAttachment>();
    attachmentsObjectStrict =
        PtrArrayCast<IAttachment>(object.GetAttachments({ IMyAttachment::UID, META_NS::IObject::UID }, true));
    attachmentsObjectNotStrict =
        PtrArrayCast<IAttachment>(object.GetAttachments({ IMyAttachment::UID, META_NS::IObject::UID }, false));
    EXPECT_THAT(attachmentsAll, testing::UnorderedElementsAre(attachment_));
    EXPECT_THAT(attachmentsObjectStrict, testing::UnorderedElementsAre(attachment_));
    EXPECT_THAT(attachmentsObjectNotStrict, testing::UnorderedElementsAre(attachment_));
}

/**
 * @tc.name: DataContext
 * @tc.desc: Tests for Data Context. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AttachTest, DataContext, testing::ext::TestSize.Level1)
{
    META_NS::AttachmentContainer object(CreateInstance(META_NS::ClassId::Object));

    ASSERT_EQ(GetValue<float>(myAttachment_->Value()), 0);
    // Attach using our custom data context, the attachment
    // should bind Value() to the property in our data context
    EXPECT_TRUE(object.Attach(attachment_, dataContext_));

    EXPECT_EQ(GetValue<float>(myAttachment_->Value()), 42.f);
    // Check that changing data context property is reflected in the attachment Value() property
    dataContextProperty_->SetValue(32.f);
    EXPECT_EQ(GetValue<float>(myAttachment_->Value()), 32.f);
}

/**
 * @tc.name: ObjectAttachment
 * @tc.desc: Tests for Object Attachment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AttachTest, ObjectAttachment, testing::ext::TestSize.Level1)
{
    META_NS::Object object(CreateInstance(META_NS::ClassId::Object));
    auto attach = interface_pointer_cast<META_NS::IAttach>(object);
    ASSERT_NE(attach, nullptr);

    ASSERT_FALSE(attach->HasAttachments());
    META_NS::Object attachment(CreateInstance(
        META_NS::ClassId::Object)); // Attachment which does not implement IAttachment, should still be able to attach
    ASSERT_TRUE(attach->Attach(attachment));

    ASSERT_TRUE(attach->HasAttachments());
    auto attachments = attach->GetAttachments();
    ASSERT_THAT(attachments, testing::SizeIs(1));
    EXPECT_EQ(attachments[0], attachment);

    ASSERT_TRUE(attach->Attach(myAttachment_));

    attachments = attach->GetAttachments();
    ASSERT_THAT(attachments, testing::SizeIs(2));
    EXPECT_THAT(attachments, testing::UnorderedElementsAre(attachment, interface_pointer_cast<IObject>(myAttachment_)));

    auto attachContainer = attach->GetAttachmentContainer();

    EXPECT_TRUE(META_NS::ContainsObject(attachContainer, attachment.GetPtr()));
    EXPECT_TRUE(META_NS::ContainsObject(attachContainer, myAttachment_));
    EXPECT_FALSE(META_NS::ContainsObject(attachContainer, object.GetPtr()));

    ASSERT_TRUE(attach->Detach(attachment));
    ASSERT_TRUE(attach->Detach(myAttachment_));
    ASSERT_FALSE(attach->HasAttachments());
    attachments = attach->GetAttachments();
    ASSERT_THAT(attachments, testing::SizeIs(0));
}

/**
 * @tc.name: ContainerAttachDetach
 * @tc.desc: Tests for Container Attach Detach. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AttachTest, ContainerAttachDetach, testing::ext::TestSize.Level1)
{
    META_NS::Object object(CreateInstance(META_NS::ClassId::Object));
    auto attach = interface_pointer_cast<META_NS::IAttach>(object);
    ASSERT_NE(attach, nullptr);
    EXPECT_EQ(myAttachment_->GetAttachCount(), 0);
    EXPECT_EQ(myAttachment_->GetDetachCount(), 0);
    EXPECT_FALSE(attach->HasAttachments());
    EXPECT_TRUE(attach->GetAttachmentContainer(true));

    EXPECT_TRUE(AttachmentContainer(object).Attach(attachment_));

    auto container = interface_pointer_cast<META_NS::IContainer>(attach->GetAttachmentContainer());
    auto attachContainer = interface_pointer_cast<META_NS::IAttachmentContainer>(container);
    EXPECT_NE(container, nullptr);
    EXPECT_NE(attachContainer, nullptr);

    auto allObj = container->GetAll();
    auto allAtt = PtrArrayCast<IAttachment>(attachContainer->GetAttachments());
    EXPECT_THAT(allObj, testing::SizeIs(1));
    EXPECT_THAT(allAtt, testing::SizeIs(1));
    EXPECT_THAT(allObj, testing::UnorderedElementsAre(interface_pointer_cast<IObject>(attachment_)));
    EXPECT_THAT(allAtt, testing::UnorderedElementsAre(attachment_));

    auto attachment2 = META_NS::GetObjectRegistry().Create<META_NS::IAttachment>(ClassId::MyAttachment);
    auto att2 = interface_cast<IMyAttachment>(attachment2);
    ASSERT_NE(att2, nullptr);
    auto attachment3 = META_NS::GetObjectRegistry().Create<META_NS::IAttachment>(ClassId::MyAttachment);
    auto att3 = interface_cast<IMyAttachment>(attachment3);
    ASSERT_NE(att3, nullptr);

    EXPECT_TRUE(container->Add(attachment2));
    EXPECT_EQ(attachment2->AttachedTo()->GetValue().lock(), object.GetPtr<META_NS::IAttach>());
    EXPECT_TRUE(attachContainer->Attach(attachment3));
    EXPECT_EQ(attachment3->AttachedTo()->GetValue().lock(), object.GetPtr<META_NS::IAttach>());

    EXPECT_EQ(container->GetSize(), 3);
    EXPECT_TRUE(attach->HasAttachments());

    EXPECT_EQ(att2->GetAttachCount(), 1);
    EXPECT_EQ(att2->GetDetachCount(), 0);
    EXPECT_EQ(att3->GetAttachCount(), 1);
    EXPECT_EQ(att3->GetDetachCount(), 0);

    EXPECT_TRUE(attach->Detach(attachment2));
    EXPECT_EQ(attachment2->AttachedTo()->GetValue().lock(), nullptr);
    EXPECT_EQ(att2->GetAttachCount(), 1);
    EXPECT_EQ(att2->GetDetachCount(), 1);

    EXPECT_TRUE(container->Remove(attachment_));
    EXPECT_EQ(attachment_->AttachedTo()->GetValue().lock(), nullptr);
    EXPECT_EQ(myAttachment_->GetAttachCount(), 1);
    EXPECT_EQ(myAttachment_->GetDetachCount(), 1);

    EXPECT_TRUE(attachContainer->Detach(attachment3));
    EXPECT_EQ(attachment3->AttachedTo()->GetValue().lock(), nullptr);
    EXPECT_EQ(att3->GetAttachCount(), 1);
    EXPECT_EQ(att3->GetDetachCount(), 1);

    EXPECT_FALSE(attach->HasAttachments());
    EXPECT_EQ(container->GetSize(), 0);
}

/**
 * @tc.name: ContainerInsert
 * @tc.desc: Tests for Container Insert. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AttachTest, ContainerInsert, testing::ext::TestSize.Level1)
{
    META_NS::Object object(CreateInstance(META_NS::ClassId::Object));
    auto attach = interface_pointer_cast<META_NS::IAttach>(object);
    ASSERT_NE(attach, nullptr);
    auto container = attach->GetAttachmentContainer(true);
    ASSERT_NE(container, nullptr);

    auto attachment2 = META_NS::GetObjectRegistry().Create<META_NS::IAttachment>(ClassId::MyAttachment);
    auto att2 = interface_cast<IMyAttachment>(attachment2);
    ASSERT_NE(att2, nullptr);

    EXPECT_TRUE(AttachmentContainer(object).Attach(attachment_));
    EXPECT_TRUE(container->Insert(0, attachment2));

    auto attachments = attach->GetAttachments<IAttachment>();
    EXPECT_THAT(attachments, testing::ElementsAre(attachment2, attachment_));

    EXPECT_EQ(myAttachment_->GetAttachCount(), 1);
    EXPECT_EQ(myAttachment_->GetDetachCount(), 0);
    EXPECT_EQ(GetValue(attachment_->AttachedTo()).lock(), object.GetPtr<META_NS::IAttach>());

    EXPECT_EQ(att2->GetAttachCount(), 1);
    EXPECT_EQ(att2->GetDetachCount(), 0);
    EXPECT_EQ(GetValue(attachment2->AttachedTo()).lock(), object.GetPtr<META_NS::IAttach>());
}

/**
 * @tc.name: ContainerReplace
 * @tc.desc: Tests for Container Replace. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AttachTest, ContainerReplace, testing::ext::TestSize.Level1)
{
    META_NS::Object object(CreateInstance(META_NS::ClassId::Object));
    auto attach = interface_pointer_cast<META_NS::IAttach>(object);
    ASSERT_NE(attach, nullptr);
    auto container = attach->GetAttachmentContainer(true);
    ASSERT_NE(container, nullptr);

    auto attachment2 = META_NS::GetObjectRegistry().Create<META_NS::IAttachment>(ClassId::MyAttachment);
    auto att2 = interface_cast<IMyAttachment>(attachment2);
    ASSERT_NE(att2, nullptr);

    EXPECT_TRUE(AttachmentContainer(object).Attach(attachment_));
    EXPECT_TRUE(container->Replace(attachment_, attachment2));

    auto attachments = attach->GetAttachments<IAttachment>();
    EXPECT_THAT(attachments, testing::ElementsAre(attachment2));

    EXPECT_EQ(myAttachment_->GetAttachCount(), 1);
    EXPECT_EQ(myAttachment_->GetDetachCount(), 1);
    EXPECT_EQ(GetValue(attachment_->AttachedTo()).lock(), nullptr);

    EXPECT_EQ(att2->GetAttachCount(), 1);
    EXPECT_EQ(att2->GetDetachCount(), 0);
    EXPECT_EQ(GetValue(attachment2->AttachedTo()).lock(), object.GetPtr<META_NS::IAttach>());
}

/**
 * @tc.name: ObjectFlagsDefault
 * @tc.desc: Tests for Object Flags Default. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AttachTest, ObjectFlagsDefault, testing::ext::TestSize.Level1)
{
    TestSerialiser ser;
    AttachmentContainer object(CreateInstance(META_NS::ClassId::Object));
    object.Attach(attachment_);
    object.Attach(attachment2_);

    ASSERT_TRUE(ser.Export(Object(object)));

    auto imported = ser.Import<IAttach>();
    ASSERT_TRUE(imported);

    // Attachments should be serialized by default
    EXPECT_EQ(imported->GetAttachments().size(), 2);
}

/**
 * @tc.name: ObjectFlagsNoAttachments
 * @tc.desc: Tests for Object Flags No Attachments. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AttachTest, ObjectFlagsNoAttachments, testing::ext::TestSize.Level1)
{
    TestSerialiser ser;
    AttachmentContainer object(CreateInstance(META_NS::ClassId::Object));
    object.Attach(attachment_);
    object.Attach(attachment2_);

    auto flags = interface_cast<IObjectFlags>(object);
    ASSERT_TRUE(flags);

    auto objectFlags = flags->GetObjectFlags();
    objectFlags.Clear(ObjectFlagBits::SERIALIZE_ATTACHMENTS);
    flags->SetObjectFlags(objectFlags);

    ASSERT_TRUE(ser.Export(Object(object)));

    auto imported = ser.Import<IAttach>();
    ASSERT_TRUE(imported);

    // No attachments
    EXPECT_EQ(imported->GetAttachments().size(), 0);
}

/**
 * @tc.name: Serialization
 * @tc.desc: Tests for Serialization. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AttachTest, Serialization, testing::ext::TestSize.Level1)
{
    constexpr auto dcxPropertyName = "MyDataContext";

    TestSerialiser ser;

    META_NS::Object originalObject(CreateInstance(META_NS::ClassId::Object));
    EXPECT_TRUE(AttachmentContainer(originalObject).Attach(attachment_, dataContext_));
    EXPECT_TRUE(AttachmentContainer(originalObject).Attach(attachment2_));

    // The data context must also be serialized, so add it as a property in our object
    Metadata(originalObject).AddProperty(ConstructProperty<META_NS::IObject::Ptr>(dcxPropertyName, dataContext_));

    // Export object
    EXPECT_TRUE(ser.Export(originalObject));

    // Import a new instance
    META_NS::Object object(ser.Import());
    ASSERT_TRUE(object);

    // Find the data context object from our deserialized object properties
    auto dataContextProp = Metadata(object).GetProperty<META_NS::IObject::Ptr>(dcxPropertyName);
    ASSERT_TRUE(dataContextProp);
    auto dataContext = dataContextProp->GetValue();
    ASSERT_TRUE(dataContext);

    // We should now have the same 2 attachments in our object
    auto attachments = AttachmentContainer(object).GetAttachments<IAttachment>();
    auto myAttachments = AttachmentContainer(object).GetAttachments<IMyAttachment>();
    ASSERT_THAT(attachments, testing::SizeIs(2));
    ASSERT_THAT(myAttachments, testing::SizeIs(1));

    auto attachment = interface_cast<IAttachment>(myAttachments[0]);
    auto myAttachment = interface_cast<IMyAttachment>(attachment);
    ASSERT_TRUE(attachment);
    ASSERT_TRUE(myAttachment);

    // Check that attach/detach counters have been correctly called
    EXPECT_EQ(attachment->AttachedTo()->GetValue().lock(), object.GetPtr<META_NS::IAttach>());
    auto context = attachment->DataContext()->GetValue().lock();
    EXPECT_EQ(context, dataContext);
    EXPECT_EQ(myAttachment->GetAttachCount(), 1);
    EXPECT_EQ(myAttachment->GetDetachCount(), 0);

    // Check that bindings to the data context were restored
    ASSERT_NE(context, nullptr);
    auto contextValueProp =
        interface_pointer_cast<META_NS::IMetadata>(context)->GetProperty<float>(DataContextPropertyName);
    ASSERT_TRUE(contextValueProp);

    Property<float> myProp(myAttachment->Value());
    ASSERT_TRUE(myProp);
    EXPECT_EQ(myProp->GetValue(), contextValueProp->GetValue());
    EXPECT_EQ(contextValueProp->GetValue(), 42.f);
    contextValueProp->SetValue(32.f);
    EXPECT_EQ(myProp->GetValue(), 32.f);
}

META_REGISTER_CLASS(TestObject, "d4f734a2-b169-4b90-ae32-76e78cdf07b0", META_NS::ObjectCategoryBits::APPLICATION)
META_REGISTER_CLASS(TestAttachment, "d4f734a2-b169-4b90-ae33-76e78cdf07b0", META_NS::ObjectCategoryBits::APPLICATION)

namespace {

class ITestAttachment : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITestAttachment, "f19e17bc-7b39-494b-857d-0507abd952e5")
public:
};

class TestAttachment : public IntroduceInterfaces<AttachmentFwd, ITestAttachment, INamed> {
    META_OBJECT(TestAttachment, ClassId::TestAttachment, IntroduceInterfaces)

    bool AttachTo(const IAttach::Ptr& target, const IObject::Ptr& dataContext) override
    {
        return true;
    }
    bool DetachFrom(const IAttach::Ptr& target) override
    {
        return true;
    }

    BASE_NS::string GetName() const override
    {
        return this->Name()->GetValue().empty() ? Super::GetName() : this->Name()->GetValue();
    }

    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(INamed, BASE_NS::string, Name)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(BASE_NS::string, Name)
};

class TestObject : public ObjectFwd {
    META_OBJECT(TestObject, ClassId::TestObject, ObjectFwd)
public:
    bool Build(const IMetadata::Ptr&) override
    {
        if (auto attach = GetSelf<IAttach>()) {
            auto at = GetObjectRegistry().Create<INamed>(ClassId::TestAttachment);
            at->Name()->SetValue("MyTestAttachement");
            attach->Attach(interface_pointer_cast<IAttachment>(at));

            // one with no name
            attach->Attach(GetObjectRegistry().Create<IAttachment>(ClassId::TestAttachment));

            return true;
        }
        return false;
    }
};

} // namespace

/**
 * @tc.name: DuplicateAttachmentFromSerialization
 * @tc.desc: Tests for Duplicate Attachment From Serialization. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_AttachmentTest, DuplicateAttachmentFromSerialization, testing::ext::TestSize.Level1)
{
    RegisterObjectType<TestAttachment>();
    RegisterObjectType<TestObject>();

    TestSerialiser ser;
    {
        auto obj = GetObjectRegistry().Create<IAttach>(ClassId::TestObject);
        ASSERT_TRUE(obj);

        auto att = GetObjectRegistry().Create<IAttachment>(ClassId::TestAttachment);
        // do not serialise
        interface_cast<IObjectFlags>(att)->SetObjectFlags({});
        obj->Attach(att);

        ASSERT_EQ(obj->GetAttachments().size(), 3);
        ASSERT_TRUE(ser.Export(obj));
    }

    ser.Dump("app://attachment.json");

    auto obj = ser.Import<IAttach>();
    ASSERT_TRUE(obj);
    // the one with no name is duplicated
    ASSERT_EQ(obj->GetAttachments().size(), 3);

    UnregisterObjectType<TestObject>();
    UnregisterObjectType<TestAttachment>();
}

META_REGISTER_CLASS(
    MyTypedAttachment1, "224d1007-1b3f-4f97-9b78-b6799da4bad2", META_NS::ObjectCategoryBits::APPLICATION)
META_REGISTER_CLASS(
    MyTypedAttachment2, "484bcd3f-fe3d-4eff-bbf6-64fe5669621c", META_NS::ObjectCategoryBits::APPLICATION)

class MyTypedAttachment1 : public META_NS::AttachmentFwd {
    META_OBJECT(MyTypedAttachment1, ClassId::MyTypedAttachment1, AttachmentFwd)
public:
    int Value()
    {
        return 1;
    }
    bool AttachTo(const META_NS::IAttach::Ptr& target, const META_NS::IObject::Ptr& dataContext) override
    {
        return true;
    }
    bool DetachFrom(const META_NS::IAttach::Ptr& target) override
    {
        return true;
    }
};
class MyTypedAttachment2 : public META_NS::AttachmentFwd {
    META_OBJECT(MyTypedAttachment2, ClassId::MyTypedAttachment2, AttachmentFwd)
public:
    int Value()
    {
        return 2;
    }
    bool AttachTo(const META_NS::IAttach::Ptr& target, const META_NS::IObject::Ptr& dataContext) override
    {
        return true;
    }
    bool DetachFrom(const META_NS::IAttach::Ptr& target) override
    {
        return true;
    }
};

#if defined(_CPPRTTI) || defined(__GXX_RTTI)
/**
 * @tc.name: ResolveFinalClass
 * @tc.desc: Tests for Resolve Final Class. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_AttachmentTest, ResolveFinalClass, testing::ext::TestSize.Level1)
{
    auto& registry = GetObjectRegistry();
    RegisterObjectType<MyTypedAttachment1>();
    RegisterObjectType<MyTypedAttachment2>();

    {
        META_NS::AttachmentContainer object(CreateInstance(META_NS::ClassId::Object));
        object.Attach(registry.Create<IAttachment>(ClassId::MyTypedAttachment1));
        object.Attach(registry.Create<IAttachment>(ClassId::MyTypedAttachment2));

        MyTypedAttachment1* typed1 {};
        MyTypedAttachment2* typed2 {};
        auto attachments = object.GetAttachments();

        for (auto&& attachment : attachments) {
            // Verify that dynamic_cast from IObject* -> FinalClass* works (RTII must be enabled!)
            if (auto typed = dynamic_cast<MyTypedAttachment1*>(attachment.get())) {
                EXPECT_EQ(typed1, nullptr);
                typed1 = typed;
            } else if (auto typed = dynamic_cast<MyTypedAttachment2*>(attachment.get())) {
                EXPECT_EQ(typed2, nullptr);
                typed2 = typed;
            }
        }

        ASSERT_NE(typed1, nullptr);
        ASSERT_NE(typed2, nullptr);

        EXPECT_EQ(typed1->Value(), 1);
        EXPECT_EQ(typed2->Value(), 2);
    }

    UnregisterObjectType<MyTypedAttachment1>();
    UnregisterObjectType<MyTypedAttachment2>();
}
#else
#ifdef DISABLED_TESTS_ON
/**
 * @tc.name: ResolveFinalClass
 * @tc.desc: Tests for Resolve Final Class. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_AttachmentTest, DISABLED_ResolveFinalClass, testing::ext::TestSize.Level1)
{
    // Skip
}
#endif // DISABLED_TESTS_ON
#endif

/**
 * @tc.name: AttachProperty
 * @tc.desc: Tests for Attach Property. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_AttachmentTest, AttachProperty, testing::ext::TestSize.Level1)
{
    META_NS::AttachmentContainer object(CreateInstance(META_NS::ClassId::Object));
    auto p = ConstructProperty<int>("p");
    EXPECT_TRUE(object.Attach(p));
    auto attachments = object.GetAttachments();
    ASSERT_EQ(attachments.size(), 1);
    EXPECT_EQ(interface_pointer_cast<IObject>(p.GetProperty()), attachments[0]);
    EXPECT_EQ(p->GetOwner().lock(), interface_pointer_cast<IOwner>(object));

    auto m = interface_cast<IMetadata>(object);
    EXPECT_EQ(m->GetProperty("p"), p.GetProperty());
}

struct TestEventInfo {
    constexpr static BASE_NS::Uid UID { "4c6fd37c-1d8a-4319-b95f-e2219560c1c5" };
    constexpr static char const* NAME { "test" };
};
using TestEvent = SimpleEvent<TestEventInfo, void()>;

/**
 * @tc.name: AttachmentsWithSameName
 * @tc.desc: Tests for Attachments With Same Name. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_AttachmentTest, AttachmentsWithSameName, testing::ext::TestSize.Level1)
{
    RegisterObjectType<TestAttachment>();

    auto att1 = GetObjectRegistry().Create<IAttachment>(ClassId::TestAttachment);
    ASSERT_TRUE(att1);
    interface_cast<INamed>(att1)->Name()->SetValue("test");
    auto att2 = GetObjectRegistry().Create<IAttachment>(ClassId::TestAttachment);
    ASSERT_TRUE(att2);
    interface_cast<INamed>(att2)->Name()->SetValue("test");
    auto p = ConstructProperty<int>("test");
    auto e = CreateShared<EventImpl<TestEvent>>();
    Object dummy(CreateInstance(META_NS::ClassId::Object));
    ;
    auto f = CreateFunction("test", dummy.GetPtr(), &IObject::GetName, nullptr);

    AttachmentContainer object(CreateInstance(META_NS::ClassId::Object));
    object.Attach(att1);
    object.Attach(att2);
    object.Attach(p);
    object.Attach(e);
    object.Attach(f);

    EXPECT_EQ(object.GetAttachments().size(), 5);
    EXPECT_EQ(object.GetAttachments<IAttachment>().size(), 2);

    EXPECT_EQ(Metadata(object).GetProperty("test"), p.GetProperty());
    EXPECT_EQ(Metadata(object).GetEvent("test"), e);
    EXPECT_EQ(Metadata(object).GetFunction("test"), f);

    UnregisterObjectType<TestAttachment>();
}

} // namespace UTest
META_END_NAMESPACE()
