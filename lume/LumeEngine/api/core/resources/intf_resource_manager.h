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

#include <base/containers/shared_ptr.h>
#include <base/containers/string_view.h>
#include <core/io/intf_file_manager.h>
#include <core/io/intf_file_system.h>
#include <core/plugin/intf_interface.h>
#include <core/plugin/intf_plugin.h>
#include <core/resources/intf_resource.h>

CORE_BEGIN_NAMESPACE()

/**
 * @brief Base class for defining loading and saving for resource type
 */
class IResourceType : public IInterface {
public:
    using base = IInterface;
    static constexpr BASE_NS::Uid UID { "f8553f78-a286-4d5b-9b63-a3aa13477f82" };
    using Ptr = BASE_NS::shared_ptr<IResourceType>;
    using ConstPtr = BASE_NS::shared_ptr<const IResourceType>;

    /// Get name of the resource this object can load/save
    virtual BASE_NS::string GetResourceName() const = 0;
    /// Get type of the resource this object can load/save
    virtual ResourceType GetResourceType() const = 0;

    struct StorageInfo {
        /// Pointer to file containing the options data.
        IFile* options {};
        /// Pointer to file containing the payload data.
        IFile* payload {};
        /// The resource id.
        const ResourceId& id;
        /// Path of the resource payload.
        BASE_NS::string_view path;
        /// Resource manager that is loading/saving the resource.
        ResourceManagerPtr self;
        /// User provided context for context specific resource types.
        ResourceContextPtr context;
    };

    /**
     * @brief Construct resource from storage information
     */
    virtual IResource::Ptr LoadResource(const StorageInfo&) const = 0;
    /**
     * @brief Save resource payload and/or options
     * Note: either options or payload pointers can be null if system is only asking
     *       to save one aspect of the resource
     */
    virtual bool SaveResource(const IResource::ConstPtr&, const StorageInfo&) const = 0;
    /**
     * @brief Reload resource by keeping the same object
     */
    virtual bool ReloadResource(const StorageInfo&, const IResource::Ptr&) const = 0;

protected:
    IResourceType() = default;
    virtual ~IResourceType() = default;
    IResourceType(const IResourceType&) = delete;
    void operator=(const IResourceType&) = delete;
};

/**
 * @brief Resource information
 */
struct ResourceInfo {
    /// Id of the resource
    CORE_NS::ResourceId id;
    /// Type of the resource
    CORE_NS::ResourceType type;
    /// Path of the resource payload if exists
    BASE_NS::string path;
    /// Option of the resource as serialised data
    BASE_NS::vector<uint8_t> optionData;
};

/**
 * @brief Resource loading and saving
 */
class IResourceManager : public IInterface {
public:
    using base = IInterface;
    static constexpr BASE_NS::Uid UID { "6f9f705d-7f9f-4b5c-83ab-45bb2466d974" };
    using Ptr = BASE_NS::shared_ptr<IResourceManager>;

    enum class Result {
        OK,
        /** URL didn't point to a accessible resource file. */
        FILE_NOT_FOUND,
        /** Failed to read file. */
        FILE_READ_ERROR,
        /** Failed to write to file. */
        FILE_WRITE_ERROR,
        /** Resource file is invalid. */
        INVALID_FILE,
        /** No such resource type. */
        NO_RESOURCE_TYPE,
        /** Resource data is missing. */
        NO_RESOURCE_DATA,
        /** Exporting failed. */
        EXPORT_FAILURE
    };

    /** Interface for observing adding and removing resources. */
    class IResourceListener {
    public:
        using Ptr = BASE_NS::shared_ptr<IResourceListener>;
        using WeakPtr = BASE_NS::weak_ptr<IResourceListener>;

        enum class EventType {
            /** Resource has been added. */
            ADDED,
            /** Resource was removed. */
            REMOVED,
        };
        /** Called when resources are added or removed.
         * @param type Event type.
         * @param ids Resource ids.
         */
        virtual void OnResourceEvent(EventType type, const BASE_NS::array_view<const ResourceId>& ids) = 0;

    protected:
        IResourceListener() = default;
        virtual ~IResourceListener() = default;
    };

