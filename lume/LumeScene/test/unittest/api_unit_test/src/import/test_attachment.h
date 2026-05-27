/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef TEST_ATTACHMENT_H
#define TEST_ATTACHMENT_H

#include <scene/interface/intf_node.h>

#include <meta/api/metadata_util.h>
#include <meta/api/object.h>

SCENE_BEGIN_NAMESPACE()
namespace UTest {
namespace {

class ITestAttachment : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITestAttachment, "4c262fcd-b9df-410c-ba98-dd65bc8a3f42")
public:
    META_PROPERTY(float, Float)
    META_PROPERTY(INode::WeakPtr, NodeRef)
    META_PROPERTY(META_NS::IObject::WeakPtr, ObjectRef)
};

META_REGISTER_CLASS(TestAttachment, "3afe351b-e2e8-441b-85f3-e3e0f0750fd1", META_NS::ObjectCategoryBits::NO_CATEGORY)

class TestAttachment : public META_NS::IntroduceInterfaces<META_NS::MetaObject, ITestAttachment, META_NS::INamed> {
    META_OBJECT(TestAttachment, ClassId::TestAttachment, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(ITestAttachment, float, Float)
    META_STATIC_PROPERTY_DATA(ITestAttachment, INode::WeakPtr, NodeRef)
    META_STATIC_PROPERTY_DATA(ITestAttachment, META_NS::IObject::WeakPtr, ObjectRef)
    META_STATIC_PROPERTY_DATA(META_NS::INamed, BASE_NS::string, Name)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(float, Float)
    META_IMPLEMENT_PROPERTY(INode::WeakPtr, NodeRef)
    META_IMPLEMENT_PROPERTY(META_NS::IObject::WeakPtr, ObjectRef)
    META_IMPLEMENT_PROPERTY(BASE_NS::string, Name)

    BASE_NS::string GetName() const override
    {
        return META_NS::GetValue(Name());
    }
};

}  // namespace
}  // namespace UTest
SCENE_END_NAMESPACE()

#endif
