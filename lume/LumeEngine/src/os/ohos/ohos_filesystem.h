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

#ifndef CORE__IO__OHOS_FILESYSTEM_H
#define CORE__IO__OHOS_FILESYSTEM_H

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <base/containers/unordered_map.h>
#include <base/namespace.h>
#include <core/io/intf_directory.h>
#include <core/io/intf_file.h>
#include <core/io/intf_file_system.h>
#include <core/namespace.h>

#include "ohos_file.h"

CORE_BEGIN_NAMESPACE()
class FileManager;

class OhosFilesystem final : public IFilesystem {
public:
    OhosFilesystem(const BASE_NS::string_view hapPath, const BASE_NS::string_view bundleName,
        const BASE_NS::string_view moduleName);
    OhosFilesystem() = delete;
    OhosFilesystem(OhosFilesystem const&) = delete;
    OhosFilesystem& operator=(OhosFilesystem const&) = delete;
    ~OhosFilesystem() override = default;

    IDirectory::Entry GetEntry(BASE_NS::string_view path) override;
    IFile::Ptr CreateFile(BASE_NS::string_view path) override;
    bool DeleteFile(BASE_NS::string_view path) override;
    IDirectory::Ptr OpenDirectory(BASE_NS::string_view path) override;
    IDirectory::Ptr CreateDirectory(BASE_NS::string_view path) override;
    bool DeleteDirectory(BASE_NS::string_view path) override;
    bool Rename(BASE_NS::string_view fromPath, BASE_NS::string_view toPath) override;
    BASE_NS::vector<BASE_NS::string> GetUriPaths(BASE_NS::string_view uri) const override;
    IFile::Ptr OpenFile(BASE_NS::string_view path) override;

protected:
    BASE_NS::string ValidatePath(const BASE_NS::string_view pathIn) const;
    void Destroy() override
    {
        delete this;
    }

    PlatformHapInfo hapInfo_;
    BASE_NS::unordered_map<BASE_NS::string, std::weak_ptr<OhosFileStorage>> ohosFiles_;
    BASE_NS::refcnt_ptr<OhosResMgr> resManager_ = nullptr;
};
CORE_END_NAMESPACE()
#endif
