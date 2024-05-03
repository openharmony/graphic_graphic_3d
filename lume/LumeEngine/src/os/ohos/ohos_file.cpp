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

#include "ohos_file.h"

#include <cstdint>

#ifdef __has_include
#if __has_include(<filesystem>)
#include <filesystem>
#define HAS_FILESYSTEM
#endif
#endif

#ifndef HAS_FILESYSTEM
#include <cerrno>
#include <dirent.h>

#ifndef _DIRENT_HAVE_D_TYPE
#include <sys/stat.h>
#include <sys/types.h>

#endif
#include <climits>
#define CORE_MAX_PATH PATH_MAX
#endif

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <core/io/intf_file.h>
#include <core/log.h>
#include <core/namespace.h>

#include "io/std_directory.h"

CORE_BEGIN_NAMESPACE()
using BASE_NS::CloneData;
using BASE_NS::string;
using BASE_NS::string_view;

const std::regex MEDIA_RES_ID_REGEX(R"(^\w+/([0-9]+)\.\w+$)", std::regex::icase);
const std::regex MEDIA_HAP_RES_PATH_REGEX(R"(^(.*)$)");
const std::regex MEDIA_HAP_RES_ID_REGEX(R"(^.*/([0-9]+)\.\w+$)", std::regex::icase);
const std::regex MEDIA_RES_NAME_REGEX(R"(^.*/(\w+)\.\w+$)", std::regex::icase);

constexpr uint32_t OHOS_RESOURCE_MATCH_SIZE = 2;

void OhosResMgr::UpdateResManager(const PlatformHapInfo& hapInfo)
{
    auto key = hapInfo.bundleName + "+" + hapInfo.moduleName;
    auto resourceMgrIter = resourceManagers_.find(key);
    if (resourceMgrIter != resourceManagers_.end()) {
        resourceManager_ = resourceMgrIter->second;
        return;
    }
    std::shared_ptr<OHOS::Global::Resource::ResourceManager>
        newResMgr(OHOS::Global::Resource::CreateResourceManager());
    auto resRet = newResMgr->AddResource(hapInfo.hapPath.c_str());
    resourceManagers_[key] = newResMgr;
    resourceManager_ = newResMgr;
}

std::shared_ptr<OHOS::Global::Resource::ResourceManager> OhosResMgr::GetResMgr() const
{
    return resourceManager_;
}

OhosFileDirectory::OhosFileDirectory(BASE_NS::refcnt_ptr<OhosResMgr> resMgr) : dirResMgr_(resMgr) {}

OhosFileDirectory::~OhosFileDirectory()
{
    Close();
}

void OhosFileDirectory::Close()
{
    if (dir_) {
        dir_.reset();
    }
}

bool OhosFileDirectory::IsDir(BASE_NS::string_view path, std::vector<std::string>& fileList) const
{
    auto state = dirResMgr_->GetResMgr()->GetRawFileList(path.data(), fileList);
    if (state != OHOS::Global::Resource::SUCCESS || fileList.empty()) {
        CORE_LOG_E("GetRawfilepath error, filename:%s, error:%u", path.data(), state);
        return false;
    }
    return true;
}

bool OhosFileDirectory::IsFile(BASE_NS::string_view path) const
{
    std::unique_ptr<uint8_t[]> data;
    size_t dataLen = 0;
    auto state = dirResMgr_->GetResMgr()->GetRawFileFromHap(path.data(), dataLen, data);
    if (state != OHOS::Global::Resource::SUCCESS) {
        return false;
    }
    return true;
}

bool OhosFileDirectory::Open(const BASE_NS::string_view pathIn)
{
    auto path = pathIn;
    if (path.back() == '/') {
        path.remove_suffix(1);
    }
    if (path.front() == '/') {
        path.remove_prefix(1);
    }
    std::vector<std::string> fileList;
    if (IsDir(path, fileList)) {
        dir_ = BASE_NS::make_unique<OhosDirImpl>(path, fileList);
        return true;
    }
    return false;
}

BASE_NS::vector<IDirectory::Entry> OhosFileDirectory::GetEntries() const
{
    CORE_ASSERT_MSG(dir_, "Dir not open");
    BASE_NS::vector<IDirectory::Entry> result;
    if (dir_) {
        for (int i = 0; i < static_cast<int>(dir_->fileList_.size()); i++) {
            auto path = dir_->path_ + "/" + BASE_NS::string(dir_->fileList_[i].c_str());
            auto entry = GetEntry(path);
            entry.timestamp = static_cast<uint32_t>(i);
            entry.name = dir_->fileList_[i].c_str();
            result.emplace_back(entry);
        }
    }
    return result;
}

IDirectory::Entry OhosFileDirectory::GetEntry(BASE_NS::string_view uriIn) const
{
    if (!uriIn.empty()) {
        IDirectory::Entry::Type type;
        std::vector<std::string> fileList;
        if (IsFile(uriIn)) {
            type = IDirectory::Entry::FILE;
        } else if (IsDir(uriIn, fileList)) {
            type = IDirectory::Entry::DIRECTORY;
        } else {
            type = IDirectory::Entry::UNKNOWN;
        }
        // timestamp set 0
        uint64_t timestamp = 0;
        BASE_NS::string entryName { uriIn };
        return IDirectory::Entry { type, entryName, timestamp };
    }
    return {};
}

