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

#ifndef CORE__IO__OHOS_FILE_H
#define CORE__IO__OHOS_FILE_H

#include <atomic>
#include <fstream>
#include <iostream>
#include <memory>
#include <regex>

#include <base/containers/refcnt_ptr.h>
#include <base/containers/string_view.h>
#include <base/containers/type_traits.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <core/io/intf_directory.h>
#include <core/io/intf_file.h>
#include <core/namespace.h>

#include "base/global/resource_management/interfaces/inner_api/include/resource_manager.h"

CORE_BEGIN_NAMESPACE()
struct OhosDirImpl {
    OhosDirImpl(const BASE_NS::string_view path, std::vector<std::string> fileList) : path_(path), fileList_(fileList)
    {}

    BASE_NS::string path_;
    std::vector<std::string> fileList_;
};

struct PlatformHapInfo {
    PlatformHapInfo(const BASE_NS::string_view hapPath, const BASE_NS::string_view bundleName,
        const BASE_NS::string_view moduleName) : hapPath(hapPath), bundleName(bundleName), moduleName(moduleName) {}
    BASE_NS::string hapPath = " ";
    BASE_NS::string bundleName = " ";
    BASE_NS::string moduleName = " ";
};

class OhosResMgr {
public:
    explicit OhosResMgr(const PlatformHapInfo& hapInfo) : hapInfo_(hapInfo) {}
    std::shared_ptr<OHOS::Global::Resource::ResourceManager> GetResMgr() const;
    void UpdateResManager(const PlatformHapInfo& hapInfo);
    void Ref()
    {
        refCount_.fetch_add(1, std::memory_order_relaxed);
    }
    void Unref()
    {
        if (refCount_.fetch_sub(1, std::memory_order_release) == 1) {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete this;
        }
    }

private:
    std::atomic_int32_t refCount_ { 0 };
    PlatformHapInfo hapInfo_;
    std::shared_ptr<OHOS::Global::Resource::ResourceManager> resourceManager_  { nullptr };
    BASE_NS::unordered_map<BASE_NS::string,
        std::shared_ptr<OHOS::Global::Resource::ResourceManager>> resourceManagers_;
};

class OhosFileDirectory final : public IDirectory {
public:
    explicit OhosFileDirectory(BASE_NS::refcnt_ptr<OhosResMgr> resMgr);
    ~OhosFileDirectory() override;

    bool Open(BASE_NS::string_view path);
    void Close() override;
    BASE_NS::vector<Entry> GetEntries() const override;
    // new add
    IDirectory::Entry GetEntry(BASE_NS::string_view uriIn) const;
    bool IsFile(BASE_NS::string_view path) const;
    bool IsDir(BASE_NS::string_view path, std::vector<std::string>& fileList) const;

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    BASE_NS::unique_ptr<OhosDirImpl> dir_;
    BASE_NS::refcnt_ptr<OhosResMgr> dirResMgr_;
};

class OhosFileStorage {
public:
    explicit OhosFileStorage(std::unique_ptr<uint8_t[]> buffer) : buffer_(std::move(buffer)) {}
    ~OhosFileStorage() = default;

    const std::unique_ptr<uint8_t[]>& GetStorage() const
    {
        return buffer_;
    }

    size_t Size() const
    {
        return size_;
    }

    void SetBuffer(std::unique_ptr<uint8_t[]> buffer, size_t size)
    {
        buffer_ = BASE_NS::move(buffer);
        size_ = size;
    }

private:
    std::unique_ptr<uint8_t[]> buffer_;
    size_t size_ { 0 };
};

class OhosFile final : public IFile {
public:
    explicit OhosFile(BASE_NS::refcnt_ptr<OhosResMgr> resMgr);
    ~OhosFile() override = default;

    Mode GetMode() const override;
    // Open an existing file, fails if the file does not exist.
    std::shared_ptr<OhosFileStorage> Open(BASE_NS::string_view path);
    // Close file.
    void Close() override;
    uint64_t Read(void* buffer, uint64_t count) override;
    uint64_t Write(const void* buffer, uint64_t count) override;
    uint64_t GetLength() const override;
    bool Seek(uint64_t offset) override;
    uint64_t GetPosition() const override;
    bool OpenRawFile(BASE_NS::string_view src, size_t& len, std::unique_ptr<uint8_t[]>& dest);
    void UpdateStorage(std::shared_ptr<OhosFileStorage> buffer);

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    bool GetResourceId(const std::string& uri, uint32_t& resId) const;
    bool GetResourceId(const std::string& uri, std::string& path) const;
    bool GetResourceName(const std::string& uri, std::string& resName) const;

    uint64_t index_ { 0 };
    std::shared_ptr<OhosFileStorage> buffer_;
    BASE_NS::refcnt_ptr<OhosResMgr> fileResMgr_;
};
CORE_END_NAMESPACE()
#endif
