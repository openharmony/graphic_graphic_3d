/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_CORE_IO_IFILESYSTEM_H
#define API_CORE_IO_IFILESYSTEM_H

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <core/io/intf_directory.h>
#include <core/io/intf_file.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
/** @ingroup group_io_ifilesystem */
/** IFilesystem */
class IFilesystem {
public:
    /** Get directory entry from uri
     *  @param uri Path to file
     *  @return IDirectory::Entry defining entry type , size and timestamp.
     */
    virtual IDirectory::Entry GetEntry(BASE_NS::string_view uri) = 0;

    /** Opens file from designated path
     *  @param path Path to file
     */
    virtual IFile::Ptr OpenFile(BASE_NS::string_view path) = 0;

    /** Creates file to given path
     *  @param path Path where file is created
     */
    virtual IFile::Ptr CreateFile(BASE_NS::string_view path) = 0;

    /** Deletes file from given path
     *  @param path Path to file
     */
    virtual bool DeleteFile(BASE_NS::string_view path) = 0;

    /** Opens directory
     *  @param path Path to directory for opening
     */
    virtual IDirectory::Ptr OpenDirectory(BASE_NS::string_view path) = 0;

    /** Creates a directory
     *  @param path Path where directory is created
     */
    virtual IDirectory::Ptr CreateDirectory(BASE_NS::string_view path) = 0;

    /** Deletes directory from given path
     *  @param path Path to directory
     */
    virtual bool DeleteDirectory(BASE_NS::string_view path) = 0;

    /** Renames file or directory from given path to another path
     *  @param fromPath Path to file or directory to be renamed
     *  @param toPath Path with destination name
     */
    virtual bool Rename(BASE_NS::string_view fromPath, BASE_NS::string_view toPath) = 0;

    /** Resolves all files which uri might be pointing to
     *  @param uri Uri to resolve
     */
    virtual BASE_NS::vector<BASE_NS::string> GetUriPaths(BASE_NS::string_view uri) const = 0;

    struct Deleter {
        constexpr Deleter() noexcept = default;
        void operator()(IFilesystem* ptr) const
        {
            ptr->Destroy();
        }
    };
    using Ptr = BASE_NS::unique_ptr<IFilesystem, Deleter>;

protected:
    IFilesystem() = default;
    virtual ~IFilesystem() = default;
    virtual void Destroy() = 0;
};
CORE_END_NAMESPACE()

#endif // API_CORE_IO_IFILESYSTEM_H
