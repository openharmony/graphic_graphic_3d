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

#ifndef API_CORE_RESOURCES_IRESOURCE_H
#define API_CORE_RESOURCES_IRESOURCE_H

#include <base/containers/shared_ptr.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <core/io/intf_file.h>
#include <core/plugin/intf_interface.h>

CORE_BEGIN_NAMESPACE()

/**
 * @brief Type to identify resource
 */
struct ResourceId {
    BASE_NS::string name;
    BASE_NS::string group;

    ResourceId() = default;
    ResourceId(const char* name) : name(name) {}
    ResourceId(BASE_NS::string name) : name(BASE_NS::move(name)) {}
    ResourceId(BASE_NS::string name, BASE_NS::string group) : name(BASE_NS::move(name)), group(BASE_NS::move(group)) {}

    bool IsValid() const
    {
        return !name.empty();
    }

    BASE_NS::string ToString() const
    {
        if (group.empty()) {
            return name;
        }
        return group + "::" + name;
    }

    bool operator==(const ResourceId& id) const
    {
        return name == id.name && group == id.group;
    }
    bool operator!=(const ResourceId& id) const
    {
        return !(*this == id);
    }
};
using ResourceType = BASE_NS::Uid;

/**
 * @brief Type that is used to select resource ids
 */
struct MatchingResourceId {
    /// Match all names and groups
    MatchingResourceId() : hasWildCardName(true), hasWildCardGroup(true) {}
    /// Match all names in given group
    MatchingResourceId(BASE_NS::string group) : hasWildCardName(true), group(BASE_NS::move(group)) {}
    MatchingResourceId(BASE_NS::string_view group) : hasWildCardName(true), group(group) {}
    /// Match specific resource
    MatchingResourceId(const ResourceId& id) : name(id.name), group(id.group) {}

    bool hasWildCardName {};
    BASE_NS::string name;
    bool hasWildCardGroup {};
    BASE_NS::string group;
};

/**
 * @brief Base class for resources
 */
class IResource : public IInterface {
public:
    using base = IInterface;
    static constexpr BASE_NS::Uid UID { "597e8395-4990-492c-9cac-ad32804c0dfd" };
    using Ptr = BASE_NS::shared_ptr<IResource>;
    using ConstPtr = BASE_NS::shared_ptr<const IResource>;
    using WeakPtr = BASE_NS::weak_ptr<IResource>;
    using ConstWeakPtr = BASE_NS::weak_ptr<const IResource>;

    /** Get the resource's type UID.
     * @return Type UID.
     */
    virtual ResourceType GetResourceType() const = 0;

    /** Get the resource's id. A resource can be queried from the resource manager with the same id.
     * @return Resource id.
     */
    virtual ResourceId GetResourceId() const = 0;

protected:
    IResource() = default;
    virtual ~IResource() = default;
    IResource(const IResource&) = delete;
    void operator=(const IResource&) = delete;
};

/**
 * @brief Interface to set resource id for existing resource object
 */
class ISetResourceId : public IInterface {
public:
    using base = IInterface;
    static constexpr BASE_NS::Uid UID { "1d2aa2c6-7a08-49bf-a360-aecf201a8681" };
    using Ptr = BASE_NS::shared_ptr<ISetResourceId>;
    using ConstPtr = BASE_NS::shared_ptr<const ISetResourceId>;
    using WeakPtr = BASE_NS::weak_ptr<ISetResourceId>;
    using ConstWeakPtr = BASE_NS::weak_ptr<const ISetResourceId>;

    virtual void SetResourceId(CORE_NS::ResourceId) = 0;

protected:
    ISetResourceId() = default;
    virtual ~ISetResourceId() = default;
    ISetResourceId(const ISetResourceId&) = delete;
    void operator=(const ISetResourceId&) = delete;
};

class IResourceManager;
using ResourceContextPtr = BASE_NS::shared_ptr<IInterface>;
using ResourceManagerPtr = BASE_NS::shared_ptr<IResourceManager>;

/**
 * @brief Base class for options that can be used to construct resources
 */
class IResourceOptions : public IInterface {
public:
    using base = IInterface;
    static constexpr BASE_NS::Uid UID { "c523171f-c6b8-40a2-939a-eb10672bb87c" };
    using Ptr = BASE_NS::shared_ptr<IResourceOptions>;
    using ConstPtr = BASE_NS::shared_ptr<const IResourceOptions>;
    using WeakPtr = BASE_NS::weak_ptr<IResourceOptions>;
    using ConstWeakPtr = BASE_NS::weak_ptr<const IResourceOptions>;

    virtual bool Load(IFile& options, const ResourceManagerPtr&, const ResourceContextPtr&) = 0;
    virtual bool Save(IFile& options, const ResourceManagerPtr&) const = 0;

protected:
    IResourceOptions() = default;
    virtual ~IResourceOptions() = default;
    IResourceOptions(const IResourceOptions&) = delete;
    void operator=(const IResourceOptions&) = delete;
};

CORE_END_NAMESPACE()

template<>
inline uint64_t BASE_NS::hash(const CORE_NS::ResourceId& id)
{
    return BASE_NS::Hash(id.name, id.group);
}

#endif // API_CORE_RESOURCES_IRESOURCE_H
