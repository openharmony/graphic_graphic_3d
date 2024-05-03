/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 *
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software

 * * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.
 * See the License for the specific language governing permissions and
 * limitations
 * under the License.
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
    if (!path.empty()) {
        // remove suffix -> '/'
        string temp;
        if (path.back() == '/') {
            size_t len = path.length();
            temp = path.substr(0, len - 1);
        }
        // remove the '/' slash, which is not used in ResourceMgr
        return string(temp.substr(1));
    }
    return path;
}

IFile::Ptr OhosFilesystem::OpenFile(const BASE_NS::string_view path)
{
    if (auto const pos = ohosFiles_.find(path); pos != ohosFiles_.end()) {
        auto storage = pos->second.lock();
        if (storage) {
            auto file = BASE_NS::make_unique<OhosFile>(resManager_);
            file->UpdateStorage(storage);
            return IFile::Ptr { file.release() };
        }
    }
    auto storage = std::make_shared<OhosFileStorage>(nullptr);
    auto file = BASE_NS::make_unique<OhosFile>(resManager_);
    if (!path.empty()) {
        storage = file->Open(path.data());
    }
    if (storage == nullptr) {
        return IFile::Ptr();
    }
    ohosFiles_[path] = std::move(storage);
    return IFile::Ptr { file.release() };
}

OhosFilesystem::OhosFilesystem(const BASE_NS::string_view hapPath, const BASE_NS::string_view bundleName,
    const BASE_NS::string_view moduleName) : hapInfo_({ hapPath, bundleName, moduleName })
{
    resManager_ = BASE_NS::refcnt_ptr<OhosResMgr>(new OhosResMgr(hapInfo_));
    resManager_->UpdateResManager(hapInfo_);
}

IDirectory::Entry OhosFilesystem::GetEntry(BASE_NS::string_view path)
{
    if (!path.empty()) {
        auto directory = BASE_NS::make_unique<OhosFileDirectory>(resManager_);
        return directory->GetEntry(path.data());
    }
    return {};
}

IFile::Ptr OhosFilesystem::CreateFile(BASE_NS::string_view path)
{
    return IFile::Ptr();
}

bool OhosFilesystem::DeleteFile(BASE_NS::string_view path)
{
    return ohosFiles_.erase(path) != 0U;
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
    return false;
}

bool OhosFilesystem::Rename(BASE_NS::string_view fromPath, BASE_NS::string_view toPath)
{
    return false;
}

BASE_NS::vector<BASE_NS::string> OhosFilesystem::GetUriPaths(BASE_NS::string_view uri) const
{
    return {};
}
CORE_END_NAMESPACE()
