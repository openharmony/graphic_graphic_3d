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

#include "resources/resource_manager.h"

#include <algorithm>
#include <unordered_set>

#include <base/containers/byte_array.h>
#include <base/math/mathf.h>
#include <base/util/uid_util.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/log.h>
#include <core/resources/intf_serializable.h>

template<>
struct std::hash<BASE_NS::string> {
    std::size_t operator()(const BASE_NS::string& s) const noexcept
    {
        return BASE_NS::hash(s);
    }
};

CORE_BEGIN_NAMESPACE()

struct ResourceFileHeader {
    uint8_t magic[4U] { 'L', 'R', 'F', 0x0U };
    uint32_t resourceCount { 0U };
    uint8_t padding[16U] {};
};

struct ResourceBlobHeader {
    BASE_NS::Uid uid;
    uint32_t uriOffset { 0U };
    uint32_t uriSize { 0U };
    uint32_t dataOffset { 0U };
    uint32_t dataSize { 0U };
};

struct ResourceManager::ImportFile {
    IFile::Ptr file;
    BASE_NS::vector<ResourceBlobHeader> resourceHeaders;
    BASE_NS::vector<BASE_NS::string> resourceUris;
};

struct ResourceManager::ExportData {
    BASE_NS::vector<ResourceBlobHeader> resourceHeaders;
    BASE_NS::string uris;
    BASE_NS::vector<uint8_t> data;
};

ResourceManager::ResourceManager(IEngine& engine) : engine_(engine) {}

ResourceManager::~ResourceManager() = default;

const IInterface* ResourceManager::GetInterface(const BASE_NS::Uid& uid) const
{
    if (auto ret = IInterfaceHelper::GetInterface(uid)) {
        return ret;
    }
    return nullptr;
}

IInterface* ResourceManager::GetInterface(const BASE_NS::Uid& uid)
{
    if (auto ret = IInterfaceHelper::GetInterface(uid)) {
        return ret;
    }
    return nullptr;
}

IResourceManager::Status ResourceManager::Import(BASE_NS::string_view url)
{
    auto lock = std::lock_guard(mutex_);
    if (importedFiles_.contains(url)) {
        return Status::STATUS_OK;
    }
    auto& fm = engine_.GetFileManager();
    auto file = fm.OpenFile(url);
    if (!file) {
        return Status::STATUS_FILE_NOT_FOUND;
    }
    ResourceFileHeader header;
    if (file->Read(&header, sizeof(header)) != sizeof(header)) {
        return Status::STATUS_FILE_READ_ERROR;
    }
    if (memcmp(header.magic, ResourceFileHeader {}.magic, sizeof(header.magic)) != 0) {
        return Status::STATUS_INVALID_FILE;
    }
    const auto fileLength = file->GetLength();
    // check that all the blob headers fit in the file.
    if ((sizeof(ResourceBlobHeader) * header.resourceCount) > fileLength) {
        return Status::STATUS_INVALID_FILE;
    }
    BASE_NS::vector<ResourceBlobHeader> resourceHeaders;
    resourceHeaders.resize(header.resourceCount);
    if (file->Read(resourceHeaders.data(), sizeof(ResourceBlobHeader) * resourceHeaders.size()) !=
        (sizeof(ResourceBlobHeader) * header.resourceCount)) {
        return Status::STATUS_INVALID_FILE;
    }
    // check that all the offset + size fit in the file.
    for (const auto& blob : resourceHeaders) {
        if (((uint64_t(blob.uriOffset) + blob.uriSize) > fileLength)) {
            return Status::STATUS_INVALID_FILE;
        }
        if (((uint64_t(blob.dataOffset) + blob.dataSize) > fileLength)) {
            return Status::STATUS_INVALID_FILE;
        }
    }
    BASE_NS::vector<BASE_NS::string> resourceUris;
    for (const auto& blob : resourceHeaders) {
        file->Seek(blob.uriOffset);
        BASE_NS::string uri;
        uri.resize(blob.uriSize);
        file->Read(uri.data(), blob.uriSize);
        resourceUris.push_back(BASE_NS::move(uri));
    }

    importedFiles_.insert_or_assign(
        url, ImportFile { BASE_NS::move(file), BASE_NS::move(resourceHeaders), BASE_NS::move(resourceUris) });

    return Status::STATUS_OK;
}

BASE_NS::vector<BASE_NS::string> ResourceManager::GetResourceUris(BASE_NS::string_view url) const
{
    BASE_NS::vector<BASE_NS::string> uris;
    if (auto file = importedFiles_.find(url); file != importedFiles_.end()) {
        uris = file->second.resourceUris;
    }
    for (auto it = importedResources_.begin(); it != importedResources_.end(); ++it) {
        if (!it->first.starts_with(url)) {
            continue;
        }
        uris.emplace_back(it->first.substr(url.size() + 1U));
    }
    return uris;
}

