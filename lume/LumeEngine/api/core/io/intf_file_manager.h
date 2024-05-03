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

#ifndef API_CORE_IO_IFILE_MANAGER_H
#define API_CORE_IO_IFILE_MANAGER_H

#include <cstdint>

#include <base/containers/refcnt_ptr.h>
#include <base/containers/string_view.h>
#include <base/namespace.h>
#include <base/util/uid.h>
#include <core/io/intf_directory.h>
#include <core/io/intf_file.h>
#include <core/io/intf_file_system.h>
#include <core/namespace.h>
#include <core/plugin/intf_interface.h>

CORE_BEGIN_NAMESPACE()

/** @ingroup group_io_ifilemanager */
/**
 * File Manager.
 * Provides basic file I/O across different file systems and protocols.
 */
class IFileManager : public IInterface {
public:
    static constexpr auto UID = BASE_NS::Uid { "5507e02b-b900-44e4-a969-0eda2d0918ac" };

    using Ptr = BASE_NS::refcnt_ptr<IFileManager>;

    /** Get uri information.
     *  @param uri Absolute uri, such as '%file://logs/log.txt'
     *  @return IDirectory::Entry defining entry type , size and timestamp.
     */
    virtual IDirectory::Entry GetEntry(BASE_NS::string_view uri) = 0;

    /** Opens file in given uri.
     *  @param uri The complete file uri, such as '%file://images/texture.png'
     *  @return Read-only file that was opened, empty ptr if file cannot be opened.
     */
    virtual IFile::Ptr OpenFile(BASE_NS::string_view uri) = 0;

    /** Creates a new file to given uri.
     *  @param uri The complete file uri, such as '%file://logs/log.txt'
     *  @return File that was created, empty ptr if file cannot be created.
     */
    virtual IFile::Ptr CreateFile(BASE_NS::string_view uri) = 0;

    /** Delete file.
     *  @param uri The complete file uri, such as '%file://logs/log.txt'
     *  @return True if deletion is successful, otherwise false.
     */
    virtual bool DeleteFile(BASE_NS::string_view uri) = 0;

    /** Opens directory in given uri.
     *  @param uri The complete directory uri, such as '%file://images/'
     *  @return Directory that was opened, empty ptr if directory cannot be opened.
     */
    virtual IDirectory::Ptr OpenDirectory(BASE_NS::string_view uri) = 0;

    /** Creates a new directory to given uri.
     *  @param uri The complete directory uri, such as '%file://logs/'
     *  @return Directory that was created, empty ptr if directory cannot be created.
     */
    virtual IDirectory::Ptr CreateDirectory(BASE_NS::string_view uri) = 0;

    /** Delete directory.
     *  @param uri The complete directory uri, such as '%file://logs/'
     *  @return True if deletion is successful, otherwise false.
     */
    virtual bool DeleteDirectory(BASE_NS::string_view uri) = 0;

    /** Rename file or directory
     *  @param fromUri The complete uri for file/directory to rename, such as '%file://logs/old.txt'
     *  @param toUri The complete file uri for destination name, such as '%file://logs/new.txt'
     *  @return True if rename is successful, otherwise false.
     */
    virtual bool Rename(BASE_NS::string_view fromUri, BASE_NS::string_view toUri) = 0;

    /** Register a new custom filesystem. Filesystem data can be accessed using the functions in this file manager using
     *  an uri with the registered protocol string.
     *  @param protocol protocol string that is used to point to the filesystem.
     *  @param filesystem file system object that implements the filesystem.
     */
    virtual void RegisterFilesystem(BASE_NS::string_view protocol, IFilesystem::Ptr filesystem) = 0;

    /** Unregister a filesystem.
     *  @param protocol protocol string that is used to point to the filesystem.
     */
    virtual void UnregisterFilesystem(BASE_NS::string_view protocol) = 0;

    /** Register an uri that will be mapped to the "assets:" protocol. Each mapped uri will be searched for assets in
     *  the order they are added.
     *  @param uri Path to assets.
     */
    virtual void RegisterAssetPath(BASE_NS::string_view uri) = 0;

    /** Unregister an asset path.
     *  @param uri Path to assets.
     */
    virtual void UnregisterAssetPath(BASE_NS::string_view uri) = 0;

    /** Register uri with given protocol for assets usage
     *  @param protocol Protocol to be registered
     *  @param uri Uri where protocol points
     *  @param prepend If true, adds path to begin of search list
     */
    virtual bool RegisterPath(BASE_NS::string_view protocol, BASE_NS::string_view uri, bool prepend) = 0;

    /**
     * Unregister uri from given protocol
     * @param protocol A protocol where to remove search path
     * @param uri Uri which is removed from paths
     */
    virtual void UnregisterPath(BASE_NS::string_view protocol, BASE_NS::string_view uri) = 0;

    virtual IFilesystem::Ptr CreateROFilesystem(const void* const data, uint64_t size) = 0;

protected:
    IFileManager() = default;
    virtual ~IFileManager() = default;
};

inline constexpr BASE_NS::string_view GetName(const IFileManager*)
{
    return "IFileManager";
}
CORE_END_NAMESPACE()

#endif // API_CORE_IO_IFILE_MANAGER_H
