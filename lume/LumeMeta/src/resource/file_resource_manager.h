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

#ifndef META_SRC_RESOURCE_FILE_RESOURCE_MANAGER_H
#define META_SRC_RESOURCE_FILE_RESOURCE_MANAGER_H

#include <mutex>
#include <shared_mutex>
#include <string>

#include <base/containers/unordered_map.h>
#include <core/resources/intf_resource_manager.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/interface/resource/intf_owned_resource_groups.h>
#include <meta/interface/resource/intf_resource.h>
#include <meta/interface/resource/intf_resource_manager_extension.h>
#include <meta/interface/resource/intf_resource_query.h>

META_BEGIN_NAMESPACE()

using CORE_NS::ResourceContextPtr;
using CORE_NS::ResourceContextWeakPtr;
using CORE_NS::ResourceIdContext;

inline ResourceContextPtr ResolveContext(const ResourceContextPtr& p)
{
    auto i = interface_pointer_cast<IResourceContext>(p);
    return i ? ResolveContext(i->GetContext()) : p;
}
inline ResourceContextWeakPtr ResolveContext(const ResourceContextWeakPtr& p)
{
    if (p.expired()) {
        return p;
    }
    return ResolveContext(p.lock());
}

struct ResourceGroupContext {
    ResourceGroupContext(BASE_NS::string group, const ResourceContextPtr& context)
        : group(BASE_NS::move(group)), context(ResolveContext(context))
    {}
    ResourceGroupContext(BASE_NS::string group, const ResourceContextWeakPtr& context)
        : group(BASE_NS::move(group)), context(ResolveContext(context))
    {}
    explicit ResourceGroupContext(const ResourceIdContext& ric) : ResourceGroupContext(ric.id.group, ric.context)
    {}
    explicit ResourceGroupContext(const CORE_NS::IResource::Ptr& r)
        : ResourceGroupContext(r->GetResourceId().group, r->GetContext())
    {}

    bool operator==(const ResourceGroupContext& r) const
    {
        return group == r.group && context.GetRawPointer() == r.context.GetRawPointer();
    }
    bool operator!=(const ResourceGroupContext& r) const
    {
        return !(*this == r);
    }

    BASE_NS::string group;
    ResourceContextWeakPtr context;
};

inline bool IsCorrectContext(const ResourceGroupContext& gc, const ResourceContextPtr& context)
{
    return gc.context.GetRawPointer() == ResolveContext(context).get();
}

struct ResourceTypeKey {
    BASE_NS::Uid uid;
    BASE_NS::string version{};

    bool operator==(const ResourceTypeKey& o) const
    {
        return uid == o.uid && version == o.version;
    }
};

META_END_NAMESPACE()

template <>
inline uint64_t BASE_NS::hash(const META_NS::ResourceGroupContext& g)
{
    return BASE_NS::Hash(g.group, g.context.GetRawPointer());
}

template <>
inline uint64_t BASE_NS::hash(const META_NS::ResourceTypeKey& g)
{
    return BASE_NS::Hash(g.uid, g.version);
}

META_BEGIN_NAMESPACE()

