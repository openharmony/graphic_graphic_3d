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

#include "io/file_manager.h"

#include <algorithm>
#include <cstddef>

#include <base/containers/iterator.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/type_traits.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <base/util/uid.h>
#include <core/io/intf_directory.h>
#include <core/io/intf_file.h>
#include <core/io/intf_file_manager.h>
#include <core/io/intf_file_system.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/plugin/intf_interface.h>

#include "io/path_tools.h"
#include "io/proxy_filesystem.h"
#include "io/rofs_filesystem.h"
#include "io/std_directory.h"

CORE_BEGIN_NAMESPACE()
using BASE_NS::make_unique;
using BASE_NS::string;
using BASE_NS::string_view;
using BASE_NS::Uid;
using BASE_NS::vector;

string FileManager::FixPath(string_view pathIn) const
{
    string_view protocol;
    string_view path;
    if (ParseUri(pathIn, protocol, path)) {
        // Try to identify relative "file" uris, and convert them to absolute.
        if (protocol == "file") {
            if (path.empty()) {
                // so it's the base path then? (empty relative path)
                return protocol + "://" + basePath_;
            }
#if _WIN32
            // Handle win32 specific drive letters.
            if (IsRelative(path)) {
                // might still be absolute (if it has drive letter)
                if ((path.size() > 1) && (path[1] == ':')) {
                    // seems to start with drive letter so, it must be absolute?
                    if (path.size() == 2) { // 2: path size
                        // has only drive letter? consider it as root of drive then
                        return protocol + ":///" + path + "/";
                    }
                    return protocol + ":///" + path;
                }
                // no drive letter so it's really relative.
                return protocol + "://" + NormalizePath(basePath_ + path);
            }
            // Even if it's "absolute" it might still be missing the drive letter.
            if ((path.size() < 3) || (path[2] != ':')) { // 3: path size limit; 2: the third letter
                // seems to be missing the drive letter.
                return protocol + "://" + NormalizePath(basePath_.substr(0, 3) + path); // 3: substring size
            }
            if (path.size() == 3) { // 3: path size
                // has only drive letter? consider it as root of drive then
                return protocol + "://" + path + "/";
            }
            return protocol + "://" + NormalizePath(path);
#else
            if (IsRelative(path)) {
                // normalize it with current path..
                return protocol + "://" + NormalizePath(basePath_ + path);
            }
            return protocol + "://" + NormalizePath(path);
#endif
        }
    }
    return string(pathIn);
}

FileManager::FileManager() : basePath_(GetCurrentDirectory()) {}

const IInterface* FileManager::GetInterface(const Uid& uid) const
{
    return const_cast<FileManager*>(this)->GetInterface(uid);
}

IInterface* FileManager::GetInterface(const Uid& uid)
{
    if ((uid == IInterface::UID) || (uid == IFileManager::UID)) {
        return this;
    }
    return nullptr;
}

void FileManager::Ref()
{
    refCount_++;
}

void FileManager::Unref()
{
    if (--refCount_ == 0) {
        delete this;
    }
}

IFile::Ptr FileManager::OpenFile(const string_view uriIn)
{
    string_view protocol;
    string_view path;
    auto uri = FixPath(uriIn);
    if (ParseUri(uri, protocol, path)) {
        IFilesystem* filesystem = GetFilesystem(protocol);
        if (filesystem) {
            return filesystem->OpenFile(path);
        }
        CORE_LOG_E("Failed to open file, no file system for uri: '%s'", string(uri).c_str());
    } else {
        CORE_LOG_E("Failed to open file, invalid uri: '%s'", string(uri).c_str());
    }

    return IFile::Ptr();
}

IFile::Ptr FileManager::CreateFile(const string_view uriIn)
{
    string_view protocol;
    string_view path;
    auto uri = FixPath(uriIn);
    if (ParseUri(uri, protocol, path)) {
        IFilesystem* filesystem = GetFilesystem(protocol);
        if (filesystem) {
            return filesystem->CreateFile(path);
        }
        CORE_LOG_E("Failed to create file, no file system for uri: '%s'", string(uri).c_str());
    } else {
        CORE_LOG_E("Failed to create file, invalid uri: '%s'", string(uri).c_str());
    }

    return IFile::Ptr();
}

bool FileManager::DeleteFile(const string_view uriIn)
{
    string_view protocol;
    string_view path;
    auto uri = FixPath(uriIn);
    if (ParseUri(uri, protocol, path)) {
        IFilesystem* filesystem = GetFilesystem(protocol);
        if (filesystem) {
            return filesystem->DeleteFile(path);
        }
    }

    return false;
}

bool FileManager::Rename(const string_view fromUri, const string_view toUri)
{
    string_view fromProtocol;
    string_view fromPath;
    auto from = FixPath(fromUri);
    if (ParseUri(from, fromProtocol, fromPath)) {
        string_view toProtocol;
        string_view toPath;
        auto to = FixPath(toUri);
        if (ParseUri(to, toProtocol, toPath)) {
            if (fromProtocol == toProtocol) {
                IFilesystem* filesystem = GetFilesystem(fromProtocol);
                if (filesystem) {
                    return filesystem->Rename(fromPath, toPath);
                }
            } else {
                CORE_LOG_E("Rename requires both uris have same protocol");
            }
        }
    }

    return false;
}

