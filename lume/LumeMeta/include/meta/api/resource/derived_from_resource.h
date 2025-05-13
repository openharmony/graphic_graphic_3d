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

#ifndef META_API_RESOURCE_DERIVED_FROM_RESOURCE_H
#define META_API_RESOURCE_DERIVED_FROM_RESOURCE_H

#include <mutex>
#include <shared_mutex>

#include <meta/api/metadata_util.h>
#include <meta/ext/implementation_macros.h>
#include <meta/interface/resource/intf_object_resource.h>
#include <meta/interface/resource/intf_resource.h>

META_BEGIN_NAMESPACE()

class DerivedFromResource : public IDerivedFromResource {
public:
    explicit DerivedFromResource(const ObjectId& id = {}) : resourceType_(id) {}

    bool SetResource(const CORE_NS::IResource::Ptr& p) override
    {
        auto ores = interface_cast<IObjectResource>(p);
        if (!ores || p->GetResourceType() != resourceType_.ToUid()) {
            CORE_LOG_W("Invalid resource type");
            return false;
        }
        if (auto resm = interface_cast<META_NS::IMetadata>(p)) {
            if (auto m = interface_cast<META_NS::IMetadata>(this)) {
                for (auto&& p : resm->GetProperties()) {
                    CopyToDefaultAndReset(p, *m);
                }
            }
        }
        std::unique_lock lock { mutex_ };
        resource_ = p->GetResourceId();
        return true;
    }
    CORE_NS::ResourceId GetResource() const override
    {
        std::shared_lock lock { mutex_ };
        return resource_;
    }

    // static void CloneToDefaultWithoutReadonly(const IMetadata& in, IMetadata& out)
    // {
    //     for (auto&& p : in.GetProperties()) {
    //         if (!in.GetMetadata(META_NS::MetadataType::PROPERTY, p->GetName()).readOnly) {
    //             CloneToDefault(p, out);
    //         }
    //     }
    // }

    CORE_NS::IResource::Ptr CreateResource() const override
    {
        auto res = GetObjectRegistry().Create<CORE_NS::IResource>(META_NS::ClassId::ObjectResource);
        if (auto ores = interface_cast<IObjectResource>(res)) {
            ores->SetResourceType(resourceType_);
            if (auto i = interface_cast<IMetadata>(res)) {
                if (auto m = interface_cast<IMetadata>(this)) {
                    // CloneToDefaultWithoutReadonly(*m, *i);
                    CloneToDefault(*m, *i);
                }
            }
        }
        return res;
    }

    void SetResourceType(const ObjectId& id)
    {
        resourceType_ = id;
    }

protected:
    mutable std::shared_mutex mutex_;
    CORE_NS::ResourceId resource_;
    ObjectId resourceType_;
};

inline CORE_NS::IResource::Ptr CreateObjectResource(const META_NS::ObjectId& instance, const META_NS::ObjectId& subid)
{
    auto res = META_NS::GetObjectRegistry().Create<CORE_NS::IResource>(META_NS::ClassId::ObjectResource);
    if (auto ores = interface_cast<META_NS::IObjectResource>(res)) {
        ores->SetResourceType(subid);
        if (auto i = interface_cast<META_NS::IMetadata>(res)) {
            META_NS::CreatePropertiesFromStaticMeta(instance, *i, true);
        }
    }
    return res;
}

META_END_NAMESPACE()

#endif
