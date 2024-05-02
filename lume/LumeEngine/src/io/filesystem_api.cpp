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

#include <cstddef>
#include <cstdint>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/type_traits.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <base/util/uid.h>
#include <core/implementation_uids.h>
#include <core/io/intf_directory.h>
#include <core/io/intf_file_manager.h>
#include <core/io/intf_file_monitor.h>
#include <core/io/intf_file_system.h>
#include <core/io/intf_filesystem_api.h>
#include <core/namespace.h>
#include <core/plugin/intf_class_factory.h>
#include <core/plugin/intf_interface.h>
#include <core/plugin/intf_plugin.h>

#include "dev/file_monitor.h"
#include "io/file_manager.h"
#include "io/memory_filesystem.h"
#include "io/path_tools.h"
#include "io/rofs_filesystem.h"
#include "io/std_filesystem.h"
#include "util/string_util.h"

CORE_BEGIN_NAMESPACE();
namespace {
using BASE_NS::string;
using BASE_NS::string_view;
using BASE_NS::Uid;
using BASE_NS::vector;

class FileMonitorImpl final : public IFileMonitor {
public:
    const IInterface* GetInterface(const Uid& uid) const override;
    IInterface* GetInterface(const Uid& uid) override;
    void Ref() override;
    void Unref() override;

    void Initialize(IFileManager&) override;
    bool AddPath(const string_view path) override;
    bool RemovePath(const string_view path) override;
    void ScanModifications(vector<string>& added, vector<string>& removed, vector<string>& modified) override;

    FileMonitorImpl() = default;
    explicit FileMonitorImpl(IFileManager& manager)
    {
        Initialize(manager);
    }

    BASE_NS::unique_ptr<FileMonitor> fileMonitor_;
    uint32_t refCount_ { 0 };
};

class FilesystemApi final : public IFileSystemApi {
public:
    string basePath_;
    IFilesystem::Ptr rootFs_;
    FilesystemApi() : basePath_(GetCurrentDirectory()), rootFs_(CreateStdFileSystem()) {}

    const IInterface* GetInterface(const Uid& uid) const override
    {
        return const_cast<FilesystemApi*>(this)->GetInterface(uid);
    }
    IInterface* GetInterface(const Uid& uid) override
    {
        if ((uid == IFileSystemApi::UID) || (uid == IClassFactory::UID) || (uid == IInterface::UID)) {
            return this;
        }
        return nullptr;
    }
    void Ref() override {}
    void Unref() override {}

    IInterface::Ptr CreateInstance(const Uid& uid) override
    {
        if (uid == UID_FILE_MONITOR) {
            return IInterface::Ptr(new FileMonitorImpl());
        }
        if (uid == UID_FILE_MANAGER) {
            return IInterface::Ptr(new FileManager());
        }
        return IInterface::Ptr();
    }

    IFileManager::Ptr CreateFilemanager() override
    {
        return IInterface::Ptr(new FileManager());
    }
    IFileMonitor::Ptr CreateFilemonitor(IFileManager& manager) override
    {
        return IInterface::Ptr(new FileMonitorImpl(manager));
    }
    string ResolvePath(string_view inPathRaw) const
    {
#if _WIN32
        string_view curDrive;
        string_view curPath;
        string_view curFilename;
        string_view curExt;
        SplitPath(basePath_, curDrive, curPath, curFilename, curExt);

        if (inPathRaw.empty()) {
            return {};
        }
        // fix slashes. (just change \\ to /)
        string_view pathIn = inPathRaw;
        string tmp;
        if (pathIn.find("\\") != string_view::npos) {
            tmp = pathIn;
            StringUtil::FindAndReplaceAll(tmp, "\\", "/");
            pathIn = tmp;
        }

        string_view drive;
        string_view path;
        string_view filename;
        string_view ext;
        SplitPath(pathIn, drive, path, filename, ext);
        string res = "/";
        if (drive.empty()) {
            // relative to current drive then
            res += curDrive;
        } else {
            res += drive;
        }
        res += ":";
        string normalizedPath;
        if (path.empty()) {
            return "";
        }
        if (path[0] != '/') {
            // relative path.
            normalizedPath = NormalizePath(curPath + path);
        } else {
            normalizedPath = NormalizePath(path);
        }
        if (normalizedPath.empty()) {
            return "";
        }
        return res + normalizedPath;
#else
        if (IsRelative(inPathRaw)) {
            return NormalizePath(basePath_ + inPathRaw);
        }
        return NormalizePath(inPathRaw);
#endif
    }

    IFilesystem::Ptr CreateStdFileSystem() override
    {
        return IFilesystem::Ptr(new StdFilesystem("/"));
    }

    IFilesystem::Ptr CreateStdFileSystem(string_view rootPathIn) override
    {
        string_view protocol;
        string_view path;
        if (ParseUri(rootPathIn, protocol, path)) {
            if (protocol != "file") {
                return {};
            }
            rootPathIn = path;
        }
        auto rootPath = ResolvePath(rootPathIn);
        if (!rootPath.empty()) {
            auto entry = rootFs_->GetEntry(rootPath);
            if (entry.type == IDirectory::Entry::DIRECTORY) {
                return IFilesystem::Ptr(new StdFilesystem(rootPath));
            }
        }
        return {};
    }
    IFilesystem::Ptr CreateMemFileSystem() override
    {
        return IFilesystem::Ptr(new MemoryFilesystem());
    }
    IFilesystem::Ptr CreateROFilesystem(const void* const data, uint64_t size) override
    {
        return IFilesystem::Ptr { new RoFileSystem(data, static_cast<size_t>(size)) };
    }
};
} // namespace

const IInterface* FileMonitorImpl::GetInterface(const Uid& uid) const
{
    if ((uid == IFileMonitor::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

IInterface* FileMonitorImpl::GetInterface(const Uid& uid)
{
    if ((uid == IFileMonitor::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

void FileMonitorImpl::Ref()
{
    refCount_++;
}

void FileMonitorImpl::Unref()
{
    if (--refCount_ == 0) {
        delete this;
    }
}

void FileMonitorImpl::Initialize(IFileManager& manager)
{
    fileMonitor_ = BASE_NS::make_unique<FileMonitor>(manager);
}

bool FileMonitorImpl::AddPath(const string_view path)
{
    if (fileMonitor_) {
        return fileMonitor_->AddPath(path);
    }
    return false;
}

bool FileMonitorImpl::RemovePath(const string_view path)
{
    if (fileMonitor_) {
        return fileMonitor_->RemovePath(path);
    }
    return false;
}

void FileMonitorImpl::ScanModifications(vector<string>& added, vector<string>& removed, vector<string>& modified)
{
    added.clear();
    removed.clear();
    modified.clear();
    if (fileMonitor_) {
        fileMonitor_->ScanModifications(added, removed, modified);
    }
}

IInterface* CreateFileMonitor(IClassFactory& /* registry */, PluginToken /* token */)
{
    return new FileMonitorImpl();
}
IInterface* GetFileApiFactory(IClassRegister& /* registry */, PluginToken /* token */)
{
    static FilesystemApi fact;
    return &fact;
}
CORE_END_NAMESPACE()
