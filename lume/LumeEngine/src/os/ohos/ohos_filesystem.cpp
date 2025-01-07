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

#include "ohos_filesystem.h"

#include <algorithm>
#include <cstring>

#include <base/containers/iterator.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/type_traits.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/io/intf_directory.h>
#include <core/io/intf_file.h>
#include <core/log.h>
#include <core/namespace.h>

#include "io/file_manager.h"
#include "io/path_tools.h"
#include "io/proxy_directory.h"

CORE_BEGIN_NAMESPACE()
using BASE_NS::move;
using BASE_NS::string;
using BASE_NS::string_view;
using BASE_NS::vector;
string OhosFilesystem::ValidatePath(const string_view pathIn) const
{
    auto path = NormalizePath(pathIn);
    if (!path.empty() && (path.back() == '/')) {
        // remove suffix -> '/'
        size_t len = path.length();
        path.resize(len - 1U);
    }
    // remove the '/' slash, which is not used in ResourceMgr
    if (!path.empty() && path.front() == '/') {
        path.erase(path.cbegin());
    }
    return path;
}

IFile::Ptr OhosFilesystem::OpenFile(const BASE_NS::string_view path, const IFile::Mode mode)
{
    if (mode == IFile::Mode::READ_ONLY) {
        if (auto const pos = ohosFiles_.find(path); pos != ohosFiles_.end()) {
            auto storage = pos->second.lock();
            if (storage) {
                auto file = BASE_NS::make_unique<OhosFile>(resManager_);
                file->UpdateStorage(storage);
                return IFile::Ptr { file.release() };
            }
        }
        if (!path.empty()) {
            auto file = BASE_NS::make_unique<OhosFile>(resManager_);
            auto storage = file->Open(path.data());
            ohosFiles_[path] = std::move(storage);
            return IFile::Ptr { file.release() };
        }
    }
    return IFile::Ptr();
}

OhosFilesystem::OhosFilesystem(
    const BASE_NS::string_view hapPath, const BASE_NS::string_view bundleName, const BASE_NS::string_view moduleName,
    std::shared_ptr<OHOS::Global::Resource::ResourceManager> resourceManager)
    : hapInfo_({hapPath, bundleName, moduleName, resourceManager})
{
    resManager_ = BASE_NS::refcnt_ptr<OhosResMgr>(new OhosResMgr(hapInfo_));
    resManager_->UpdateResManager(hapInfo_);
}

IDirectory::Entry OhosFilesystem::GetEntry(BASE_NS::string_view path) const
{
    if (!path.empty()) {
        auto directory = BASE_NS::make_unique<OhosFileDirectory>(resManager_);
        return directory->GetEntry(path.data());
    }
    return {};
}

IDirectory::Entry OhosFilesystem::GetEntry(BASE_NS::string_view path)
{
    const auto& ofs = *this;
    return ofs.GetEntry(path);
}

IFile::Ptr OhosFilesystem::CreateFile(BASE_NS::string_view path)
{
    return IFile::Ptr();
}

bool OhosFilesystem::DeleteFile(BASE_NS::string_view path)
{
    // read only filesystem. can not delete files.
    return false;
}

bool OhosFilesystem::FileExists(const string_view path) const
{
    return GetEntry(path).type == IDirectory::Entry::Type::FILE;
}

IDirectory::Ptr OhosFilesystem::OpenDirectory(BASE_NS::string_view pathIn)
{
    auto path = ValidatePath(pathIn);
    auto directory = BASE_NS::make_unique<OhosFileDirectory>(resManager_);
    if (directory->Open(path.data())) {
        return IDirectory::Ptr { directory.release() };
    }
    return IDirectory::Ptr();
}

IDirectory::Ptr OhosFilesystem::CreateDirectory(BASE_NS::string_view path)
{
    return IDirectory::Ptr();
}

bool OhosFilesystem::DeleteDirectory(BASE_NS::string_view path)
{
    // read only filesystem. can not delete files.
    return false;
}

bool OhosFilesystem::DirectoryExists(const string_view path) const
{
    return GetEntry(path).type == IDirectory::Entry::Type::DIRECTORY;
}

bool OhosFilesystem::Rename(BASE_NS::string_view fromPath, BASE_NS::string_view toPath)
{
    // read only filesystem. can not rename files.
    return false;
}

BASE_NS::vector<BASE_NS::string> OhosFilesystem::GetUriPaths(BASE_NS::string_view uri) const
{
    return {};
}
CORE_END_NAMESPACE()
