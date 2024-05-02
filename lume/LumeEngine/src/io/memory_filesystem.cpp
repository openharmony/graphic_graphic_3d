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

#include "memory_filesystem.h"

#include <memory>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/io/intf_directory.h>
#include <core/io/intf_file.h>
#include <core/namespace.h>

#include "memory_file.h"

CORE_BEGIN_NAMESPACE()
using BASE_NS::string;
using BASE_NS::string_view;
using BASE_NS::vector;

IDirectory::Entry MemoryFilesystem::GetEntry(const string_view path)
{
    if (auto const pos = memoryFiles_.find(path); pos != memoryFiles_.end()) {
        return { IDirectory::Entry::FILE, string(path), 0 };
    }
    return {};
}
IFile::Ptr MemoryFilesystem::OpenFile(const string_view path)
{
    if (auto const pos = memoryFiles_.find(path); pos != memoryFiles_.end()) {
        auto storage = pos->second.lock();
        if (storage) {
            return IFile::Ptr { new MemoryFile(BASE_NS::move(storage)) };
        }
    }
    return IFile::Ptr();
}

IFile::Ptr MemoryFilesystem::CreateFile(const string_view path)
{
    if (auto const pos = memoryFiles_.find(path); pos != memoryFiles_.end()) {
        return IFile::Ptr { new MemoryFile(pos->second.lock()) };
    }
    auto storage = std::make_shared<MemoryFileStorage>();
    memoryFiles_[path] = storage;

    return IFile::Ptr { new MemoryFile(BASE_NS::move(storage)) };
}

bool MemoryFilesystem::DeleteFile(const string_view path)
{
    return memoryFiles_.erase(path) != 0u;
}

IDirectory::Ptr MemoryFilesystem::OpenDirectory(const string_view /* path */)
{
    return IDirectory::Ptr();
}

IDirectory::Ptr MemoryFilesystem::CreateDirectory(const string_view /* path */)
{
    return IDirectory::Ptr();
}

bool MemoryFilesystem::DeleteDirectory(const string_view /* path */)
{
    return false;
}

bool MemoryFilesystem::Rename(const string_view /* fromPath */, const string_view /* toPath */)
{
    return false;
}

vector<string> MemoryFilesystem::GetUriPaths(const string_view) const
{
    return {};
}
CORE_END_NAMESPACE()