IDirectory::Entry FileManager::GetEntry(const string_view uriIn)
{
    string_view protocol;
    string_view path;
    auto uri = FixPath(uriIn);
    if (ParseUri(uri, protocol, path)) {
        IFilesystem* filesystem = GetFilesystem(protocol);
        if (filesystem) {
            return filesystem->GetEntry(path);
        }
        CORE_LOG_E("Failed to get entry for uri, no file system for uri: '%s'", string(uri).c_str());
    } else {
        CORE_LOG_E("Failed to get entry for uri, invalid uri: '%s'", string(uri).c_str());
    }

    return {};
}
IDirectory::Ptr FileManager::OpenDirectory(const string_view uriIn)
{
    string_view protocol;
    string_view path;
    auto uri = FixPath(uriIn);
    if (ParseUri(uri, protocol, path)) {
        IFilesystem* filesystem = GetFilesystem(protocol);
        if (filesystem) {
            return filesystem->OpenDirectory(path);
        }
        CORE_LOG_E("Failed to open directory, no file system for uri: '%s'", string(uri).c_str());
    } else {
        CORE_LOG_E("Failed to open directory, invalid uri: '%s'", string(uri).c_str());
    }

    return IDirectory::Ptr();
}

IDirectory::Ptr FileManager::CreateDirectory(const string_view uriIn)
{
    string_view protocol;
    string_view path;
    auto uri = FixPath(uriIn);
    if (ParseUri(uri, protocol, path)) {
        IFilesystem* filesystem = GetFilesystem(protocol);
        if (filesystem) {
            return filesystem->CreateDirectory(path);
        }
        CORE_LOG_E("Failed to create directory, no file system for uri: '%s'", string(uri).c_str());
    } else {
        CORE_LOG_E("Failed to create directory, invalid uri: '%s'", string(uri).c_str());
    }

    return IDirectory::Ptr();
}

bool FileManager::DeleteDirectory(const string_view uriIn)
{
    string_view protocol;
    string_view path;
    auto uri = FixPath(uriIn);
    if (ParseUri(uri, protocol, path)) {
        IFilesystem* filesystem = GetFilesystem(protocol);
        if (filesystem) {
            return filesystem->DeleteDirectory(path);
        }
    }

    return false;
}

void FileManager::RegisterFilesystem(const string_view protocol, IFilesystem::Ptr filesystem)
{
    CORE_ASSERT_MSG(filesystems_.find(protocol) == filesystems_.cend(), "File system already registered");

    filesystems_[protocol] = move(filesystem);
}

void FileManager::UnregisterFilesystem(const string_view protocol)
{
    const auto iterator = filesystems_.find(protocol);
    if (iterator != filesystems_.end()) {
        filesystems_.erase(iterator);
    }
}

void FileManager::RegisterAssetPath(const string_view uriIn)
{
    auto uri = FixPath(uriIn);
    RegisterPath("assets", uri, false);
}

void FileManager::UnregisterAssetPath(const string_view uriIn)
{
    auto uri = FixPath(uriIn);
    UnregisterPath("assets", uri);
}

vector<string> FileManager::GetAbsolutePaths(const string_view uriIn) const
{
    vector<string> ret;
    string_view protocol;
    string_view path;
    auto uri = FixPath(uriIn);
    if (ParseUri(uri, protocol, path)) {
        const IFilesystem* filesystem = GetFilesystem(protocol);
        if (filesystem) {
            // a single URI path can be found in several paths in a proxy filesystem
            auto uriPaths = filesystem->GetUriPaths(path);
            for (auto& uriPath : uriPaths) {
                if (uriPath.find("file://") == string::npos) {
                    auto tmp = GetAbsolutePaths(uriPath);
                    ret.insert(ret.cend(), tmp.cbegin(), tmp.cend());
                } else {
                    ret.push_back(move(uriPath));
                }
            }
        }
    }
    std::transform(ret.begin(), ret.end(), ret.begin(), [](const string& uri) {
        string_view protocol;
        string_view path;
        if (ParseUri(uri, protocol, path)) {
            return StdDirectory::ResolveAbsolutePath(path, true);
        }
        return uri;
    });

    return ret;
}

bool FileManager::RegisterPath(const string_view protocol, const string_view uriIn, bool prepend)
{
    auto uri = FixPath(uriIn);
    // Check if the proxy protocol exists already.
    auto it = proxyFilesystems_.find(protocol);
    if (it != proxyFilesystems_.end()) {
        // Yes, add the new search path to it.
        if (prepend) {
            it->second->PrependSearchPath(uri);
        } else {
            it->second->AppendSearchPath(uri);
        }
        return true;
    }

    // Check if the protocol is already declared..
    const auto itp = filesystems_.find(protocol);
    if (itp != filesystems_.end()) {
        // Okay there is a protocol handler already, we can't add paths to non-proxy protocols.
        CORE_LOG_W("Tried to register a path to non-proxy filesystem. protocol [%s] uriIn [%s]",
            string(protocol).c_str(), string(uriIn).c_str());
        return false;
    }

    // Create new proxy protocol handler.
    auto pfs = make_unique<ProxyFilesystem>(*this, uri);
    proxyFilesystems_[protocol] = pfs.get();
    RegisterFilesystem(protocol, IFilesystem::Ptr { pfs.release() });
    return true;
}

void FileManager::UnregisterPath(const string_view protocol, const string_view uriIn)
{
    auto uri = FixPath(uriIn);
    auto it = proxyFilesystems_.find(protocol);
    if (it != proxyFilesystems_.end()) {
        it->second->RemoveSearchPath(uri);
    }
}

IFilesystem* FileManager::GetFilesystem(const string_view protocol) const
{
    const auto it = filesystems_.find(protocol);
    if (it != filesystems_.end()) {
        return it->second.get();
    }

    return nullptr;
}

IFilesystem::Ptr FileManager::CreateROFilesystem(const void* const data, uint64_t size)
{
    return IFilesystem::Ptr { new RoFileSystem(data, static_cast<size_t>(size)) };
}
CORE_END_NAMESPACE()