OhosFile::OhosFile(BASE_NS::refcnt_ptr<OhosResMgr> resMgr) : fileResMgr_(resMgr)
{
    buffer_ = std::make_shared<OhosFileStorage>(nullptr);
}

void OhosFile::UpdateStorage(std::shared_ptr<OhosFileStorage> buffer)
{
    buffer_ = BASE_NS::move(buffer);
}

IFile::Mode OhosFile::GetMode() const
{
    return IFile::Mode::READ_ONLY;
}

void OhosFile::Close() {}

uint64_t OhosFile::Read(void* buffer, uint64_t count)
{
    uint64_t toRead = count;
    uint64_t sum = index_ + toRead;
    if (sum < index_) {
        return 0;
    }
    if (sum > buffer_->Size()) {
        toRead = buffer_->Size() - index_;
    }
    if (toRead <= 0) {
        return toRead;
    }
    if (toRead > SIZE_MAX) {
        CORE_ASSERT_MSG(false, "Unable to read chunks bigger than (SIZE_MAX) bytes.");
        return 0;
    }
    if (CloneData(buffer, static_cast<size_t>(count), &(buffer_->GetStorage()[index_]),
                static_cast<size_t>(toRead))) {
        index_ += toRead;
    }
    return toRead;
}

uint64_t OhosFile::Write(const void* buffer, uint64_t count)
{
    return 0;
}

uint64_t OhosFile::GetLength() const
{
    return buffer_->Size();
}

bool OhosFile::Seek(uint64_t aOffset)
{
    if (aOffset < buffer_->Size()) {
        index_ = aOffset;
        return true;
    }
    return false;
}

uint64_t OhosFile::GetPosition() const
{
    return index_;
}

std::shared_ptr<OhosFileStorage> OhosFile::Open(BASE_NS::string_view rawFile)
{
    std::unique_ptr<uint8_t[]> data;
    size_t dataLen = 0;
    if (OpenRawFile(rawFile, dataLen, data)) {
        buffer_->SetBuffer(std::move(data), static_cast<uint64_t>(dataLen));
        return buffer_;
    }
    return nullptr;
}

// Parsing URI
bool OhosFile::GetResourceId(const std::string& uri, uint32_t& resId) const
{
    std::smatch matches;
    if (std::regex_match(uri, matches, MEDIA_RES_ID_REGEX) && matches.size() == OHOS_RESOURCE_MATCH_SIZE) {
        resId = static_cast<uint32_t>(std::stoul(matches[1].str()));
        return true;
    }
    std::smatch hapMatches;
    if (std::regex_match(uri, hapMatches, MEDIA_HAP_RES_ID_REGEX) && hapMatches.size() == OHOS_RESOURCE_MATCH_SIZE) {
        resId = static_cast<uint32_t>(std::stoul(hapMatches[1].str()));
        return true;
    }
    return false;
}

bool OhosFile::GetResourceId(const std::string& uri, std::string& path) const
{
    std::smatch matches;
    if (std::regex_match(uri, matches, MEDIA_HAP_RES_PATH_REGEX) && matches.size() == OHOS_RESOURCE_MATCH_SIZE) {
        path = matches[1].str();
        return true;
    }
    return false;
}

bool OhosFile::GetResourceName(const std::string& uri, std::string& resName) const
{
    std::smatch matches;
    if (std::regex_match(uri, matches, MEDIA_RES_NAME_REGEX) && matches.size() == OHOS_RESOURCE_MATCH_SIZE) {
        resName = matches[1].str();
        return true;
    }
    return false;
}

bool OhosFile::OpenRawFile(BASE_NS::string_view uriIn, size_t& dataLen, std::unique_ptr<uint8_t[]>& dest)
{
    std::string uri(uriIn.data());
    std::string rawFile;
    if (GetResourceId(uri, rawFile)) {
        auto state = fileResMgr_->GetResMgr()->GetRawFileFromHap(rawFile.c_str(), dataLen, dest);
        if (state != OHOS::Global::Resource::SUCCESS || !dest) {
            CORE_LOG_E("GetRawFileFromHap error, raw filename:%s, error:%u", rawFile.c_str(), state);
            return false;
        }
        return true;
    }
    uint32_t resId = 0;
    if (GetResourceId(uri, resId)) {
        auto state = fileResMgr_->GetResMgr()->GetMediaDataById(resId, dataLen, dest);
        if (state != OHOS::Global::Resource::SUCCESS || !dest) {
            CORE_LOG_E("GetMediaDataById error, resId:%u, error:%u", resId, state);
            return false;
        }
        return true;
    }
    std::string resName;
    if (GetResourceName(uri, resName)) {
        auto state = fileResMgr_->GetResMgr()->GetMediaDataByName(resName.c_str(), dataLen, dest);
        if (state != OHOS::Global::Resource::SUCCESS || !dest) {
            CORE_LOG_E("GetMediaDataByName error, resName:%s, error:%u", resName.c_str(), state);
            return false;
        }
        return true;
    }
    CORE_LOG_E("load image data failed, as uri is invalid:%s", uri.c_str());
    return false;
}
CORE_END_NAMESPACE()
