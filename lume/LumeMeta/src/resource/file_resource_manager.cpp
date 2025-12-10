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

// disabled exceptions for msvc iomanip
#ifndef __OHOS__
#define _HAS_EXCEPTIONS 0
#endif

#include "file_resource_manager.h"

#include <iomanip>
#include <sstream>

#include <base/util/base64_decode.h>
#include <base/util/base64_encode.h>
#include <core/io/intf_filesystem_api.h>

#include <meta/interface/resource/intf_resource.h>

#include "memfile.h"
#include "resource_group_handle.h"

META_BEGIN_NAMESPACE()

static bool ResourceMatches(
    const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection, const CORE_NS::ResourceId& id)
{
    for (auto&& s : selection) {
        if ((s.hasWildCardName || id.name == s.name) && (s.hasWildCardGroup || id.group == s.group)) {
            return true;
        }
    }
    return false;
}

FileResourceManager::FileResourceManager()
{
    if (auto factory = CORE_NS::GetInstance<CORE_NS::IFileSystemApi>(CORE_NS::UID_FILESYSTEM_API_FACTORY)) {
        fileManager_ = factory->CreateFilemanager();
        fileManager_->RegisterFilesystem("file", factory->CreateStdFileSystem());
    }
}

void FileResourceManager::AddListener(const IResourceListener::Ptr& p)
{
    std::unique_lock lock { listenerMutex_ };
    for (auto&& v : listeners_) {
        if (v.lock() == p) {
            return;
        }
    }
    listeners_.push_back(p);
}
void FileResourceManager::RemoveListener(const IResourceListener::Ptr& p)
{
    std::unique_lock lock { listenerMutex_ };
    auto it = listeners_.begin();
    for (; it != listeners_.end() && it->lock() != p; ++it) {
    }
    if (it != listeners_.end()) {
        listeners_.erase(it);
    }
}
void FileResourceManager::Notify(const BASE_NS::vector<CORE_NS::ResourceId>& ids, IResourceListener::EventType type)
{
    if (!ids.empty()) {
        BASE_NS::vector<IResourceListener::WeakPtr> list;
        {
            std::shared_lock lock { listenerMutex_ };
            list = listeners_;
        }
        for (auto&& v : list) {
            if (auto l = v.lock()) {
                l->OnResourceEvent(type, ids);
            }
        }
    }
}
bool FileResourceManager::AddResourceType(CORE_NS::IResourceType::Ptr p)
{
    if (p) {
        std::unique_lock lock { mutex_ };
        types_[p->GetResourceType()] = p;
    }
    return p != nullptr;
}
bool FileResourceManager::RemoveResourceType(const CORE_NS::ResourceType& type)
{
    std::unique_lock lock { mutex_ };
    types_.erase(type);
    return true;
}
BASE_NS::vector<CORE_NS::IResourceType::Ptr> FileResourceManager::GetResourceTypes() const
{
    std::shared_lock lock { mutex_ };
    BASE_NS::vector<CORE_NS::IResourceType::Ptr> res;
    for (auto&& v : types_) {
        res.push_back(v.second);
    }
    return res;
}
CORE_NS::ResourceId FileResourceManager::ReadHeader(
    const std::string& line, BASE_NS::vector<CORE_NS::IResource::Ptr>& destroy)
{
    std::istringstream in(line);
    std::string name;
    std::string group;
    std::string path;
    std::string type;
    in >> std::quoted(name) >> std::quoted(group) >> std::quoted(path) >> type;
    if (!in || type.size() != 36U) {
        return {};
    }

    auto data = BASE_NS::make_shared<ResourceData>();
    data->id =
        CORE_NS::ResourceId { BASE_NS::string(name.data(), name.size()), BASE_NS::string(group.data(), group.size()) };
    data->path = BASE_NS::string(path.data(), path.size());
    data->type = BASE_NS::StringToUid(BASE_NS::string_view(type.data(), type.size()));

    std::string opt;
    if (in >> opt) {
        data->optionData = BASE_NS::Base64Decode(opt.c_str());
    }
    auto& v = resources_[data->id.group][data->id.name];
    if (v && v->object) {
        destroy.push_back(v->object);
    }
    v = data;
    return data->id;
}
bool FileResourceManager::ReadHeaders(CORE_NS::IFile& file, BASE_NS::vector<CORE_NS::ResourceId>& result,
    BASE_NS::vector<CORE_NS::IResource::Ptr>& destroy)
{
    std::string vec;
    vec.resize(file.GetLength());
    file.Read(vec.data(), vec.size());
    std::istringstream ss(vec);
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line[0] != '#') {
            auto res = ReadHeader(line, destroy);
            if (!res.IsValid()) {
                return false;
            }
            result.push_back(std::move(res));
        }
    }
    return true;
}
FileResourceManager::Result FileResourceManager::Import(BASE_NS::string_view url)
{
    BASE_NS::vector<CORE_NS::IResource::Ptr> destroy;
    BASE_NS::vector<CORE_NS::ResourceId> result;
    {
        std::unique_lock lock { mutex_ };
        auto f = fileManager_->OpenFile(url);
        if (!f) {
            CORE_LOG_W("Failed to open resource manager file: %s", BASE_NS::string(url).c_str());
            return Result::FILE_NOT_FOUND;
        }
        if (!ReadHeaders(*f, result, destroy)) {
            return Result::INVALID_FILE;
        }
    }
    Notify(result, IResourceListener::EventType::ADDED);
    return Result::OK;
}
BASE_NS::vector<CORE_NS::ResourceInfo> FileResourceManager::GetResourceInfos(
    const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection) const
{
    std::shared_lock lock { mutex_ };
    BASE_NS::vector<CORE_NS::ResourceInfo> infos;
    for (auto&& g : resources_) {
        for (auto&& v : g.second) {
            if (ResourceMatches(selection, v.second->id)) {
                infos.push_back(*v.second);
            }
        }
    }
    return infos;
}
BASE_NS::vector<BASE_NS::string> FileResourceManager::GetResourceGroups() const
{
    std::shared_lock lock { mutex_ };
    BASE_NS::vector<BASE_NS::string> groups;
    for (auto&& g : resources_) {
        groups.push_back(g.first);
    }
    return groups;
}
CORE_NS::ResourceInfo FileResourceManager::GetResourceInfo(const CORE_NS::ResourceId& id) const
{
    std::shared_lock lock { mutex_ };
    auto git = resources_.find(id.group);
    if (git != resources_.end()) {
        auto it = git->second.find(id.name);
        return it != git->second.end() ? CORE_NS::ResourceInfo { *it->second } : CORE_NS::ResourceInfo {};
    }
    return {};
}
BASE_NS::shared_ptr<ResourceData> FileResourceManager::FindResource(const CORE_NS::ResourceId& id)
{
    auto git = resources_.find(id.group);
    if (git == resources_.end()) {
        return nullptr;
    }
    auto it = git->second.find(id.name);
    if (it == git->second.end()) {
        return nullptr;
    }
    return it->second;
}
CORE_NS::IResource::Ptr FileResourceManager::GetResource(
    const CORE_NS::ResourceId& id, const BASE_NS::shared_ptr<CORE_NS::IInterface>& context)
{
    {
        std::shared_lock lock { mutex_ };
        if (auto res = FindResource(id)) {
            if (res->object) {
                return res->object;
            }
        }
    }
    return ConstructResource(id, context);
}
BASE_NS::vector<CORE_NS::IResource::Ptr> FileResourceManager::GetResources(
    const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection,
    const BASE_NS::shared_ptr<CORE_NS::IInterface>& context)
{
    BASE_NS::vector<CORE_NS::IResource::Ptr> res;
    BASE_NS::vector<CORE_NS::ResourceId> populate;
    {
        std::shared_lock lock { mutex_ };
        for (auto&& g : resources_) {
            for (auto&& v : g.second) {
                auto& r = *v.second;
                if (ResourceMatches(selection, r.id)) {
                    if (r.object) {
                        res.push_back(r.object);
                    } else {
                        populate.push_back(r.id);
                    }
                }
            }
        }
    }
    for (auto&& v : populate) {
        if (auto p = ConstructResource(v, context)) {
            res.push_back(p);
        }
    }
    return res;
}
static void SaveResourceHeader(const ResourceData& r, CORE_NS::IFile& file)
{
    std::ostringstream out;
    out << std::quoted(r.id.name.c_str()) << " " << std::quoted(r.id.group.c_str()) << " "
        << std::quoted(r.path.c_str()) << " " << BASE_NS::to_string(r.type).c_str() << " ";
    out << BASE_NS::Base64Encode(r.optionData).c_str();
    out << "\n";
    auto buf = out.str();
    file.Write(buf.data(), buf.size());
}
FileResourceManager::Result FileResourceManager::SaveResourceData(const ResourceData& r)
{
    if (!r.object) {
        return Result::NO_RESOURCE_DATA;
    }
    auto it = types_.find(r.type);
    if (it == types_.end()) {
        CORE_LOG_W("Resource type not registered [type=%s]", BASE_NS::to_string(r.type).c_str());
        return Result::NO_RESOURCE_TYPE;
    }
    if (!r.path.empty()) {
        MemFile data;
        if (!it->second->SaveResource(r.object,
                CORE_NS::IResourceType::StorageInfo { nullptr, &data, r.id, r.path, GetSelf<IResourceManager>() })) {
            CORE_LOG_W("Failed to save resource: %s", r.id.ToString().c_str());
            return Result::EXPORT_FAILURE;
        }
        if (data.GetLength()) {
            auto f = fileManager_->CreateFile(r.path);
            if (!f) {
                CORE_LOG_W("Failed to open resource file: %s", r.path.c_str());
                return Result::FILE_WRITE_ERROR;
            }
            f->Write(data.RawData(), data.GetLength());
        }
    }
    return Result::OK;
}
bool FileResourceManager::UpdateOptions(ResourceData& r)
{
    if (r.object) {
        auto it = types_.find(r.type);
        if (it == types_.end()) {
            CORE_LOG_W("Resource type not registered [type=%s]", BASE_NS::to_string(r.type).c_str());
            return false;
        }
        MemFile opts;
        if (!it->second->SaveResource(r.object,
                CORE_NS::IResourceType::StorageInfo { &opts, nullptr, r.id, r.path, GetSelf<IResourceManager>() })) {
            CORE_LOG_W("Failed to save resource options");
            return false;
        }
        r.optionData = opts.Data();
    }
    return true;
}
FileResourceManager::Result FileResourceManager::Export(
    BASE_NS::string_view filePath, const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection)
{
    std::unique_lock lock { mutex_ };
    auto f = fileManager_->CreateFile(filePath);
    if (!f) {
        CORE_LOG_W("Failed to create resource manager file: %s", BASE_NS::string(filePath).c_str());
        return Result::FILE_WRITE_ERROR;
    }
    BASE_NS::string vstr = "# Lume Resource Index Version 1\n";
    f->Write(vstr.data(), vstr.size());
    for (auto&& g : resources_) {
        for (auto&& v : g.second) {
            auto& res = *v.second;
            if (ResourceMatches(selection, res.id)) {
                UpdateOptions(res);
                SaveResourceHeader(res, *f);
                SaveResourceData(res);
            }
        }
    }
    return Result::OK;
}

