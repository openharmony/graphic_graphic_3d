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

#ifdef __OHOS__
#include <iomanip>
#else
#include "iomanip_wo_exceptions.h"
#endif

#include <sstream>

#include <base/containers/flat_map.h>
#include <base/util/base64_decode.h>
#include <base/util/base64_encode.h>
#include <core/io/intf_filesystem_api.h>

#include <meta/base/memfile.h>
#include <meta/ext/minimal_object.h>
#include <meta/interface/resource/intf_resource.h>

#include "file_resource_manager.h"
#include "resource_group_handle.h"

META_BEGIN_NAMESPACE()

FileResourceManager::FileResourceManager()
{
    if (auto factory = CORE_NS::GetInstance<CORE_NS::IFileSystemApi>(CORE_NS::UID_FILESYSTEM_API_FACTORY)) {
        fileManager_ = factory->CreateFilemanager();
        fileManager_->RegisterFilesystem("file", factory->CreateStdFileSystem());
    }
}

void FileResourceManager::AddListener(const IResourceListener::Ptr& p)
{
    std::unique_lock lock{listenerMutex_};
    for (auto&& v : listeners_) {
        if (v.lock() == p) {
            return;
        }
    }
    listeners_.push_back(p);
}
void FileResourceManager::RemoveListener(const IResourceListener::Ptr& p)
{
    std::unique_lock lock{listenerMutex_};
    auto it = listeners_.begin();
    for (; it != listeners_.end() && it->lock() != p; ++it) {
    }
    if (it != listeners_.end()) {
        listeners_.erase(it);
    }
}
void FileResourceManager::Notify(const BASE_NS::vector<ResourceIdContext>& ids, IResourceListener::EventType type)
{
    if (!ids.empty()) {
        BASE_NS::vector<IResourceListener::WeakPtr> list;
        {
            std::shared_lock lock{listenerMutex_};
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
        std::unique_lock lock{mutex_};
        types_[ResourceTypeKey{p->GetResourceType(), p->GetVersion()}] = p;
    }
    return p != nullptr;
}
bool FileResourceManager::RemoveResourceType(const CORE_NS::ResourceType& type, BASE_NS::string_view version)
{
    std::unique_lock lock{mutex_};
    types_.erase(ResourceTypeKey{type, BASE_NS::string(version)});
    return true;
}
BASE_NS::vector<CORE_NS::IResourceType::Ptr> FileResourceManager::GetResourceTypes() const
{
    std::shared_lock lock{mutex_};
    BASE_NS::vector<CORE_NS::IResourceType::Ptr> res;
    for (auto&& v : types_) {
        res.push_back(v.second);
    }
    return res;
}
CORE_NS::ResourceIdContext FileResourceManager::ReadHeader(
    const std::string& line, const ResourceContextPtr& context, BASE_NS::vector<CORE_NS::IResource::Ptr>& destroy)
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
        CORE_NS::ResourceId{BASE_NS::string(name.data(), name.size()), BASE_NS::string(group.data(), group.size())};
    data->path = BASE_NS::string(path.data(), path.size());
    data->type = BASE_NS::StringToUid(BASE_NS::string_view(type.data(), type.size()));

    data->options = GetObjectRegistry().Create<CORE_NS::IResourceOptions>(META_NS::ClassId::ObjectResourceOptions);
    if (!data->options) {
        CORE_LOG_E("Invalid state");
        return {};
    }
    std::string opt;
    if (in >> opt) {
        if (auto opts = interface_cast<IObjectResourceOptions>(data->options)) {
            opts->SetOptionData(BASE_NS::Base64Decode(opt.c_str()));
        }
    }
    auto& v = resources_[ResourceGroupContext{data->id.group, context}][data->id.name];
    if (v && v->object) {
        destroy.push_back(v->object);
    }
    v = data;
    return {data->id, context};
}
bool FileResourceManager::ReadHeaders(CORE_NS::IFile& file, const ResourceContextPtr& context,
    BASE_NS::vector<CORE_NS::ResourceIdContext>& result, BASE_NS::vector<CORE_NS::IResource::Ptr>& destroy)
{
    std::string vec;
    vec.resize(file.GetLength());
    file.Read(vec.data(), vec.size());
    std::istringstream ss(vec);
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line[0] != '#') {
            auto res = ReadHeader(line, context, destroy);
            if (!res.id.IsValid()) {
                return false;
            }
            result.push_back(std::move(res));
        }
    }
    return true;
}
FileResourceManager::Result FileResourceManager::Import(BASE_NS::string_view url, const ResourceContextPtr& context)
{
    BASE_NS::vector<CORE_NS::IResource::Ptr> destroy;
    BASE_NS::vector<CORE_NS::ResourceIdContext> result;
    {
        std::unique_lock lock{mutex_};
        auto f = fileManager_->OpenFile(url);
        if (!f) {
            CORE_LOG_W("Failed to open resource manager file: %s", BASE_NS::string(url).c_str());
            return Result::FILE_NOT_FOUND;
        }
        if (!ReadHeaders(*f, context, result, destroy)) {
            return Result::INVALID_FILE;
        }
    }
    Notify(result, IResourceListener::EventType::ADDED);
    return Result::OK;
}
BASE_NS::vector<CORE_NS::ResourceInfo> FileResourceManager::GetResourceInfos(
    const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection, const ResourceContextPtr& context) const
{
    std::shared_lock lock{mutex_};
    BASE_NS::vector<CORE_NS::ResourceInfo> infos;
    for (auto&& g : resources_) {
        if (IsCorrectContext(g.first, context)) {
            for (auto&& v : g.second) {
                if (IsResourceMatch(selection, v.second->id)) {
                    infos.push_back(*v.second);
                }
            }
        }
    }
    return infos;
}
BASE_NS::vector<BASE_NS::string> FileResourceManager::GetResourceGroups(const ResourceContextPtr& context) const
{
    std::shared_lock lock{mutex_};
    BASE_NS::vector<BASE_NS::string> groups;
    for (auto&& g : resources_) {
        if (IsCorrectContext(g.first, context)) {
            groups.push_back(g.first.group);
        }
    }
    return groups;
}
CORE_NS::ResourceInfo FileResourceManager::GetResourceInfo(const ResourceIdContext& ric) const
{
    std::shared_lock lock{mutex_};
    auto git = resources_.find(ResourceGroupContext{ric});
    if (git != resources_.end()) {
        auto it = git->second.find(ric.id.name);
        return it != git->second.end() ? CORE_NS::ResourceInfo{*it->second} : CORE_NS::ResourceInfo{};
    }
    return {};
}
BASE_NS::shared_ptr<ResourceData> FileResourceManager::FindResource(const CORE_NS::ResourceIdContext& ric)
{
    auto git = resources_.find(ResourceGroupContext{ric});
    if (git == resources_.end()) {
        return nullptr;
    }
    auto it = git->second.find(ric.id.name);
    if (it == git->second.end()) {
        return nullptr;
    }
    return it->second;
}
CORE_NS::IResource::Ptr FileResourceManager::GetResource(const CORE_NS::ResourceIdContext& ric)
{
    {
        std::shared_lock lock{mutex_};
        if (auto res = FindResource(ric)) {
            if (res->object) {
                return res->object;
            }
        }
    }
    return ConstructResource(ric);
}
BASE_NS::vector<CORE_NS::IResource::Ptr> FileResourceManager::GetResources(
    const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection, const ResourceContextPtr& context)
{
    BASE_NS::vector<CORE_NS::IResource::Ptr> res;
    BASE_NS::vector<ResourceIdContext> populate;

    auto pushRes = [&](const auto& r) {
        if (IsResourceMatch(selection, r.id)) {
            if (r.object) {
                res.push_back(r.object);
            } else {
                populate.push_back(ResourceIdContext{r.id, context});
            }
        }
    };

    {
        std::shared_lock lock{mutex_};
        for (auto&& g : resources_) {
            if (IsCorrectContext(g.first, context)) {
                for (auto&& v : g.second) {
                    pushRes(*v.second);
                }
            }
        }
    }
    for (auto&& v : populate) {
        if (auto p = ConstructResource(v)) {
            res.push_back(p);
        }
    }
    return res;
}
static void SaveResourceHeader(const ResourceData& r, CORE_NS::IFile& file)
{
    auto opts = interface_cast<IObjectResourceOptions>(r.options);
    if (!opts) {
        return;
    }

    std::ostringstream out;
    out << std::quoted(r.id.name.c_str()) << " " << std::quoted(r.id.group.c_str()) << " "
        << std::quoted(r.path.c_str()) << " " << BASE_NS::to_string(r.type).c_str() << " ";
    out << BASE_NS::Base64Encode(opts->GetOptionData()).c_str();
    out << "\n";
    auto buf = out.str();
    file.Write(buf.data(), buf.size());
}
FileResourceManager::Result FileResourceManager::SaveResourceData(const ResourceData& r)
{
    if (!r.object) {
        return Result::NO_RESOURCE_DATA;
    }
    auto it = types_.find(ResourceTypeKey{r.type, r.version});
    if (it == types_.end()) {
        CORE_LOG_W("Resource type not registered [type=%s]", BASE_NS::to_string(r.type).c_str());
        return Result::NO_RESOURCE_TYPE;
    }
    if (r.path.empty()) {
        return Result::OK;
    }
    MemFile data;
    if (!it->second->SaveResource(
            r.object, CORE_NS::IResourceType::StorageInfo{nullptr, &data, r.id, r.path, GetSelf<IResourceManager>()})) {
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
    return Result::OK;
}
bool FileResourceManager::UpdateOptions(ResourceData& r, CORE_NS::ResourceContextPtr context) const
{
    auto it = types_.find(ResourceTypeKey{r.type, r.version});
    if (it == types_.end()) {
        CORE_LOG_W("Resource type not registered [type=%s]", BASE_NS::to_string(r.type).c_str());
        return false;
    }
    if (!it->second->SaveResource(r.object,
            CORE_NS::IResourceType::StorageInfo{
                r.options, nullptr, r.id, r.path, GetSelf<IResourceManager>(), context})) {
        CORE_LOG_W("Failed to save resource options");
        return false;
    }
    return true;
}

FileResourceManager::Result FileResourceManager::Export(BASE_NS::string_view filePath,
    const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection, const ResourceContextPtr& context)
{
    std::unique_lock lock{mutex_};
    auto f = fileManager_->CreateFile(filePath);
    if (!f) {
        CORE_LOG_W("Failed to create resource manager file: %s", BASE_NS::string(filePath).c_str());
        return Result::FILE_WRITE_ERROR;
    }
    BASE_NS::flat_map<CORE_NS::ResourceId, BASE_NS::shared_ptr<ResourceData>, ResourceIdLess> imap;
    auto pushRes = [&](auto res) {
        if (IsResourceMatch(selection, res->id)) {
            imap.insert({res->id, res});
        }
    };
    for (auto&& g : resources_) {
        if (IsCorrectContext(g.first, context)) {
            for (auto&& v : g.second) {
                pushRes(v.second);
            }
        }
    }
    BASE_NS::string vstr = "# Lume Resource Index Version 1\n";
    f->Write(vstr.data(), vstr.size());
    for (auto&& v : imap) {
        auto& res = *v.second;
        UpdateOptions(res, context);
        SaveResourceHeader(res, *f);
        SaveResourceData(res);
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
    auto id = resource->GetResourceId();
    {
        std::unique_lock lock{mutex_};
        auto it = types_.find(ResourceTypeKey{resource->GetResourceType(), ""});
        if (it == types_.end()) {
            CORE_LOG_W(
                "Resource type not registered [type=%s]", BASE_NS::to_string(resource->GetResourceType()).c_str());
            return false;
        }
        ResourceData d;
        d.object = resource;
        d.type = resource->GetResourceType();
        d.id = id;
        d.path = path;

        d.options = GetObjectRegistry().Create<CORE_NS::IResourceOptions>(META_NS::ClassId::ObjectResourceOptions);
        if (!d.options) {
            CORE_LOG_E("Invalid state");
            return false;
        }

        if (!it->second->SaveResource(resource,
                CORE_NS::IResourceType::StorageInfo{d.options, nullptr, d.id, d.path, GetSelf<IResourceManager>()})) {
            CORE_LOG_W("Failed to save resource options");
            return false;
        }
        auto& v = resources_[ResourceGroupContext{id.group, resource->GetContext()}][id.name];
        if (v) {
            res = v->object;
        }
        v = BASE_NS::make_shared<ResourceData>(BASE_NS::move(d));
    }
    Notify({ResourceIdContext{id, resource->GetContext()}}, IResourceListener::EventType::ADDED);
    return true;
}
bool FileResourceManager::AddResource(const ResourceIdContext& ric, const CORE_NS::ResourceType& type,
    BASE_NS::string_view path, const CORE_NS::IResourceOptions::Ptr& os)
{
    CORE_NS::IResourceOptions::Ptr options =
        os ? os
           : META_NS::GetObjectRegistry().Create<CORE_NS::IResourceOptions>(META_NS::ClassId::ObjectResourceOptions);
    if (!options) {
        return false;
    }
    CORE_NS::IResource::Ptr res;
    {
        std::unique_lock lock{mutex_};
        auto it = types_.find(ResourceTypeKey{type, ""});
        if (it == types_.end()) {
            CORE_LOG_W("Resource type not registered [type=%s]", BASE_NS::to_string(type).c_str());
            return false;
        }
        ResourceData d;
        d.type = type;
        d.id = ric.id;
        d.path = path;
        d.options = options;
        auto& v = resources_[ResourceGroupContext{ric}][ric.id.name];
        if (v) {
            res = v->object;
        }
        v = BASE_NS::make_shared<ResourceData>(BASE_NS::move(d));
    }
    Notify({ric}, IResourceListener::EventType::ADDED);
    return true;
}
bool FileResourceManager::ReloadResource(const CORE_NS::IResource::Ptr& resource)
{
    return ReapplyOptions(resource, resource->GetContext().lock());
}
bool FileResourceManager::RenameResource(const ResourceIdContext& ric, const ResourceIdContext& newId)
{
    {
        std::unique_lock lock{mutex_};
        auto git = resources_.find(ResourceGroupContext{ric});
        if (git == resources_.end()) {
            return false;
        }
        auto it = git->second.find(ric.id.name);
        if (it == git->second.end()) {
            return false;
        }
        auto& v = resources_[ResourceGroupContext{newId}][newId.id.name];
        if (v) {
            return false;
        }
        v = it->second;
        git->second.erase(it);
    }

    Notify({ric}, IResourceListener::EventType::REMOVED);
    Notify({newId}, IResourceListener::EventType::ADDED);
    return true;
}
bool FileResourceManager::PurgeResource(const ResourceIdContext& ric)
{
    CORE_NS::IResource::Ptr obj;
    std::unique_lock lock{mutex_};
    if (auto res = FindResource(ric)) {
        std::swap(obj, res->object);
        return true;
    }
    CORE_LOG_W("No such resource: %s", ric.id.ToString().c_str());
    return false;
}
bool FileResourceManager::RemoveResource(const ResourceIdContext& ric)
{
    CORE_NS::IResource::Ptr obj;
    bool res = false;
    {
        std::unique_lock lock{mutex_};
        if (auto git = resources_.find(ResourceGroupContext{ric}); git != resources_.end()) {
            if (auto it = git->second.find(ric.id.name); it != git->second.end()) {
                if (it->second) {
                    obj = it->second->object;
                }
                git->second.erase(it);
                res = true;
            }
        }
    }
    if (res) {
        Notify({ric}, IResourceListener::EventType::REMOVED);
    }
    return res;
}
size_t FileResourceManager::PurgeGroup(BASE_NS::string_view group, const ResourceContextPtr& context)
{
    BASE_NS::vector<CORE_NS::IResource::Ptr> objs;
    size_t count = 0;
    std::unique_lock lock{mutex_};
    auto git = resources_.find(ResourceGroupContext{BASE_NS::string(group), context});
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
bool FileResourceManager::RemoveGroup(BASE_NS::string_view group, const ResourceContextPtr& context)
{
    return RemoveGroup(group, CORE_NS::ResourceContextWeakPtr(context));
}
bool FileResourceManager::RemoveGroup(BASE_NS::string_view group, const CORE_NS::ResourceContextWeakPtr& context)
{
    bool res = false;
    BASE_NS::vector<CORE_NS::IResource::Ptr> objs;
    BASE_NS::vector<ResourceIdContext> result;
    {
        ResourceGroupContext rgc{BASE_NS::string(group), context};
        std::unique_lock lock{mutex_};
        auto it = resources_.find(rgc);
        res = it != resources_.end();
        if (res) {
            for (auto&& g : it->second) {
                objs.push_back(g.second->object);
                result.push_back(ResourceIdContext{g.second->id, context});
            }
            resources_.erase(it);
        }
        ownedGroups_.erase(rgc);
    }
    if (res) {
        Notify(result, IResourceListener::EventType::REMOVED);
    }
    return res;
}
void FileResourceManager::RemoveAllResources()
{
    BASE_NS::vector<CORE_NS::IResource::Ptr> objs;
    {
        std::unique_lock lock{mutex_};
        for (auto&& g : resources_) {
            for (auto&& v : g.second) {
                objs.push_back(v.second->object);
            }
            ownedGroups_.erase(g.first);
        }
        resources_.clear();
    }
}
void FileResourceManager::RemoveAllResources(const ResourceContextPtr& context)
{
    BASE_NS::vector<CORE_NS::IResource::Ptr> objs;
    BASE_NS::vector<ResourceIdContext> result;
    {
        std::unique_lock lock{mutex_};
        BASE_NS::vector<ResourceGroupContext> groups;
        for (auto&& g : resources_) {
            if (IsCorrectContext(g.first, context)) {
                for (auto&& v : g.second) {
                    objs.push_back(v.second->object);
                    result.push_back({v.second->id, context});
                }
                ownedGroups_.erase(g.first);
                groups.push_back(g.first);
            }
        }
        for (auto&& v : groups) {
            resources_.erase(v);
        }
    }
    Notify(result, IResourceListener::EventType::REMOVED);
}

FileResourceManager::Result FileResourceManager::ExportResourcePayload(const CORE_NS::IResource::Ptr& resource)
{
    std::shared_lock lock{mutex_};
    auto res = FindResource(ResourceIdContext{resource->GetResourceId(), resource->GetContext()});
    if (!res) {
        CORE_LOG_W("No such resource: %s", resource->GetResourceId().ToString().c_str());
        return Result::NO_RESOURCE_DATA;
    }
    return SaveResourceData(*res);
}
void FileResourceManager::SetFileManager(CORE_NS::IFileManager::Ptr fileManager)
{
    std::unique_lock lock{mutex_};
    fileManager_ = BASE_NS::move(fileManager);
}
CORE_NS::IFileManager::Ptr FileResourceManager::GetFileManager() const
{
    std::shared_lock lock{mutex_};
    return fileManager_;
}

namespace {
struct ResourceLoadInfo {
    CORE_NS::IResourceType::ConstPtr type;
    BASE_NS::string path;
    CORE_NS::IResourceOptions::Ptr options;
    CORE_NS::IFileManager::Ptr fileManager;
};

CORE_NS::IResource::Ptr LoadResourceWithType(
    const CORE_NS::ResourceIdContext& ric, const ResourceLoadInfo& info, const CORE_NS::IResourceManager::Ptr& self)
{
    CORE_NS::IFile::Ptr f;
    if (!info.path.empty()) {
        f = info.fileManager->OpenFile(info.path);
        if (!f) {
            CORE_LOG_W("Failed to open resource file: %s", info.path.c_str());
            return nullptr;
        }
    }
    CORE_NS::IResource::Ptr resObj = info.type->LoadResource(
        CORE_NS::IResourceType::StorageInfo{info.options, f.get(), ric.id, info.path, self, ric.context.lock()});
    if (resObj) {
        if (auto i = interface_cast<CORE_NS::ISetResourceId>(resObj)) {
            auto rc = ric;
            rc.context = ResolveContext(ric.context.lock());
            i->SetResourceId(rc);
        }
    } else {
        CORE_LOG_W("Failed to load resource: %s", ric.id.ToString().c_str());
    }
    return resObj;
}
}  // namespace

CORE_NS::IResource::Ptr FileResourceManager::ConstructResource(const CORE_NS::ResourceIdContext& ric)
{
    BASE_NS::shared_ptr<ResourceData> res;
    ResourceLoadInfo info;
    {
        std::shared_lock lock{mutex_};
        res = FindResource(ric);
        if (res) {
            auto tit = types_.find(ResourceTypeKey{res->type, res->version});
            if (tit != types_.end()) {
                info.type = tit->second;
                info.path = res->path;
                info.options = res->options;
                info.fileManager = fileManager_;
            }
        }
    }

    CORE_NS::IResource::Ptr resObj;
    if (res && info.type) {
        resObj = LoadResourceWithType(ric, info, GetSelf<IResourceManager>());
    }

    if (res && resObj) {
        std::unique_lock lock{mutex_};
        if (res->object) {
            resObj = res->object;
        } else {
            res->object = resObj;
        }
    }
    return resObj;
}

IResourceGroupHandle::Ptr FileResourceManager::GetGroupHandle(
    BASE_NS::string_view group, const ResourceContextPtr& context)
{
    IResourceGroupHandle::Ptr res;
    std::unique_lock lock{mutex_};
    ResourceGroupContext gc{BASE_NS::string(group), context};
    auto it = ownedGroups_.find(gc);
    if (it != ownedGroups_.end()) {
        res = it->second.lock();
    }
    if (!res) {
        res = BASE_NS::make_shared<ResourceGroupHandle>(
            GetSelf<CORE_NS::IResourceManager>(), BASE_NS::string(group), context);
        ownedGroups_[gc] = res;
    }
    return res;
}

uint32_t FileResourceManager::GetAliveCount(
    const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection, const ResourceContextPtr& context) const
{
    return FindAliveResources(selection, context).size();
}

BASE_NS::vector<CORE_NS::IResource::Ptr> FileResourceManager::FindAliveResources(
    const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection, const ResourceContextPtr& context) const
{
    BASE_NS::vector<CORE_NS::IResource::Ptr> res;
    auto push = [&](const auto& r) {
        if (IsResourceMatch(selection, r.id)) {
            if (r.object) {
                res.push_back(r.object);
            }
        }
    };

    std::shared_lock lock{mutex_};
    for (auto&& g : resources_) {
        if (IsCorrectContext(g.first, context)) {
            for (auto&& v : g.second) {
                push(*v.second);
            }
        }
    }
    return res;
}

size_t FileResourceManager::GetResourceCount() const
{
    size_t res{};
    std::shared_lock lock{mutex_};
    for (auto&& g : resources_) {
        res += g.second.size();
    }
    return res;
}

META_REGISTER_CLASS(
    DependencyCollector, "b4d26357-6a82-4b0d-9f33-4b6caea3c05c", META_NS::ObjectCategoryBits::NO_CATEGORY)

class DependencyCollector : public IntroduceInterfaces<MinimalObject, ICollectResources, IResourceContext> {
    META_IMPLEMENT_OBJECT_TYPE_INTERFACE(ClassId::DependencyCollector)
public:
    DependencyCollector(ResourceContextPtr p) : context(BASE_NS::move(p))
    {}
    void AddResource(CORE_NS::ResourceIdContext mid) override
    {
        resources.push_back(BASE_NS::move(mid));
    }

    CORE_NS::ResourceContextPtr GetContext() const override
    {
        return context;
    }

    BASE_NS::vector<CORE_NS::ResourceIdContext> resources;
    ResourceContextPtr context;
};

void FileResourceManager::UpdateOptionsData(
    const BASE_NS::unordered_map<BASE_NS::string, BASE_NS::shared_ptr<ResourceData>>& data,
    const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection, const IObject::Ptr& depsContext)
{
    for (auto&& v : data) {
        if (IsResourceMatch(selection, v.second->id)) {
            UpdateOptions(*v.second, depsContext);
        }
    }
}

BASE_NS::vector<CORE_NS::ResourceIdContext> FileResourceManager::UpdateOptionsData(
    const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection, const ResourceContextPtr& context)
{
    DependencyCollector deps{context};
    IObject::Ptr depsContext(&deps, [](auto) {});
    std::unique_lock lock{mutex_};
    for (auto&& v : selection) {
        for (auto&& g : resources_) {
            if (IsCorrectContext(g.first, context)) {
                UpdateOptionsData(g.second, selection, depsContext);
            }
        }
    }
    return deps.resources;
}
BASE_NS::vector<CORE_NS::ResourceIdContext> FileResourceManager::UpdateOptionsData(
    const BASE_NS::array_view<const CORE_NS::ResourceIdContext>& res)
{
    BASE_NS::vector<CORE_NS::ResourceIdContext> list;
    std::unique_lock lock{mutex_};
    for (auto&& v : res) {
        if (auto r = FindResource(v)) {
            DependencyCollector deps{v.context.lock()};
            IObject::Ptr depsContext(&deps, [](auto) {});
            UpdateOptions(*r, depsContext);
            list.insert(list.end(), deps.resources.begin(), deps.resources.end());
        }
    }
    return list;
}
bool FileResourceManager::ReapplyOptions(
    const CORE_NS::IResource::Ptr& resource, const CORE_NS::ResourceContextPtr& context)
{
    CORE_NS::IFile::Ptr f;
    CORE_NS::IResourceOptions::Ptr options;
    BASE_NS::string path;
    CORE_NS::IResourceType::Ptr type;
    auto id = resource->GetResourceId();
    {
        std::unique_lock lock{mutex_};
        auto res = FindResource({id, resource->GetContext()});
        if (!res) {
            CORE_LOG_W("No such resource: %s", id.ToString().c_str());
            return false;
        }
        auto it = types_.find(ResourceTypeKey{res->type, res->version});
        if (it == types_.end()) {
            CORE_LOG_W(
                "Resource type not registered [type=%s]", BASE_NS::to_string(resource->GetResourceType()).c_str());
            return false;
        }
        type = it->second;
        path = res->path;
        if (!path.empty()) {
            f = fileManager_->OpenFile(path);
            if (!f) {
                CORE_LOG_W("Failed to open resource file: %s", path.c_str());
                return false;
            }
        }
        options = res->options;
    }
    CORE_NS::IResourceType::StorageInfo info{options, f.get(), id, path, GetSelf<IResourceManager>(), context};
    return type->ReloadResource(info, resource);
}

BASE_NS::vector<ResourceData> FileResourceManager::GetResources(
    const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection, const ResourceContextPtr& context) const
{
    BASE_NS::vector<ResourceData> res;
    std::shared_lock lock{mutex_};

    auto func = [&](auto& r) {
        if (IsResourceMatch(selection, r.id)) {
            res.push_back(r);
        }
    };
    for (auto&& g : resources_) {
        if (IsCorrectContext(g.first, context)) {
            for (auto&& v : g.second) {
                func(*v.second);
            }
        }
    }
    return res;
}

ResourceData FileResourceManager::GetResource(const CORE_NS::ResourceIdContext& ric) const
{
    std::shared_lock lock{mutex_};
    auto d = const_cast<FileResourceManager*>(this)->FindResource(ric);
    return d ? *d : ResourceData{};
}

bool FileResourceManager::AddResource(ResourceData data, const CORE_NS::ResourceContextPtr& context)
{
    ResourceIdContext id{data.id, context};
    CORE_NS::IResource::Ptr obj;
    {
        std::unique_lock lock{mutex_};
        auto it = types_.find(ResourceTypeKey{data.type, data.version});
        if (it == types_.end()) {
            CORE_LOG_W("Resource type not registered [type=%s, version=%s]",
                BASE_NS::to_string(data.type).c_str(),
                data.version.c_str());
            return false;
        } else {
            auto& v = resources_[ResourceGroupContext{data.id.group, context}][data.id.name];
            if (v && v->object != data.object) {
                obj = v->object;
            }
            v = BASE_NS::make_shared<ResourceData>(BASE_NS::move(data));
        }
    }
    Notify({id}, IResourceListener::EventType::ADDED);
    return true;
}

void FileResourceManager::AddResources(
    const BASE_NS::array_view<const ResourceData> list, const ResourceContextPtr& context)
{
    BASE_NS::vector<ResourceIdContext> ids;
    BASE_NS::vector<CORE_NS::IResource::Ptr> objs;
    {
        std::unique_lock lock{mutex_};
        for (auto&& data : list) {
            auto it = types_.find(ResourceTypeKey{data.type, data.version});
            if (it == types_.end()) {
                CORE_LOG_W("Resource type not registered [type=%s]", BASE_NS::to_string(data.type).c_str());
            } else {
                auto& v = resources_[ResourceGroupContext{data.id.group, context}][data.id.name];
                if (v && v->object != data.object) {
                    objs.push_back(v->object);
                }
                ids.push_back(ResourceIdContext{data.id, context});
                v = BASE_NS::make_shared<ResourceData>(data);
            }
        }
    }
    Notify(ids, IResourceListener::EventType::ADDED);
}

bool FileResourceManager::SetResourceName(const ResourceIdContext& id, const BASE_NS::string& name)
{
    std::unique_lock lock{mutex_};
    auto res = FindResource(id);
    if (res) {
        res->name = name;
    }
    return res != nullptr;
}

CORE_NS::IResource::Ptr FileResourceManager::GetResourceFromGroup(
    const BASE_NS::unordered_map<BASE_NS::string, BASE_NS::shared_ptr<ResourceData>>& resources,
    BASE_NS::string_view name, const ResourceContextPtr& context)
{
    for (auto&& res : resources) {
        if (res.second->name == name) {
            if (res.second->object) {
                return res.second->object;
            }
            return ConstructResource({res.second->id, context});
        }
    }
    return nullptr;
}

CORE_NS::IResource::Ptr FileResourceManager::GetResourceByName(
    BASE_NS::string_view name, const ResourceContextPtr& context)
{
    std::unique_lock lock{mutex_};
    for (auto&& g : resources_) {
        if (IsCorrectContext(g.first, context)) {
            if (auto r = GetResourceFromGroup(g.second, name, context)) {
                return r;
            }
        }
    }
    return nullptr;
}

META_END_NAMESPACE()
