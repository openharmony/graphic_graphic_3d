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

#ifndef META_EXT_ATTACHMENT_H
#define META_EXT_ATTACHMENT_H

#include <meta/ext/object_fwd.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_attachment.h>

META_BEGIN_NAMESPACE()

/**
 * @brief The AttachmentBaseFwd class can be used as a base class by classes that want to create an object
 *        which implements the IAttachment interface.
 *
 *        Instead of directly implementing the IAttachment interface methods, AttachmentFwd defines
 *        two methods, AttachTo and DetachFrom which are called when the corresponding Attaching/Detaching
 *        methods of IAttachment are called.
 *
 *        AttachmentFwd handles attachment target and IAttachment::AttachedTo automatically.
 */
template<class BaseInterface>
class AttachmentBaseFwd : public IntroduceInterfaces<ObjectFwd, BaseInterface> {
    using MyBase = IntroduceInterfaces<ObjectFwd, BaseInterface>;
    META_OBJECT_NO_CLASSINFO(AttachmentBaseFwd, MyBase)
protected:
    using Super::Super;

    /** AttachmentFwd calls this when IAttachment::Attaching is called by the framework. */
    virtual bool AttachTo(const META_NS::IAttach::Ptr& target, const META_NS::IObject::Ptr& dataContext) = 0;
    /** AttachmentFwd calls this when IAttachment::Detaching is called by the framework. */
    virtual bool DetachFrom(const META_NS::IAttach::Ptr& target) = 0;

public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(IAttachment, IObject::WeakPtr, DataContext)
    META_STATIC_PROPERTY_DATA(IAttachment, IAttach::WeakPtr, AttachedTo, {}, DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_END_STATIC_DATA()

    /** Implementation of IAttachment::DataContext */
    META_IMPLEMENT_READONLY_PROPERTY(IObject::WeakPtr, DataContext)
    /** Implementation of IAttachment::AttachedTo */
    META_IMPLEMENT_READONLY_PROPERTY(IAttach::WeakPtr, AttachedTo)

private:
    /** Private implementation of IAttachment::Attaching, handle properties and call the AttachTo method defined by
     *  this class */
    bool Attaching(const META_NS::IAttach::Ptr& target, const META_NS::IObject::Ptr& dataContext) final
    {
        if (AttachTo(target, dataContext)) {
            META_ACCESS_PROPERTY(AttachedTo)->SetValue(target);
            META_ACCESS_PROPERTY(DataContext)->SetValue(dataContext);
            return true;
        }
        return false;
    }
    /** Private implementation of IAttachment::Detaching, handle properties and call the DetachFrom method defined by
     *  this class */
    bool Detaching(const META_NS::IAttach::Ptr& target) final
    {
        if (DetachFrom(target)) {
            META_ACCESS_PROPERTY(AttachedTo)->SetValue({});
            META_ACCESS_PROPERTY(DataContext)->SetValue({});
            return true;
        }
        return false;
    }
};

/**
 * @brief The AttachmentFwd is a specialization of AttachmentBaseFwd for the simple cases where
 *        the application uses ClassId::Object as the base class and implements META_NS::IAttachment directly.
 */
class AttachmentFwd : public AttachmentBaseFwd<META_NS::IAttachment> {};

META_END_NAMESPACE()

#endif
