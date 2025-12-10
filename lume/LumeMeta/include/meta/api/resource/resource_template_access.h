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

#ifndef META_API_RESOURCE_TEMPLATE_ACCESS_H
#define META_API_RESOURCE_TEMPLATE_ACCESS_H

#include <mutex>
#include <shared_mutex>

#include <meta/api/metadata_util.h>
#include <meta/ext/implementation_macros.h>
#include <meta/interface/resource/intf_object_resource.h>
#include <meta/interface/resource/intf_resource.h>

META_BEGIN_NAMESPACE()

inline CORE_NS::IResource::Ptr CreateObjectResource(const META_NS::ObjectId& instance, const META_NS::ObjectId& subid)
{
    auto res = META_NS::GetObjectRegistry().Create<CORE_NS::IResource>(META_NS::ClassId::ObjectResource);
    if (auto ores = interface_cast<META_NS::IObjectResource>(res)) {
        ores->SetResourceType(subid);
        if (auto i = interface_cast<META_NS::IMetadata>(res)) {
            META_NS::CreatePropertiesFromStaticMeta(instance, *i, false);
        }
    }
    return res;
}

class ResourceTemplateAccess : public IResourceTemplateAccess {
public:
    ResourceTemplateAccess() = default;
    ResourceTemplateAccess(const ObjectId& resourceType, const ObjectId& templateType)
        : resourceType_(resourceType), templateType_(templateType)
    {}

    CORE_NS::IResource::Ptr CreateEmptyTemplate() const override
    {
        return CreateObjectResource(resourceType_, templateType_);
    }

    CORE_NS::IResource::Ptr CreateTemplateFromResource(const CORE_NS::IResource::ConstPtr& resource) const override
    {
        CORE_NS::IResource::Ptr res;
        auto obj = interface_cast<META_NS::IObject>(resource);
        if (!obj || obj->GetClassId() != resourceType_) {
            return res;
        }
        res = GetObjectRegistry().Create<CORE_NS::IResource>(META_NS::ClassId::ObjectResource);
        if (auto ores = interface_cast<IObjectResource>(res)) {
            ores->SetResourceType(templateType_);
            if (auto i = interface_cast<IMetadata>(res)) {
                 if (auto m = interface_cast<IMetadata>(resource)) {
                    for (auto&& p : m->GetProperties()) {
                        CloneToDefault(p, *i);
                    }
                }
            }
        } else {
            CORE_LOG_W("Invalid resource given to CreateTemplateFromResource: %s",
                resource->GetResourceId().ToString().c_str());
        }
        return res;
    }

    bool SetValuesFromTemplate(
        const CORE_NS::IResource::ConstPtr& templ, const CORE_NS::IResource::Ptr& res) const override
    {
        auto obj = interface_cast<META_NS::IObject>(res);
        if (!(obj && obj->GetClassId() == resourceType_)) {
            return false;
        }
        auto ores = interface_cast<IObjectResource>(templ);
        if (!ores || templ->GetResourceType() != templateType_.ToUid()) {
            CORE_LOG_W("Invalid resource type");
            return false;
        }
        if (auto resm = interface_cast<META_NS::IMetadata>(templ)) {
            if (auto m = interface_cast<META_NS::IMetadata>(res)) {
                for (auto&& p : resm->GetProperties()) {
                    if (!m->GetMetadata(MetadataType::PROPERTY, p->GetName()).readOnly) {
                        CopyToDefaultAndReset(p, *m);
                    }
                }
                if (auto id = interface_cast<META_NS::IDerivedFromTemplate>(res)) {
                    id->SetTemplateId(templ->GetResourceId());
                }
                return true;
            }
        }
        CORE_LOG_W("Invalid resource given to SetValuesFromTemplate: %s", res->GetResourceId().ToString().c_str());
        return false;
    }

    void SetResourceType(const ObjectId& resourceType, const ObjectId& templateType)
    {
        resourceType_ = resourceType;
        templateType_ = templateType;
    }

protected:
    ObjectId resourceType_;
    ObjectId templateType_;
};

META_END_NAMESPACE()

#endif