class FileResourceManager : public IntroduceInterfaces<MetaObject, CORE_NS::IResourceManager, IOwnedResourceGroups,
                                IResourceQuery, IResourceManagerExtension> {
    META_OBJECT(FileResourceManager, ClassId::FileResourceManager, IntroduceInterfaces)
public:
    FileResourceManager();

    void AddListener(const IResourceListener::Ptr&) override;
    void RemoveListener(const IResourceListener::Ptr&) override;
    bool AddResourceType(CORE_NS::IResourceType::Ptr) override;
    bool RemoveResourceType(const CORE_NS::ResourceType& type, BASE_NS::string_view version) override;
    BASE_NS::vector<CORE_NS::IResourceType::Ptr> GetResourceTypes() const override;

    BASE_NS::vector<CORE_NS::ResourceInfo> GetResourceInfos(
        const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection,
        const ResourceContextPtr& context) const override;
    BASE_NS::vector<BASE_NS::string> GetResourceGroups(const ResourceContextPtr& context) const override;
    CORE_NS::ResourceInfo GetResourceInfo(const CORE_NS::ResourceIdContext&) const override;
    CORE_NS::IResource::Ptr GetResource(const CORE_NS::ResourceIdContext&) override;
    BASE_NS::vector<CORE_NS::IResource::Ptr> GetResources(
        const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection,
        const ResourceContextPtr& context) override;

    Result Import(BASE_NS::string_view url, const ResourceContextPtr& context) override;
    Result Export(BASE_NS::string_view filePath,
        const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection,
        const ResourceContextPtr& context) override;

    bool AddResource(const CORE_NS::IResource::Ptr& resource) override;
    bool AddResource(const CORE_NS::IResource::Ptr& resource, BASE_NS::string_view path) override;
    bool AddResource(const CORE_NS::ResourceIdContext&, const CORE_NS::ResourceType&, BASE_NS::string_view path,
        const CORE_NS::IResourceOptions::Ptr&) override;

    bool ReloadResource(const CORE_NS::IResource::Ptr& resource) override;
    bool RenameResource(const CORE_NS::ResourceIdContext&, const CORE_NS::ResourceIdContext&) override;
    bool PurgeResource(const CORE_NS::ResourceIdContext&) override;
    bool RemoveResource(const CORE_NS::ResourceIdContext&) override;
    size_t PurgeGroup(BASE_NS::string_view group, const ResourceContextPtr&) override;
    bool RemoveGroup(BASE_NS::string_view group, const ResourceContextPtr&) override;
    void RemoveAllResources() override;
    void RemoveAllResources(const ResourceContextPtr&) override;

    Result ExportResourcePayload(const CORE_NS::IResource::Ptr& resource) override;

    void SetFileManager(CORE_NS::IFileManager::Ptr fileManager) override;
    CORE_NS::IFileManager::Ptr GetFileManager() const override;

    IResourceGroupHandle::Ptr GetGroupHandle(BASE_NS::string_view group, const ResourceContextPtr&) override;

    uint32_t GetAliveCount(const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection,
        const ResourceContextPtr&) const override;
    BASE_NS::vector<CORE_NS::IResource::Ptr> FindAliveResources(
        const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection,
        const ResourceContextPtr&) const override;
    size_t GetResourceCount() const override;

    BASE_NS::vector<CORE_NS::ResourceIdContext> UpdateOptionsData(
        const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection, const ResourceContextPtr&) override;
    BASE_NS::vector<CORE_NS::ResourceIdContext> UpdateOptionsData(
        const BASE_NS::array_view<const CORE_NS::ResourceIdContext>& res) override;
    bool ReapplyOptions(const CORE_NS::IResource::Ptr& resource, const CORE_NS::ResourceContextPtr&) override;
    BASE_NS::vector<ResourceData> GetResources(
        const BASE_NS::array_view<const CORE_NS::MatchingResourceId>&, const ResourceContextPtr&) const override;
    ResourceData GetResource(const CORE_NS::ResourceIdContext&) const override;
    void AddResources(const BASE_NS::array_view<const ResourceData>, const ResourceContextPtr&) override;
    bool AddResource(ResourceData, const CORE_NS::ResourceContextPtr&) override;
    bool RemoveGroup(BASE_NS::string_view group, const CORE_NS::ResourceContextWeakPtr&) override;
    bool SetResourceName(const ResourceIdContext& id, const BASE_NS::string& name) override;
    CORE_NS::IResource::Ptr GetResourceByName(BASE_NS::string_view name, const ResourceContextPtr& context) override;

private:
    CORE_NS::IResource::Ptr ConstructResource(const CORE_NS::ResourceIdContext& id);
    Result SaveResourceData(const ResourceData& r);
    CORE_NS::ResourceIdContext ReadHeader(
        const std::string& line, const ResourceContextPtr& context, BASE_NS::vector<CORE_NS::IResource::Ptr>& destroy);
    bool ReadHeaders(CORE_NS::IFile& file, const ResourceContextPtr& context,
        BASE_NS::vector<CORE_NS::ResourceIdContext>& result, BASE_NS::vector<CORE_NS::IResource::Ptr>& destroy);
    BASE_NS::shared_ptr<ResourceData> FindResource(const ResourceIdContext& id);
    bool UpdateOptions(ResourceData& r, CORE_NS::ResourceContextPtr context = nullptr) const;
    void Notify(const BASE_NS::vector<ResourceIdContext>& id, IResourceListener::EventType type);
    void UpdateOptionsData(const BASE_NS::unordered_map<BASE_NS::string, BASE_NS::shared_ptr<ResourceData>>& data,
        const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection, const IObject::Ptr& depsContext);
    CORE_NS::IResource::Ptr GetResourceFromGroup(
        const BASE_NS::unordered_map<BASE_NS::string, BASE_NS::shared_ptr<ResourceData>>& resources,
        BASE_NS::string_view name, const ResourceContextPtr& context);

private:
    mutable std::shared_mutex mutex_;
    BASE_NS::unordered_map<ResourceTypeKey, CORE_NS::IResourceType::Ptr> types_;
    BASE_NS::unordered_map<ResourceGroupContext,
        BASE_NS::unordered_map<BASE_NS::string, BASE_NS::shared_ptr<ResourceData>>>
        resources_;
    CORE_NS::IFileManager::Ptr fileManager_;
    mutable std::shared_mutex listenerMutex_;
    BASE_NS::vector<IResourceListener::WeakPtr> listeners_;
    BASE_NS::unordered_map<ResourceGroupContext, IResourceGroupHandle::WeakPtr> ownedGroups_;
};

META_END_NAMESPACE()

#endif
