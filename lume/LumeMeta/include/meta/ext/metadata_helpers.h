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

#ifndef META_EXT_METADATA_HELPERS_H
#define META_EXT_METADATA_HELPERS_H

#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_owner.h>
#include <meta/interface/property/intf_property_internal.h>
#include <meta/interface/static_object_metadata.h>

META_BEGIN_NAMESPACE()

inline const StaticObjectMetadata* GetBaseClassMeta(const StaticObjectMetadata* d)
{
    while (d && !d->aggregate) {
        d = d->baseclass;
    }
    return d ? d->aggregate : nullptr;
}

inline const StaticObjectMetadata* GetBaseClassMeta(const IObject::ConstPtr& obj)
{
    auto m = interface_cast<IStaticMetadata>(obj);
    return m ? GetBaseClassMeta(m->GetStaticMetadata()) : nullptr;
}

inline ObjectId GetBaseClass(const IObject::ConstPtr& obj)
{
    if (auto m = GetBaseClassMeta(obj)) {
        if (m->classInfo) {
            return m->classInfo->Id();
        }
    }
    return {};
}

inline const StaticMetadata* FindStaticMetadataInClass(
    const StaticObjectMetadata& data, BASE_NS::string_view name, MetadataType type)
{
    for (size_t i = 0; i != data.size; ++i) {
        auto& p = data.metadata[i];
        if (p.name == name && ((uint8_t(p.type) & uint8_t(type)) || type == MetadataType::UNKNOWN)) {
            return &p;
        }
    }
    return data.baseclass ? FindStaticMetadataInClass(*data.baseclass, name, type) : nullptr;
        }
inline const StaticMetadata* FindStaticMetadata(
    const StaticObjectMetadata& data, BASE_NS::string_view name, MetadataType type)
{
    if (auto ret = FindStaticMetadataInClass(data, name, type)) {
        return ret;
    }
    auto base = GetBaseClassMeta(&data);
    return base ? FindStaticMetadata(*base, name, type) : nullptr;
}

template<typename Interface>
auto ConstructFromMetadata(const IOwner::Ptr& self, const StaticMetadata& pm)
{
    auto res = pm.create(self, pm);
    if (!res) {
        CORE_LOG_W("Failed to create entity from static metadata [name=%s]", pm.name);
    }
    return interface_pointer_cast<Interface>(res);
}

template<typename Interface>
auto ConstructFromMetadata(
    const IOwner::Ptr& self, const StaticObjectMetadata& sm, BASE_NS::string_view name, MetadataType type)
{
    if (auto pm = FindStaticMetadata(sm, name, type)) {
        return ConstructFromMetadata<Interface>(self, *pm);
    }
    return typename Interface::Ptr {};
}

template<typename Interface>
auto ConstructFromMetadata(const IOwner::Ptr& self, BASE_NS::string_view name, MetadataType type)
{
    if (auto s = interface_cast<IStaticMetadata>(self)) {
        if (auto pm = s->GetStaticMetadata()) {
            return ConstructFromMetadata<Interface>(self, *pm, name, type);
        }
    }
    return typename Interface::Ptr {};
}

inline auto ConstructPropertyFromMetadata(const IOwner::Ptr& self, BASE_NS::string_view name)
{
    auto p = ConstructFromMetadata<IProperty>(self, name, MetadataType::PROPERTY);
    if (auto pp = interface_cast<IPropertyInternal>(p)) {
        if (auto s = interface_pointer_cast<IObjectInstance>(self)) {
            pp->SetOwner(s);
        }
    }
    return p;
}

inline TypeId GetMetaPropertyType(const StaticMetadata& m)
{
    if (m.type == MetadataType::PROPERTY && m.runtimeValue) {
        if (auto def = m.runtimeValue()) {
            return def->GetTypeId();
        }
    }
    return {};
}
inline BASE_NS::vector<MetadataInfo> GetAllStaticMetadataInClass(const StaticObjectMetadata& data, MetadataType type)
{
    BASE_NS::vector<MetadataInfo> result;
    for (size_t i = 0; i != data.size; ++i) {
        auto& p = data.metadata[i];
        if (uint8_t(p.type) & uint8_t(type)) {
            MetadataInfo info { p.type, p.name };
            info.propertyType = GetMetaPropertyType(p);
            result.push_back(info);
        }
    }
    if (data.baseclass) {
        auto r = GetAllStaticMetadataInClass(*data.baseclass, type);
        result.insert(result.end(), r.begin(), r.end());
    }
    return result;
}
inline BASE_NS::vector<MetadataInfo> GetAllStaticMetadata(const StaticObjectMetadata& data, MetadataType type)
{
    auto result = GetAllStaticMetadataInClass(data, type);
    if (auto base = GetBaseClassMeta(&data)) {
        auto r = GetAllStaticMetadata(*base, type);
        result.insert(result.end(), r.begin(), r.end());
    }
    return result;
}
META_END_NAMESPACE()

#endif