BASE_NS::vector<BASE_NS::string> ResourceManager::GetResourceUris(
    BASE_NS::string_view url, BASE_NS::Uid resourceType) const
{
    BASE_NS::vector<BASE_NS::string> uris;
    if (auto pos = importedFiles_.find(url); pos != importedFiles_.end()) {
        const auto& file = pos->second;
        const auto count = BASE_NS::Math::min(file.resourceHeaders.size(), file.resourceUris.size());
        for (auto i = 0LLU; i < count; ++i) {
            if (file.resourceHeaders[i].uid == resourceType) {
                uris.push_back(file.resourceUris[i]);
            }
        }
    }
    return uris;
}

IResource::Ptr ResourceManager::GetResource(BASE_NS::string_view uri)
{
    auto lock = std::lock_guard(mutex_);
    if (auto pos = importedResources_.find(uri); pos != importedResources_.end()) {
        return pos->second;
    }

    for (const auto& file : importedFiles_) {
        if (!uri.starts_with(file.first)) {
            continue;
        }
        const auto resourceUri = uri.substr(file.first.size() + 1U);
        auto& importFile = file.second;
        auto pos = std::find(importFile.resourceUris.cbegin(), importFile.resourceUris.cend(), resourceUri);
        if (pos == importFile.resourceUris.cend()) {
            continue;
        }
        auto index = static_cast<size_t>(pos - importFile.resourceUris.cbegin());
        auto& resHeader = importFile.resourceHeaders[index];

        auto resource = engine_.GetInterface<CORE_NS::IClassFactory>()->CreateInstance(resHeader.uid);
        if (!resource) {
            const auto uidStr = BASE_NS::to_string(resHeader.uid);
            CORE_LOG_ONCE_W(uidStr, "Unknown resource type '%s'", uidStr.data());
            return {};
        }
        auto serializable = resource->GetInterface<CORE_NS::ISerializable>();
        if (!serializable) {
            const auto uidStr = BASE_NS::to_string(resHeader.uid);
            CORE_LOG_ONCE_W(uidStr, "Cannot import resource type '%s'", uidStr.data());
            return {};
        }
        BASE_NS::ByteArray data(resHeader.dataSize);
        if (resHeader.dataSize) {
            importFile.file->Seek(resHeader.dataOffset);
            importFile.file->Read(data.GetData().data(), resHeader.dataSize);
        }

        if (serializable->Import(resourceUri, data.GetData())) {
            importedResources_.insert({ uri, resource });
            return resource;
        }
    }
    return {};
}

IResource::Ptr ResourceManager::GetResource(BASE_NS::string_view groupUri, BASE_NS::string_view uri)
{
    return GetResource(groupUri + '/' + uri);
}

IResourceManager::Status ResourceManager::Export(BASE_NS::string_view groupUri, BASE_NS::string_view filePath)
{
    auto lock = std::lock_guard(mutex_);

    auto& fm = engine_.GetFileManager();
    auto file = fm.CreateFile(filePath);
    if (!file) {
        return Status::STATUS_FILE_NOT_FOUND;
    }

    const auto exportData = GatherExportData(groupUri);

    ResourceFileHeader header;
    header.resourceCount = static_cast<uint32_t>(exportData.resourceHeaders.size());
    auto status = Status::STATUS_OK;
    if (file->Write(&header, sizeof(header)) != sizeof(header)) {
        status = Status::STATUS_FILE_WRITE_ERROR;
    }
    if (file->Write(exportData.resourceHeaders.data(), sizeof(ResourceBlobHeader) * header.resourceCount) !=
        (sizeof(ResourceBlobHeader) * header.resourceCount)) {
        status = Status::STATUS_FILE_WRITE_ERROR;
    }
    if (file->Write(exportData.uris.data(), exportData.uris.size()) != exportData.uris.size()) {
        status = Status::STATUS_FILE_WRITE_ERROR;
    }
    if (file->Write(exportData.data.data(), exportData.data.size()) != exportData.data.size()) {
        status = Status::STATUS_FILE_WRITE_ERROR;
    }
    if (status != Status::STATUS_OK) {
        file.reset();
        fm.DeleteFile(filePath);
    }
    return status;
}

void ResourceManager::AddResource(BASE_NS::string_view groupUri, const IResource::Ptr& resource)
{
    if (!resource) {
        return;
    }
    auto uri = resource->GetUri();
    if (uri.empty()) {
        return;
    }
    const auto fullUri = groupUri + '/' + uri;
    auto lock = std::lock_guard(mutex_);
    importedResources_.insert_or_assign(fullUri, resource);
}