bool FileResourceManager::AddResource(const CORE_NS::IResource::Ptr& resource)
{
    return AddResource(resource, resource->GetResourceId().name);
}
bool FileResourceManager::AddResource(const CORE_NS::IResource::Ptr& resource, BASE_NS::string_view path)
{
    CORE_NS::IResource::Ptr res;
    {
        std::unique_lock lock { mutex_ };
        auto it = types_.find(resource->GetResourceType());
        if (it == types_.end()) {
            CORE_LOG_W(
                "Resource type not registered [type=%s]", BASE_NS::to_string(resource->GetResourceType()).c_str());
            return false;
        }
        auto id = resource->GetResourceId();
        ResourceData d;
        d.object = resource;
        d.type = resource->GetResourceType();
        d.id = id;
        d.path = path;
        MemFile opts;
        if (!it->second->SaveResource(resource,
                CORE_NS::IResourceType::StorageInfo { &opts, nullptr, d.id, d.path, GetSelf<IResourceManager>() })) {
            CORE_LOG_W("Failed to save resource options");
            return false;
        }
        d.optionData = opts.Data();
        auto& v = resources_[id.group][id.name];
        if (v) {
            res = v->object;
        }
        v = BASE_NS::make_shared<ResourceData>(BASE_NS::move(d));
    }
    Notify({ resource->GetResourceId() }, IResourceListener::EventType::ADDED);
    return true;
}
bool FileResourceManager::AddResource(const CORE_NS::ResourceId& id, const CORE_NS::ResourceType& type,
    BASE_NS::string_view path, const CORE_NS::IResourceOptions::ConstPtr& options)
{
    CORE_NS::IResource::Ptr res;
    {
        std::unique_lock lock { mutex_ };
        auto it = types_.find(type);
        if (it == types_.end()) {
            CORE_LOG_W("Resource type not registered [type=%s]", BASE_NS::to_string(type).c_str());
            return false;
        }
        ResourceData d;
        d.type = type;
        d.id = id;
        d.path = path;
        if (options) {
            MemFile opts;
            options->Save(opts, GetSelf<IResourceManager>(), {});
            d.optionData = opts.Data();
        }
        auto& v = resources_[id.group][id.name];
        if (v) {
            res = v->object;
        }
        v = BASE_NS::make_shared<ResourceData>(BASE_NS::move(d));
    }
    Notify({ id }, IResourceListener::EventType::ADDED);
    return true;
}
bool FileResourceManager::ReloadResource(
    const CORE_NS::IResource::Ptr& resource, const BASE_NS::shared_ptr<CORE_NS::IInterface>& context)
{
    CORE_NS::IFile::Ptr f;
    MemFile opts;
    BASE_NS::string path;
    CORE_NS::IResourceType::Ptr type;
    {
        std::unique_lock lock { mutex_ };
        auto it = types_.find(resource->GetResourceType());
        if (it == types_.end()) {
            CORE_LOG_W(
                "Resource type not registered [type=%s]", BASE_NS::to_string(resource->GetResourceType()).c_str());
            return false;
        }
        type = it->second;
        auto res = FindResource(resource->GetResourceId());
        if (!res) {
            CORE_LOG_W("No such resource: %s", resource->GetResourceId().ToString().c_str());
            return false;
        }
        path = res->path;
        if (!path.empty()) {
            f = fileManager_->OpenFile(path);
            if (!f) {
                CORE_LOG_W("Failed to open resource file: %s", path.c_str());
                return false;
            }
        }
        opts = MemFile(res->optionData);
    }
    CORE_NS::IResourceType::StorageInfo info { &opts, f.get(), resource->GetResourceId(), path,
        GetSelf<IResourceManager>(), context };
    return type->ReloadResource(info, resource);
}
bool FileResourceManager::RenameResource(const CORE_NS::ResourceId& id, const CORE_NS::ResourceId& newId)
{
    {
        std::unique_lock lock { mutex_ };
        auto git = resources_.find(id.group);
        if (git == resources_.end()) {
            return false;
        }
        auto it = git->second.find(id.name);
        if (it == git->second.end()) {
            return false;
        }
        auto& v = resources_[newId.group][newId.name];
        if (v) {
            return false;
        }
        v = it->second;
        git->second.erase(it);
    }

    Notify({ id }, IResourceListener::EventType::REMOVED);
    Notify({ newId }, IResourceListener::EventType::ADDED);
    return true;
}
bool FileResourceManager::PurgeResource(const CORE_NS::ResourceId& id)
{
    CORE_NS::IResource::Ptr obj;
    std::unique_lock lock { mutex_ };
    if (auto res = FindResource(id)) {
        std::swap(obj, res->object);
        return true;
    }
    CORE_LOG_W("No such resource: %s", id.ToString().c_str());
    return false;
}
bool FileResourceManager::RemoveResource(const CORE_NS::ResourceId& id)
{
    CORE_NS::IResource::Ptr obj;
    bool res = false;
    {
        std::unique_lock lock { mutex_ };
        if (auto git = resources_.find(id.group); git != resources_.end()) {
            if (auto it = git->second.find(id.name); it != git->second.end()) {
                if (it->second) {
                    obj = it->second->object;
                }
                git->second.erase(it);
                res = true;
            }
        }
    }
    if (res) {
        Notify({ id }, IResourceListener::EventType::REMOVED);
    }
    return res;
}
size_t FileResourceManager::PurgeGroup(BASE_NS::string_view group)
{
    BASE_NS::vector<CORE_NS::IResource::Ptr> objs;
    size_t count = 0;
    std::unique_lock lock { mutex_ };
    auto git = resources_.find(group);
    if (git != resources_.end()) {
        for (auto it = git->second.begin(); it != git->second.end(); ++it) {
            if (it->second->object) {
                objs.push_back(it->second->object);
                it->second->object = nullptr;
                ++count;
            }
        }
    }
    return count;
}
bool FileResourceManager::RemoveGroup(BASE_NS::string_view group)
{
    bool res = false;
    BASE_NS::vector<CORE_NS::IResource::Ptr> objs;
    BASE_NS::vector<CORE_NS::ResourceId> result;
    {
        std::unique_lock lock { mutex_ };
        auto it = resources_.find(group);
        res = it != resources_.end();
        if (res) {
            for (auto&& g : it->second) {
                objs.push_back(g.second->object);
                result.push_back(g.second->id);
            }
            resources_.erase(it);
        }
        ownedGroups_.erase(group);
    }
    if (res) {
        Notify(result, IResourceListener::EventType::REMOVED);
    }
    return res;
}
void FileResourceManager::RemoveAllResources()
{
    BASE_NS::vector<CORE_NS::IResource::Ptr> objs;
    BASE_NS::vector<CORE_NS::ResourceId> result;
    {
        std::unique_lock lock { mutex_ };
        for (auto&& g : resources_) {
            for (auto&& v : g.second) {
                objs.push_back(v.second->object);
                result.push_back(v.second->id);
            }
            ownedGroups_.erase(g.first);
        }
        resources_.clear();
    }
    Notify(result, IResourceListener::EventType::REMOVED);
}

