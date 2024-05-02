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

#ifndef META_EXT_CONTENT_OBJECT_H
#define META_EXT_CONTENT_OBJECT_H

#include <meta/interface/intf_content.h>
#include <meta/interface/intf_iterable.h>
#include <meta/interface/intf_required_interfaces.h>

#include "object.h"

META_BEGIN_NAMESPACE()

/**
 * @brief A helper class which can be inherited by custom ContentObject implementations to prevent re-implementing these
 *        forwarders.
 *
 *        This forwarder always uses ClassId::ContentObject as the base instance, and also always specifies that the
 *        class implements interfaces from ObjectFwd and IContent.
 */
template<class FinalClass, const ClassInfo& ClassInfo, class... Interfaces>
class ContentObjectFwd : public ObjectFwd<FinalClass, ClassInfo, ClassId::ContentObject, IContent, IRequiredInterfaces,
                             IIterable, Interfaces...> {
    using Super = ObjectFwd<FinalClass, ClassInfo, ClassId::ContentObject, IContent, IRequiredInterfaces, IIterable,
        Interfaces...>;

protected:
    META_FORWARD_READONLY_PROPERTY(IObject::Ptr, Content, content_->PropertyContent())
    META_FORWARD_PROPERTY(bool, ContentSearchable, content_->PropertyContentSearchable())
    META_FORWARD_PROPERTY(IContentLoader::Ptr, ContentLoader, content_->PropertyContentLoader())

    void SetSuperInstance(const IObject::Ptr& aggr, const IObject::Ptr& super) override
    {
        Super::SetSuperInstance(aggr, super);
        content_ = interface_cast<IContent>(super);
        assert(content_);
        iterable_ = interface_cast<IIterable>(super);
        assert(iterable_);
    }
    bool SetRequiredInterfaces(const BASE_NS::vector<TypeId>& interfaces) override
    {
        const auto req = interface_cast<IRequiredInterfaces>(GetSelf());
        return req ? req->SetRequiredInterfaces(interfaces) : false;
    }
    BASE_NS::vector<TypeId> GetRequiredInterfaces() const override
    {
        const auto req = interface_cast<IRequiredInterfaces>(GetSelf());
        return req ? req->GetRequiredInterfaces() : nullptr;
    }
    IterationResult Iterate(const IterationParameters& params) override
    {
        return iterable_->Iterate(params);
    }
    IterationResult Iterate(const IterationParameters& params) const override
    {
        return iterable_->Iterate(params);
    }

    IContent* content_ {};
    IIterable* iterable_ {};
};

META_END_NAMESPACE()

#endif
