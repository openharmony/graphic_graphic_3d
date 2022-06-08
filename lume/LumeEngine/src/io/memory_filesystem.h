/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE__IO__MEMORY_FILESYSTEM_H
#define CORE__IO__MEMORY_FILESYSTEM_H

#include <base/containers/unordered_map.h>
#include <core/io/intf_file_system.h>
#include <core/namespace.h>

#include "memory_file.h"

CORE_BEGIN_NAMESPACE()

/** Memory protocol.
 * Protocol implementation that wraps a buffer in memory.
 */
class MemoryFilesystem final : public IFilesystem {
public:
    MemoryFilesystem() = default;
    ~MemoryFilesystem() override = default;
    MemoryFilesystem(MemoryFilesystem const&) = delete;
    MemoryFilesystem& operator=(MemoryFilesystem const&) = delete;

    IDirectory::Entry GetEntry(BASE_NS::string_view uri) override;
    IFile::Ptr OpenFile(BASE_NS::string_view path) override;
    IFile::Ptr CreateFile(BASE_NS::string_view path) override;
    bool DeleteFile(BASE_NS::string_view path) override;

    IDirectory::Ptr OpenDirectory(BASE_NS::string_view path) override;
    IDirectory::Ptr CreateDirectory(BASE_NS::string_view path) override;
    bool DeleteDirectory(BASE_NS::string_view path) override;

    bool Rename(BASE_NS::string_view fromPath, BASE_NS::string_view toPath) override;

    BASE_NS::vector<BASE_NS::string> GetUriPaths(BASE_NS::string_view uri) const override;

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    BASE_NS::unordered_map<BASE_NS::string, std::weak_ptr<MemoryFileStorage>> memoryFiles_;
};
CORE_END_NAMESPACE()

#endif // CORE__IO__MEMORY_FILESYSTEM_H