FileResourceManager::Result FileResourceManager::ExportResourcePayload(const CORE_NS::IResource::Ptr& resource)
{
    std::shared_lock lock { mutex_ };
    auto res = FindResource(resource->GetResourceId());
    if (!res) {
        CORE_LOG_W("No such resource: %s", resource->GetResourceId().ToString().c_str());
        return Result::NO_RESOURCE_DATA;
    }
    return SaveResourceData(*res);
}
void FileResourceManager::SetFileManager(CORE_NS::IFileManager::Ptr fileManager)
{
    std::unique_lock lock { mutex_ };
    fileManager_ = BASE_NS::move(fileManager);
}
CORE_NS::IResource::Ptr FileResourceManager::ConstructResource(
    const CORE_NS::ResourceId& id, const BASE_NS::shared_ptr<CORE_NS::IInterface>& context)
{
    BASE_NS::shared_ptr<ResourceData> res;
    CORE_NS::IResourceType::ConstPtr type;
    BASE_NS::string path;
    BASE_NS::vector<uint8_t> optionData;
    CORE_NS::IFileManager::Ptr fileManager;

    {
        std::shared_lock lock { mutex_ };
        res = FindResource(id);
        if (res) {
            auto tit = types_.find(res->type);
            if (tit != types_.end()) {
                type = tit->second;
                path = res->path;
                optionData = res->optionData;
                fileManager = fileManager_;
            }
        }
    }

    CORE_NS::IResource::Ptr resObj;
    if (res && type) {
        CORE_NS::IFile::Ptr f;
        if (!path.empty()) {
            f = fileManager->OpenFile(path);
            if (!f) {
                CORE_LOG_W("Failed to open resource file: %s", path.c_str());
                return nullptr;
            }
        }
        MemFile opts(optionData);
        resObj = type->LoadResource(
            CORE_NS::IResourceType::StorageInfo { &opts, f.get(), id, path, GetSelf<IResourceManager>(), context });
        if (resObj) {
            if (auto i = interface_cast<CORE_NS::ISetResourceId>(resObj)) {
                i->SetResourceId(id);
            }
        } else {
            CORE_LOG_W("Failed to load resource: %s", id.ToString().c_str());
        }
    }

    if (res && resObj) {
        std::unique_lock lock { mutex_ };
        if (res->object) {
            resObj = res->object;
        } else {
            res->object = resObj;
        }
    }
    return resObj;
}

IResourceGroupHandle::Ptr FileResourceManager::GetGroupHandle(BASE_NS::string_view group)
{
    IResourceGroupHandle::Ptr res;
    std::unique_lock lock { mutex_ };
    auto it = ownedGroups_.find(group);
    if (it != ownedGroups_.end()) {
        res = it->second.lock();
    }
    if (!res) {
        res = BASE_NS::make_shared<ResourceGroupHandle>(GetSelf<CORE_NS::IResourceManager>(), BASE_NS::string(group));
        ownedGroups_[group] = res;
    }
    return res;
}

uint32_t FileResourceManager::GetAliveCount(
    const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection) const
{
    return FindAliveResources(selection).size();
}

BASE_NS::vector<CORE_NS::IResource::Ptr> FileResourceManager::FindAliveResources(
    const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection) const
{
    BASE_NS::vector<CORE_NS::IResource::Ptr> res;
    std::shared_lock lock { mutex_ };
    for (auto&& g : resources_) {
        for (auto&& v : g.second) {
            auto& r = *v.second;
            if (ResourceMatches(selection, r.id)) {
                if (r.object) {
                    res.push_back(r.object);
                }
            }
        }
    }
    return res;
}

META_END_NAMESPACE()
