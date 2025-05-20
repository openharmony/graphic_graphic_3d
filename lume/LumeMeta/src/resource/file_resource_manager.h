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

META_BEGIN_NAMESPACE()

struct ResourceData : CORE_NS::ResourceInfo {
    CORE_NS::IResource::Ptr object;
};

class FileResourceManager : public IntroduceInterfaces<MetaObject, CORE_NS::IResourceManager> {
    META_OBJECT(FileResourceManager, ClassId::FileResourceManager, IntroduceInterfaces)
public:
    FileResourceManager();

    void AddListener(const IResourceListener::Ptr&) override;
    void RemoveListener(const IResourceListener::Ptr&) override;
    bool AddResourceType(CORE_NS::IResourceType::Ptr) override;
    bool RemoveResourceType(const CORE_NS::ResourceType& type) override;
    BASE_NS::vector<CORE_NS::IResourceType::Ptr> GetResourceTypes() const override;
    Result Import(BASE_NS::string_view url) override;
    BASE_NS::vector<CORE_NS::ResourceInfo> GetResourceInfos(
        const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection) const override;
    BASE_NS::vector<BASE_NS::string> GetResourceGroups() const override;
    CORE_NS::ResourceInfo GetResourceInfo(const CORE_NS::ResourceId&) const override;
    CORE_NS::IResource::Ptr GetResource(
        const CORE_NS::ResourceId&, const BASE_NS::shared_ptr<CORE_NS::IInterface>& context) override;
    BASE_NS::vector<CORE_NS::IResource::Ptr> GetResources(
        const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection,
        const BASE_NS::shared_ptr<CORE_NS::IInterface>& context) override;
    Result Export(BASE_NS::string_view filePath,
        const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection) override;

    bool AddResource(const CORE_NS::IResource::Ptr& resource) override;
    bool AddResource(const CORE_NS::IResource::Ptr& resource, BASE_NS::string_view path) override;
    bool AddResource(const CORE_NS::ResourceId&, const CORE_NS::ResourceType&, BASE_NS::string_view path,
        const CORE_NS::IResourceOptions::ConstPtr&) override;

    bool ReloadResource(
        const CORE_NS::IResource::Ptr& resource, const BASE_NS::shared_ptr<CORE_NS::IInterface>& context) override;
    bool RenameResource(const CORE_NS::ResourceId&, const CORE_NS::ResourceId&) override;
    bool PurgeResource(const CORE_NS::ResourceId&) override;
    bool RemoveResource(const CORE_NS::ResourceId&) override;
    size_t PurgeGroup(BASE_NS::string_view group) override;
    bool RemoveGroup(BASE_NS::string_view group) override;
    void RemoveAllResources() override;

    Result ExportResourcePayload(const CORE_NS::IResource::Ptr& resource) override;

    void SetFileManager(CORE_NS::IFileManager::Ptr fileManager) override;

private:
    CORE_NS::IResource::Ptr ConstructResource(
        const CORE_NS::ResourceId& id, const BASE_NS::shared_ptr<CORE_NS::IInterface>& context);
    Result SaveResourceData(const ResourceData& r);
    CORE_NS::ResourceId ReadHeader(const std::string& line);
    bool ReadHeaders(CORE_NS::IFile& file, BASE_NS::vector<CORE_NS::ResourceId>& result);
    BASE_NS::shared_ptr<ResourceData> FindResource(const CORE_NS::ResourceId& id);
    bool UpdateOptions(ResourceData& r);
    void Notify(const BASE_NS::vector<CORE_NS::ResourceId>& id, IResourceListener::EventType type);

private:
    mutable std::shared_mutex mutex_;
    BASE_NS::unordered_map<BASE_NS::Uid, CORE_NS::IResourceType::Ptr> types_;
    BASE_NS::unordered_map<BASE_NS::string, BASE_NS::unordered_map<BASE_NS::string, BASE_NS::shared_ptr<ResourceData>>>
        resources_;
    CORE_NS::IFileManager::Ptr fileManager_;
    mutable std::shared_mutex listenerMutex_;
    BASE_NS::vector<IResourceListener::WeakPtr> listeners_;
};

META_END_NAMESPACE()

#endif
