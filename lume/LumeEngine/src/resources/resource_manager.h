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

#ifndef CORE_RESOURCES_RESOURCE_MANAGER_H
#define CORE_RESOURCES_RESOURCE_MANAGER_H

#include <mutex>

#include <base/containers/unordered_map.h>
#include <core/plugin/intf_interface_helper.h>
#include <core/resources/intf_resource_manager.h>

CORE_BEGIN_NAMESPACE()
class IEngine;

class ResourceManager final : public IInterfaceHelper<IResourceManager> {
public:
    explicit ResourceManager(IEngine& engine);
    ~ResourceManager();

    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;

    // IResourceManager
    Status Import(BASE_NS::string_view url) override;
    BASE_NS::vector<BASE_NS::string> GetResourceUris(BASE_NS::string_view url) const override;
    BASE_NS::vector<BASE_NS::string> GetResourceUris(
        BASE_NS::string_view url, BASE_NS::Uid resourceType) const override;
    IResource::Ptr GetResource(BASE_NS::string_view url) override;
    IResource::Ptr GetResource(BASE_NS::string_view groupUri, BASE_NS::string_view uri) override;

    Status Export(BASE_NS::string_view groupUri, BASE_NS::string_view filePath) override;
    void AddResource(BASE_NS::string_view groupUri, const IResource::Ptr& resource) override;
    void RemoveResource(BASE_NS::string_view groupUri, const IResource::Ptr& resource) override;
    void RemoveResources(BASE_NS::string_view groupUri) override;

private:
    struct ImportFile;
    struct ExportData;
    ExportData GatherExportData(BASE_NS::string_view groupUri) const;

    std::mutex mutex_;
    IEngine& engine_;
    // import url to file
    BASE_NS::unordered_map<BASE_NS::string, ImportFile> importedFiles_;
    // resource uri to resource
    BASE_NS::unordered_map<BASE_NS::string, IResource::Ptr> importedResources_;
};
CORE_END_NAMESPACE()
#endif // CORE_RESOURCES_RESOURCE_MANAGER_H