    /**
     * @brief Add listener to find out when resources are added and removed
     */
    virtual void AddListener(const IResourceListener::Ptr&) = 0;
    /**
     * @brief Remove previously added listener
     */
    virtual void RemoveListener(const IResourceListener::Ptr&) = 0;
    /**
     * @brief Add resource type that is responsible loading and saving resources for specific type
     */
    virtual bool AddResourceType(IResourceType::Ptr) = 0;
    /**
     * @brief Remove resource type
     */
    virtual bool RemoveResourceType(const ResourceType& type) = 0;
    /**
     * @brief Get list of all resource types supported by with manager
     */
    virtual BASE_NS::vector<IResourceType::Ptr> GetResourceTypes() const = 0;
    /**
     * @brief Imports a resource file into the resource manager.
     * @note  The imported index is merged with the current one, already existing resource info is overwritten.
     * @return Result of the import process
     */
    virtual Result Import(BASE_NS::string_view url) = 0;
    /**
     * @brief Get the resource infos for all resources or for specific group
     */
    virtual BASE_NS::vector<ResourceInfo> GetResourceInfos(
        const BASE_NS::array_view<const MatchingResourceId>& selection) const = 0;
    BASE_NS::vector<ResourceInfo> GetResourceInfos(BASE_NS::string_view group) const
    {
        return GetResourceInfos({ MatchingResourceId(group) });
    }
    BASE_NS::vector<ResourceInfo> GetResourceInfos() const
    {
        return GetResourceInfos({ MatchingResourceId() });
    }
    /**
     * @brief Get resource info for single resource
     */
    virtual ResourceInfo GetResourceInfo(const ResourceId&) const = 0;
    /**
     * @brief Get all resource groups
     */
    virtual BASE_NS::vector<BASE_NS::string> GetResourceGroups() const = 0;
    /**
     * @brief Get a resource matching the resource id.
     * @param id resource id.
     * @param context resource context for resources that can only exists in specific context (required to construct
     * such resources).
     * @return Resource
     */
    virtual IResource::Ptr GetResource(const ResourceId& id, const ResourceContextPtr& context) = 0;
    IResource::Ptr GetResource(const ResourceId& id)
    {
        return GetResource(id, nullptr);
    }
    /**
     * @brief Get all resources in a group
     * @param selection resources to return.
     * @param context resource context for resources that can only exists in specific context (required to construct
     * such resources).
     */
    virtual BASE_NS::vector<CORE_NS::IResource::Ptr> GetResources(
        const BASE_NS::array_view<const MatchingResourceId>& selection, const ResourceContextPtr& context) = 0;
    BASE_NS::vector<CORE_NS::IResource::Ptr> GetResources(
        BASE_NS::string_view group, const ResourceContextPtr& context = nullptr)
    {
        return GetResources({ MatchingResourceId(group) }, context);
    }
    BASE_NS::vector<CORE_NS::IResource::Ptr> GetResources()
    {
        return GetResources({ MatchingResourceId() }, nullptr);
    }
    /** Exports selected resources into a resource file.
     * @param filePath File to write to.
     * @param selection Selected resources
     * @return Result of the export process
     */
    virtual Result Export(
        BASE_NS::string_view filePath, const BASE_NS::array_view<const MatchingResourceId>& selection) = 0;
    virtual Result Export(BASE_NS::string_view filePath)
    {
        return Export(filePath, { MatchingResourceId {} });
    }

    /** Add a resource into the resource manager.
     * @param resource Resource object to add
     * @return true on success
     */
    virtual bool AddResource(const IResource::Ptr& resource) = 0;
    /** Add a resource into the resource manager.
     * @param resource Resource object to add
     * @param path Path of the payload
     * @return true on success
     */
    virtual bool AddResource(const IResource::Ptr& resource, BASE_NS::string_view path) = 0;
    /** Add a resource into the resource manager.
     * @param id Resource id
     * @param type Resource type
     * @param path Path to the resource payload
     * @param opts Resource options
     * @return true on success
     */
    virtual bool AddResource(const ResourceId& id, const ResourceType& type, BASE_NS::string_view path,
        const IResourceOptions::ConstPtr& opts) = 0;
    bool AddResource(const ResourceId& id, const ResourceType& type, BASE_NS::string_view path)
    {
        return AddResource(id, type, path, nullptr);
    }
    /**
     * @brief Reload resource
     * @param resource Resource to be reloaded
     * @param context resource context for resources that can only exists in specific context (required to construct
     * such resources).
     * @return true on success
     */
    virtual bool ReloadResource(const IResource::Ptr& resource, const ResourceContextPtr& context) = 0;
    bool ReloadResource(const IResource::Ptr& resource)
    {
        return ReloadResource(resource, nullptr);
    }
    /**
     * @brief Rename resource
     * @param id Resource to rename
     * @param newId New resource id
     * @return true on success
     */
    virtual bool RenameResource(const ResourceId& id, const ResourceId& newId) = 0;
    /**
     * @brief Remove the resource object from cache, the resource is not removed from the index
     */
    virtual bool PurgeResource(const ResourceId&) = 0;
    /**
     * @brief Remove the resource object from cache for group, the resources are not removed from the index
     */
    virtual size_t PurgeGroup(BASE_NS::string_view group) = 0;
    /**
     * @brief Remove resource from the index (along the object cache)
     */
    virtual bool RemoveResource(const ResourceId&) = 0;
    /**
     * @brief Remove all resource in a group from the index (along the object cache)
     */
    virtual bool RemoveGroup(BASE_NS::string_view group) = 0;
    /**
     * @brief Remove all resources
     */
    virtual void RemoveAllResources() = 0;
    /**
     * @brief Export resource's payload (if supported for the resource)
     */
    virtual Result ExportResourcePayload(const IResource::Ptr& resource) = 0;
    /**
     * @brief Set file manager to access files; If not set, default constructed manager is used with "file" filesystem
     * using the standard file system.
     */
    virtual void SetFileManager(CORE_NS::IFileManager::Ptr fileManager) = 0;

protected:
    IResourceManager() = default;
    virtual ~IResourceManager() = default;

    IResourceManager(const IResourceManager&) = delete;
    void operator=(const IResourceManager&) = delete;
};
CORE_END_NAMESPACE()
#endif // API_CORE_RESOURCES_IRESOURCE_MANAGER_H