void ResourceManager::RemoveResource(BASE_NS::string_view groupUri, const IResource::Ptr& resource)
{
    if (!resource) {
        return;
    }
    const auto resourceUri = resource->GetUri();
    const auto fullUri = groupUri + '/' + resourceUri;
    auto lock = std::lock_guard(mutex_);
    importedResources_.erase(fullUri);
    if (auto file = importedFiles_.find(groupUri); file != importedFiles_.end()) {
        const auto& resourceUris = file->second.resourceUris;
        const auto pos = std::find(resourceUris.cbegin(), resourceUris.cend(), resourceUri);
        if (pos != resourceUris.cend()) {
            const auto index = pos - resourceUris.cbegin();
            file->second.resourceHeaders.erase(file->second.resourceHeaders.cbegin() + index);
            file->second.resourceUris.erase(pos);
        }
    }
}

void ResourceManager::RemoveResources(BASE_NS::string_view groupUri)
{
    auto lock = std::lock_guard(mutex_);
    for (auto it = importedResources_.begin(); it != importedResources_.end();) {
        if (it->first.starts_with(groupUri)) {
            it = importedResources_.erase(it);
        } else {
            ++it;
        }
    }
    importedFiles_.erase(groupUri);
}

ResourceManager::ExportData ResourceManager::GatherExportData(BASE_NS::string_view groupUri) const
{
    ExportData exportData;
    std::unordered_set<BASE_NS::string> exportedResources;
    for (auto it = importedResources_.begin(); it != importedResources_.end(); ++it) {
        if (!it->first.starts_with(groupUri)) {
            continue;
        }
        const auto& imported = *it;
        const auto uriOffset = static_cast<uint32_t>(exportData.uris.size());
        const auto resourceUri = imported.first.substr(groupUri.size() + 1U);
        const auto uriSize = static_cast<uint32_t>(resourceUri.size());
        exportData.uris.append(resourceUri);
        exportedResources.insert(BASE_NS::string(resourceUri));

        const auto& resource = imported.second;
        const auto uid = resource->GetType();
        auto serializable = resource->GetInterface<CORE_NS::ISerializable>();
        if (!serializable) {
            const auto uidStr = BASE_NS::to_string(uid);
            CORE_LOG_ONCE_W(uidStr, "Cannot serialize resource type '%s'", uidStr.data());
            continue;
        }
        auto& header = exportData.resourceHeaders.emplace_back();
        header.uid = uid;
        header.uriOffset = uriOffset;
        header.uriSize = uriSize;
        auto resData = serializable->Export();
        header.dataOffset = static_cast<uint32_t>(exportData.data.size());
        header.dataSize = static_cast<uint32_t>(resData.size());
        exportData.data.append(resData.cbegin(), resData.cend());
    }

    // check if groupUri is found from importedFiles_ and copy the resource which were not in importedResources_
    if (auto pos = importedFiles_.find(groupUri); pos != importedFiles_.cend()) {
        auto& importedFile = pos->second;
        for (auto uriIt = importedFile.resourceUris.cbegin(), end = importedFile.resourceUris.cend(); uriIt != end;
             ++uriIt) {
            if (exportedResources.count(*uriIt)) {
                continue;
            }
            const auto index = size_t(uriIt - importedFile.resourceUris.cbegin());
            const auto& resourceHeader = importedFile.resourceHeaders[index];
            if (!resourceHeader.dataSize) {
                continue;
            }
            BASE_NS::ByteArray resourceData(resourceHeader.dataSize);
            importedFile.file->Seek(resourceHeader.dataOffset);
            if (importedFile.file->Read(resourceData.GetData().data(), resourceHeader.dataSize) !=
                resourceHeader.dataSize) {
                continue;
            }

            const auto uriOffset = static_cast<uint32_t>(exportData.uris.size());
            const auto uriSize = static_cast<uint32_t>(uriIt->size());
            exportData.uris.append(*uriIt);
            exportedResources.insert(*uriIt);

            auto& header = exportData.resourceHeaders.emplace_back();
            header.uid = resourceHeader.uid;
            header.uriOffset = uriOffset;
            header.uriSize = uriSize;
            header.dataOffset = static_cast<uint32_t>(exportData.data.size());
            header.dataSize = resourceHeader.dataSize;
            exportData.data.append(resourceData.GetData().cbegin(), resourceData.GetData().cend());
        }
    }
    const auto resourceCount = exportData.resourceHeaders.size();
    const auto uriOffset =
        static_cast<uint32_t>(sizeof(ResourceFileHeader) + sizeof(ResourceBlobHeader) * resourceCount);
    const auto dataOffset = uriOffset + static_cast<uint32_t>(exportData.uris.size());
    for (auto& blob : exportData.resourceHeaders) {
        blob.uriOffset += uriOffset;
        blob.dataOffset += dataOffset;
    }
    return exportData;
}
CORE_END_NAMESPACE()
