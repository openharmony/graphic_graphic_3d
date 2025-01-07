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

#ifndef API_CORE_RESOURCES_IRESOURCE_MANAGER_H
#define API_CORE_RESOURCES_IRESOURCE_MANAGER_H

#include <base/containers/string_view.h>
#include <core/io/intf_file_system.h>
#include <core/plugin/intf_interface.h>
#include <core/plugin/intf_plugin.h>
#include <core/resources/intf_resource.h>

CORE_BEGIN_NAMESPACE()
class IResourceManager : public IInterface {
public:
    static constexpr BASE_NS::Uid UID { "6f9f705d-7f9f-4b5c-83ab-45bb2466d974" };

    enum class Status {
        STATUS_OK,
        /** URL didn't point to a accessible resource file. */
        STATUS_FILE_NOT_FOUND,
        /** Failed to read file. */
        STATUS_FILE_READ_ERROR,
        /** Failed to write to file. */
        STATUS_FILE_WRITE_ERROR,
        /** Resource file is invalid. */
        STATUS_INVALID_FILE,
    };

    /** Imports a resource file into the resource manager. Available resources are grouped based on the file URL.
     * @param url URL of the file to import.
     * @return true if the file could be successfully imported.
     */
    virtual Status Import(BASE_NS::string_view url) = 0;

    /** Get the URIs of the resources included in a resource file.
     * @param url URL of a previously imported resource file.
     * @return List of resource URIs available in the resource file pointed by URL.
     */
    virtual BASE_NS::vector<BASE_NS::string> GetResourceUris(BASE_NS::string_view url) const = 0;

    /** Get the URIs of the resources of a certain type included in a resource file.
     * @param url URL of a previously imported resource file.
     * @param resourceType Resource type UID.
     * @return List of resource URIs with a matching type available in the resource file pointed by URL.
     */
    virtual BASE_NS::vector<BASE_NS::string> GetResourceUris(
        BASE_NS::string_view url, BASE_NS::Uid resourceType) const = 0;

    /** Get a resource matching the URL. The URL is a combination of a previously imported resource file and the URI of
     * a resource in that file. The same URL will return a pointer to the same instance.
     * @param url URL of the resource to import.
     * @return Pointer to the resource if one matched the given URL.
     */
    virtual IResource::Ptr GetResource(BASE_NS::string_view url) = 0;

    /** Get a resource matching the URI. The same URI will return a pointer to the same instance.
     * @param groupUri URI which is used to group resources.
     * @param uri URI of a resource in the resource file.
     * @return Pointer to the resource if one matched the given URI.
     */
    virtual IResource::Ptr GetResource(BASE_NS::string_view groupUri, BASE_NS::string_view uri) = 0;

    /** Exports all resources added to a group into a resource file. If the target URL doesn't resolve into an
     * existing file the file will be created. If the target URL resolves into an existing file the file contents will
     * be overwritten.
     * @param groupUri URI which is used to select group of resources to export.
     * @param filePath File to write to.
     * @return true if the file could be successfully exported.
     */
    virtual Status Export(BASE_NS::string_view groupUri, BASE_NS::string_view filePath) = 0;

    /** Add a resource into the resource manager.
     * @param groupUri URI which is used to group resources.
     * @param resource Resource instance to add.
     */
    virtual void AddResource(BASE_NS::string_view groupUri, const IResource::Ptr& resource) = 0;

    /** Remove a resource from resource manager.
     * @param groupUri URI of the resource group from which the resource is removed.
     * @param resource Resource instance to remove.
     */
    virtual void RemoveResource(BASE_NS::string_view groupUri, const IResource::Ptr& resource) = 0;

    /** Remove a group of resources from resource manager.
     * @param groupUri URI of the resource group to remove.
     */
    virtual void RemoveResources(BASE_NS::string_view groupUri) = 0;

protected:
    IResourceManager() = default;
    virtual ~IResourceManager() = default;

    IResourceManager(const IResourceManager&) = delete;
    void operator=(const IResourceManager&) = delete;
};
CORE_END_NAMESPACE()
#endif // API_CORE_RESOURCES_IRESOURCE_MANAGER_H
