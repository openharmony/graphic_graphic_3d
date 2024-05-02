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

#ifndef CORE__IO__MEMORY_FILESYSTEM_H
#define CORE__IO__MEMORY_FILESYSTEM_H

#include <memory>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/io/intf_directory.h>
#include <core/io/intf_file.h>
#include <core/io/intf_file_system.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class MemoryFileStorage;

/** Memory protocol.
 * Protocol implementation that wraps a buffer in memory.
 */
class MemoryFilesystem final : public IFilesystem {
public:
    MemoryFilesystem() = default;
    ~MemoryFilesystem() override = default;
    MemoryFilesystem(MemoryFilesystem const&) = delete;
    MemoryFilesystem& operator=(MemoryFilesystem const&) = delete;

    IDirectory::Entry GetEntry(BASE_NS::string_view path) override;
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
