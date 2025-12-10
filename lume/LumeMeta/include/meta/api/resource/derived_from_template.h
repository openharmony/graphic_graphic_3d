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

#ifndef META_API_RESOURCE_DERIVED_FROM_TEMPLATE_H
#define META_API_RESOURCE_DERIVED_FROM_TEMPLATE_H

#include <mutex>
#include <shared_mutex>

#include <meta/api/metadata_util.h>
#include <meta/ext/implementation_macros.h>
#include <meta/interface/resource/intf_object_resource.h>
#include <meta/interface/resource/intf_resource.h>

META_BEGIN_NAMESPACE()

class DerivedFromTemplate : public IDerivedFromTemplate {
public:
    bool SetTemplateId(CORE_NS::ResourceId id) override
    {
        std::unique_lock lock { mutex_ };
        resource_ = BASE_NS::move(id);
        return true;
    }
    CORE_NS::ResourceId GetTemplateId() const override
    {
        std::shared_lock lock { mutex_ };
        return resource_;
    }

    virtual IResourceTemplateAccess::Ptr ConstructDefaultAccess() const
    {
        return META_NS::GetObjectRegistry().Create<IResourceTemplateAccess>(GetDefaultAccess());
    }

    CORE_NS::IResource::Ptr CreateEmptyTemplate() const override
    {
        auto p = ConstructDefaultAccess();
        return p ? p->CreateEmptyTemplate() : nullptr;
    }
    CORE_NS::IResource::Ptr CreateTemplate() const override
    {
        CORE_NS::IResource::Ptr res;
        if (auto p = ConstructDefaultAccess()) {
            if (auto obj = interface_cast<META_NS::IObjectInstance>(this)) {
                if (auto self = obj->GetSelf<CORE_NS::IResource>()) {
                    res = p->CreateTemplateFromResource(self);
                }
            }
        }
        return res;
    }
    bool SetTemplate(const CORE_NS::IResource::ConstPtr& templ) const override
    {
        bool res = false;
        if (auto p = ConstructDefaultAccess()) {
            if (auto obj = interface_cast<META_NS::IObjectInstance>(this)) {
                if (auto self = obj->GetSelf<CORE_NS::IResource>()) {
                    res = p->SetValuesFromTemplate(templ, self);
                }
            }
        }
        return res;
    }

protected:
    mutable std::shared_mutex mutex_;
    CORE_NS::ResourceId resource_;
};

META_END_NAMESPACE()

#endif
