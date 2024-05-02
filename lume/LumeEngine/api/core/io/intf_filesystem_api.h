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

#ifndef API_CORE_IFILE_SYSTEM_API_H
#define API_CORE_IFILE_SYSTEM_API_H

#include <cstdint>

#include <base/containers/string_view.h>
#include <base/namespace.h>
#include <base/util/uid.h>
#include <core/io/intf_file_manager.h>
#include <core/io/intf_file_monitor.h>
#include <core/io/intf_file_system.h>
#include <core/namespace.h>
#include <core/plugin/intf_class_factory.h>

CORE_BEGIN_NAMESPACE()

/** @ingroup group_ifilesystemapifactory */
/** file system api factory interface
 *  Helpers to create
 *  Usage:
 */

class IFileSystemApi : public IClassFactory {
public:
    static constexpr auto UID = BASE_NS::Uid { "dfe630b1-04ce-4c20-ac0d-7a521369dfbe" };

    virtual IFileManager::Ptr CreateFilemanager() = 0;
    virtual IFileMonitor::Ptr CreateFilemonitor(IFileManager&) = 0;
    virtual IFilesystem::Ptr CreateStdFileSystem() = 0;
    virtual IFilesystem::Ptr CreateStdFileSystem(BASE_NS::string_view rootPath) = 0;
    virtual IFilesystem::Ptr CreateMemFileSystem() = 0;
    virtual IFilesystem::Ptr CreateROFilesystem(const void* const data, uint64_t size) = 0;

protected:
    IFileSystemApi() = default;
    virtual ~IFileSystemApi() = default;
};

inline constexpr BASE_NS::string_view GetName(const IFileSystemApi*)
{
    return "IFileSystemApi";
}
CORE_END_NAMESPACE()

#endif // API_CORE_IFILE_SYSTEM_API_H
