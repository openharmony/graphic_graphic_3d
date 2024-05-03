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

#ifndef CORE__IO__FILEMANAGER_H
#define CORE__IO__FILEMANAGER_H

#include <cstdint>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/io/intf_directory.h>
#include <core/io/intf_file.h>
#include <core/io/intf_file_manager.h>
#include <core/io/intf_file_system.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class ProxyFilesystem;

class FileManager final : public IFileManager {
public:
    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;
    void Ref() override;
    void Unref() override;

    FileManager();
    ~FileManager() override = default;

    IDirectory::Entry GetEntry(BASE_NS::string_view uri) override;

    IFile::Ptr OpenFile(BASE_NS::string_view uri) override;
    IFile::Ptr CreateFile(BASE_NS::string_view uri) override;

    bool DeleteFile(BASE_NS::string_view uri) override;

    IDirectory::Ptr OpenDirectory(BASE_NS::string_view uri) override;
    IDirectory::Ptr CreateDirectory(BASE_NS::string_view uri) override;
    bool DeleteDirectory(BASE_NS::string_view uri) override;

    bool Rename(BASE_NS::string_view fromUri, BASE_NS::string_view toUri) override;

    void RegisterFilesystem(BASE_NS::string_view protocol, IFilesystem::Ptr filesystem) override;
    void UnregisterFilesystem(BASE_NS::string_view protocol) override;

    void RegisterAssetPath(BASE_NS::string_view uri) override;
    void UnregisterAssetPath(BASE_NS::string_view uri) override;

    BASE_NS::vector<BASE_NS::string> GetAbsolutePaths(BASE_NS::string_view uri) const;

    bool RegisterPath(BASE_NS::string_view protocol, BASE_NS::string_view uri, bool prepend) override;
    void UnregisterPath(BASE_NS::string_view protocol, BASE_NS::string_view uri) override;

    virtual IFilesystem::Ptr CreateROFilesystem(const void* const data, uint64_t size) override;

private:
    // NOTE: Johannes Pystynen 2019/10/25, Faster access when protocol is known.
    IFilesystem* GetFilesystem(BASE_NS::string_view protocol) const;

    // Fix "invalid" uris (converts relative file:// -> absolute. does not affect other protocols.)
    BASE_NS::string FixPath(BASE_NS::string_view pathIn) const;

    // Current working directory for the "file://" protocol, used to resolve the absolute path.
    BASE_NS::string basePath_;

    BASE_NS::unordered_map<BASE_NS::string, IFilesystem::Ptr> filesystems_;

    BASE_NS::unordered_map<BASE_NS::string, ProxyFilesystem*> proxyFilesystems_;

    uint32_t refCount_ { 0 };
};
CORE_END_NAMESPACE()

#endif // CORE__IO__FILEMANAGER_H
