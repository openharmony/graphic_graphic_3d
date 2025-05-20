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
#include <meta/interface/static_object_metadata.h>

META_BEGIN_NAMESPACE()

/// Get the aggregate baseclass' metadata
inline const StaticObjectMetadata* GetBaseClassMeta(const StaticObjectMetadata* d)
{
    while (d && !d->aggregate) {
        d = d->baseclass;
    }
    return d ? d->aggregate : nullptr;
}

/// Get the aggregate baseclass' metadata
inline const StaticObjectMetadata* GetBaseClassMeta(const IObject::ConstPtr& obj)
{
    auto m = interface_cast<IStaticMetadata>(obj);
    return m ? GetBaseClassMeta(m->GetStaticMetadata()) : nullptr;
}

/// Get the aggregate baseclass' object id
inline ObjectId GetBaseClass(const IObject::ConstPtr& obj)
{
    if (auto m = GetBaseClassMeta(obj)) {
        if (m->classInfo) {
            return m->classInfo->Id();
        }
    }
    return {};
}

/// Search static metadata in class metadata, not considering aggregate bases
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

/**
 * @brief Find static metadata for given name and type
 * @param data Metadata to search from
 * @param name Name of the entity to search metadata for
 * @param type Type of the entity to search, this contains bit field of the possible types
 * @notice Returns the first matching metadata
 * @return Static metadata for matching entity.
 */
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
struct MetadataObject {
    typename Interface::Ptr object {};
    bool isForward {};
};

/// Construct entity from given metadata
template<typename Interface>
MetadataObject<Interface> ConstructFromMetadata(const IOwner::Ptr& self, const StaticMetadata& pm)
{
    auto res = pm.create(self, pm);
    if (!res) {
        CORE_LOG_W("Failed to create entity from static metadata [name=%s]", pm.name);
    }
    return MetadataObject<Interface> { interface_pointer_cast<Interface>(res),
        bool(pm.flags & uint8_t(Internal::StaticMetaFlag::FORWARD)) };
}

/// Find and construct entity with given criteria starting search from static metadata
template<typename Interface>
MetadataObject<Interface> ConstructFromMetadata(
    const IOwner::Ptr& self, const StaticObjectMetadata& sm, BASE_NS::string_view name, MetadataType type)
{
    if (auto pm = FindStaticMetadata(sm, name, type)) {
        return ConstructFromMetadata<Interface>(self, *pm);
    }
    return {};
}

/// Find and construct entity with given criteria for object
template<typename Interface>
MetadataObject<Interface> ConstructFromMetadata(const IOwner::Ptr& self, BASE_NS::string_view name, MetadataType type)
{
    if (auto s = interface_cast<IStaticMetadata>(self)) {
        if (auto pm = s->GetStaticMetadata()) {
            return ConstructFromMetadata<Interface>(self, *pm, name, type);
        }
    }
    return {};
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

/// Get all static metadata recursively without considering aggregate bases
inline BASE_NS::vector<MetadataInfo> GetAllStaticMetadataInClass(const StaticObjectMetadata& data, MetadataType type)
{
    BASE_NS::vector<MetadataInfo> result;
    for (size_t i = 0; i != data.size; ++i) {
        auto& p = data.metadata[i];
        if (uint8_t(p.type) & uint8_t(type)) {
            MetadataInfo info { p.type, p.name, p.interfaceInfo };
            if (p.type == MetadataType::PROPERTY) {
                info.propertyType = GetMetaPropertyType(p);
                info.readOnly = p.flags & static_cast<uint8_t>(Internal::PropertyFlag::READONLY);
            }
            info.data = &p;
            result.push_back(info);
        }
    }
    if (data.baseclass) {
        auto r = GetAllStaticMetadataInClass(*data.baseclass, type);
        result.insert(result.end(), r.begin(), r.end());
    }
    return result;
}

/// Get all static metadata recursively
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
