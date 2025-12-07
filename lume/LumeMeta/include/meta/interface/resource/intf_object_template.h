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

#ifndef META_INTERFACE_RESOURCE_IOBJECT_TEMPLATE_H
#define META_INTERFACE_RESOURCE_IOBJECT_TEMPLATE_H

#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/intf_content.h>

META_BEGIN_NAMESPACE()

class IObjectTemplate;

class IObjectInstantiator : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IObjectInstantiator, "454329f5-21d3-47fb-a91c-41a03da30d97")
public:
    virtual IObject::Ptr Instantiate(const IObjectTemplate& templ, const SharedPtrIInterface& context) = 0;
    virtual META_NS::IObject::Ptr Update(
        IObject& inst, const IObjectTemplate& templ, const SharedPtrIInterface& context) = 0;
};

class IObjectTemplate : public IContent {
    META_INTERFACE(IContent, IObjectTemplate, "ac5e6039-22a3-4d8b-b98f-7f09addde319")
public:
    META_PROPERTY(ObjectId, Instantiator);

    virtual IObject::Ptr Instantiate(const SharedPtrIInterface& context) const = 0;
    virtual META_NS::IObject::Ptr Update(IObject& inst, const SharedPtrIInterface& context) const = 0;

    template<typename Interface>
    typename Interface::Ptr Instantiate(const SharedPtrIInterface& context) const
    {
        return interface_pointer_cast<Interface>(Instantiate(context));
    }
};

META_INTERFACE_TYPE(META_NS::IObjectTemplate)

META_END_NAMESPACE()

#endif
