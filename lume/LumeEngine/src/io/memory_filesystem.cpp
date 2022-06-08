/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "memory_filesystem.h"

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
    return memoryFiles_.erase(path) != memoryFiles_.end();
}

IDirectory::Ptr MemoryFilesystem::OpenDirectory(const string_view path)
{
    return IDirectory::Ptr();
}

IDirectory::Ptr MemoryFilesystem::CreateDirectory(const string_view path)
{
    return IDirectory::Ptr();
}

bool MemoryFilesystem::DeleteDirectory(const string_view path)
{
    return false;
}

bool MemoryFilesystem::Rename(const string_view fromPath, const string_view toPath)
{
    return false;
}

vector<string> MemoryFilesystem::GetUriPaths(const string_view) const
{
    return {};
}
CORE_END_NAMESPACE()
