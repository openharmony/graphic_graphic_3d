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

#include "proxy_filesystem.h"

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
#include <core/namespace.h>

#include "file_manager.h"
#include "path_tools.h"
#include "proxy_directory.h"

CORE_BEGIN_NAMESPACE()
using BASE_NS::move;
using BASE_NS::string;
using BASE_NS::string_view;
using BASE_NS::vector;

ProxyFilesystem::ProxyFilesystem(FileManager& fileManager, const string_view destination)
    : fileManager_(fileManager), destinations_()
{
    AppendSearchPath(destination);
}

void ProxyFilesystem::AppendSearchPath(string_view path)
{
    if (path.back() == '/') {
        path.remove_suffix(1);
    }
    destinations_.emplace_back(path);
}

void ProxyFilesystem::PrependSearchPath(string_view path)
{
    if (path.back() == '/') {
        path.remove_suffix(1);
    }
    destinations_.emplace(destinations_.begin(), path);
}

void ProxyFilesystem::RemoveSearchPath(string_view destination)
{
    if (destination.back() == '/') {
        destination.remove_suffix(1);
    }
    const auto it = std::find(destinations_.cbegin(), destinations_.cend(), destination);
    if (it != destinations_.cend()) {
        destinations_.erase(it);
    }
}

IDirectory::Entry ProxyFilesystem::GetEntry(const string_view path)
{
    auto normalizedPath = NormalizePath(path);
    if (!normalizedPath.empty()) {
        for (auto&& destination : destinations_) {
            auto file = fileManager_.GetEntry(destination + normalizedPath);
            if (file.type != IDirectory::Entry::UNKNOWN) {
                return file;
            }
        }
    }
    return {};
}
IFile::Ptr ProxyFilesystem::OpenFile(const string_view path)
{
    auto normalizedPath = NormalizePath(path);
    if (!normalizedPath.empty()) {
        for (auto&& destination : destinations_) {
            auto file = fileManager_.OpenFile(destination + normalizedPath);
            if (file) {
                return file;
            }
        }
    }

    return IFile::Ptr();
}

IFile::Ptr ProxyFilesystem::CreateFile(const string_view path)
{
    auto normalizedPath = NormalizePath(path);
    if (!normalizedPath.empty()) {
        for (auto&& destination : destinations_) {
            auto file = fileManager_.CreateFile(destination + normalizedPath);
            if (file) {
                return file;
            }
        }
    }

    return IFile::Ptr();
}

bool ProxyFilesystem::DeleteFile(const string_view path)
{
    auto normalizedPath = NormalizePath(path);
    if (!normalizedPath.empty()) {
        for (auto&& destination : destinations_) {
            if (fileManager_.DeleteFile(destination + normalizedPath)) {
                return true;
            }
        }
    }

    return false;
}

IDirectory::Ptr ProxyFilesystem::OpenDirectory(const string_view path)
{
    IDirectory::Ptr proxyDirectory;
    vector<IDirectory::Ptr> directories;
    auto normalizedPath = NormalizePath(path);
    if (!normalizedPath.empty()) {
        for (auto&& destination : destinations_) {
            auto directory = fileManager_.OpenDirectory(destination + normalizedPath);
            if (directory) {
                directories.push_back(move(directory));
            }
        }
    }

    if (!directories.empty()) {
        proxyDirectory.reset(new ProxyDirectory(move(directories)));
    }

    return proxyDirectory;
}

IDirectory::Ptr ProxyFilesystem::CreateDirectory(const string_view path)
{
    auto normalizedPath = NormalizePath(path);
    if (!normalizedPath.empty()) {
        for (auto&& destination : destinations_) {
            auto directory = fileManager_.CreateDirectory(destination + normalizedPath);
            if (directory) {
                return directory;
            }
        }
    }

    return IDirectory::Ptr();
}

bool ProxyFilesystem::DeleteDirectory(const string_view path)
{
    auto normalizedPath = NormalizePath(path);
    if (!normalizedPath.empty()) {
        for (auto&& destination : destinations_) {
            if (fileManager_.DeleteDirectory(destination + normalizedPath)) {
                return true;
            }
        }
    }

    return false;
}

bool ProxyFilesystem::Rename(const string_view fromPath, const string_view toPath)
{
    if (!fromPath.empty() && !toPath.empty()) {
        auto pathFrom = NormalizePath(fromPath);
        auto pathTo = NormalizePath(toPath);
        for (auto&& destination : destinations_) {
            if (fileManager_.Rename(destination + pathFrom, destination + pathTo)) {
                return true;
            }
        }
    }

    return false;
}

vector<string> ProxyFilesystem::GetUriPaths(const string_view uri) const
{
    vector<string> paths;

    auto path = NormalizePath(uri);
    if (!path.empty()) {
        for (auto&& destination : destinations_) {
            auto directory = fileManager_.OpenDirectory(destination + path);
            if (directory) {
                paths.push_back(destination + path);
            }
        }
    }

    return paths;
}
CORE_END_NAMESPACE()
