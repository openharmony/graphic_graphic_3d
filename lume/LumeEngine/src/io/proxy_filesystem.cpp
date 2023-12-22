/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

void ProxyFilesystem::AppendSearchPath(const string_view destination)
{
    auto dst = destination;
    if (dst.back() == '/') {
        dst = dst.substr(0, dst.size() - 1);
    }
    destinations_.emplace_back(dst);
}

void ProxyFilesystem::PrependSearchPath(const string_view uri)
{
    auto dst = uri;
    if (dst.back() == '/') {
        dst = dst.substr(0, dst.size() - 1);
    }
    destinations_.emplace(destinations_.begin(), dst);
}

void ProxyFilesystem::RemoveSearchPath(const string_view destination)
{
    auto dst = destination;
    if (dst.back() == '/') {
        dst = dst.substr(0, dst.size() - 1);
    }
    const auto it = std::find(destinations_.begin(), destinations_.end(), dst);
    if (it != destinations_.end()) {
        destinations_.erase(it);
    }
}

IDirectory::Entry ProxyFilesystem::GetEntry(const string_view pathIn)
{
    auto path = NormalizePath(pathIn);
    if (!path.empty()) {
        for (auto&& destination : destinations_) {
            auto file = fileManager_.GetEntry(destination + path);
            if (file.type != IDirectory::Entry::UNKNOWN) {
                return file;
            }
        }
    }
    return {};
}
IFile::Ptr ProxyFilesystem::OpenFile(const string_view pathIn)
{
    auto path = NormalizePath(pathIn);
    if (!path.empty()) {
        for (auto&& destination : destinations_) {
            auto file = fileManager_.OpenFile(destination + path);
            if (file) {
                return file;
            }
        }
    }

    return IFile::Ptr();
}

IFile::Ptr ProxyFilesystem::CreateFile(const string_view pathIn)
{
    auto path = NormalizePath(pathIn);
    if (!path.empty()) {
        for (auto&& destination : destinations_) {
            auto file = fileManager_.CreateFile(destination + path);
            if (file) {
                return file;
            }
        }
    }

    return IFile::Ptr();
}

bool ProxyFilesystem::DeleteFile(const string_view pathIn)
{
    auto path = NormalizePath(pathIn);
    if (!path.empty()) {
        for (auto&& destination : destinations_) {
            if (fileManager_.DeleteFile(destination + path)) {
                return true;
            }
        }
    }

    return false;
}

IDirectory::Ptr ProxyFilesystem::OpenDirectory(const string_view pathIn)
{
    IDirectory::Ptr proxyDirectory;
    vector<IDirectory::Ptr> directories;
    auto path = NormalizePath(pathIn);
    if (!path.empty()) {
        for (auto&& destination : destinations_) {
            auto directory = fileManager_.OpenDirectory(destination + path);
            if (directory) {
                directories.emplace_back(move(directory));
            }
        }
    }

    if (!directories.empty()) {
        proxyDirectory.reset(new ProxyDirectory(move(directories)));
    }

    return proxyDirectory;
}

IDirectory::Ptr ProxyFilesystem::CreateDirectory(const string_view pathIn)
{
    auto path = NormalizePath(pathIn);
    if (!path.empty()) {
        for (auto&& destination : destinations_) {
            auto directory = fileManager_.CreateDirectory(destination + path);
            if (directory) {
                return directory;
            }
        }
    }

    return IDirectory::Ptr();
}

bool ProxyFilesystem::DeleteDirectory(const string_view pathIn)
{
    auto path = NormalizePath(pathIn);
    if (!path.empty()) {
        for (auto&& destination : destinations_) {
            if (fileManager_.DeleteDirectory(destination + path)) {
                return true;
            }
        }
    }

    return false;
}

bool ProxyFilesystem::Rename(const string_view pathFrom, const string_view pathTo)
{
    auto fromPath = NormalizePath(pathFrom);
    auto toPath = NormalizePath(pathTo);
    if (!pathFrom.empty() && !pathTo.empty()) {
        for (auto&& destination : destinations_) {
            if (fileManager_.Rename(destination + fromPath, destination + toPath)) {
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
                paths.emplace_back(destination + path);
            }
        }
    }

    return paths;
}
CORE_END_NAMESPACE()
