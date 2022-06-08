/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE__IO__STD_FILESYSTEM_H
#define CORE__IO__STD_FILESYSTEM_H

#include <core/io/intf_file_system.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
/** File protocol.
 * Protocol implementation that wraps standard file i/o.
 */
class StdFilesystem final : public IFilesystem {
public:
    // base path must be absolute, start with a "/" and on windows contain the drive letter "c:".
    // and should end on a "/". must use a "/" as directory separator.
    explicit StdFilesystem(BASE_NS::string_view basePath);
    ~StdFilesystem() override = default;
    StdFilesystem(StdFilesystem const&) = delete;
    StdFilesystem& operator=(StdFilesystem const&) = delete;

    // All uris should be absolute, and are considered such. (should start with "/")
    IDirectory::Entry GetEntry(BASE_NS::string_view uri) override;
    IFile::Ptr OpenFile(BASE_NS::string_view path) override;
    IFile::Ptr CreateFile(BASE_NS::string_view path) override;
    bool DeleteFile(BASE_NS::string_view path) override;

    IDirectory::Ptr OpenDirectory(BASE_NS::string_view path) override;
    IDirectory::Ptr CreateDirectory(BASE_NS::string_view path) override;
    bool DeleteDirectory(BASE_NS::string_view path) override;

    bool Rename(BASE_NS::string_view fromPath, BASE_NS::string_view toPath) override;

    BASE_NS::vector<BASE_NS::string> GetUriPaths(BASE_NS::string_view uri) const override;

protected:
    BASE_NS::string ValidatePath(BASE_NS::string_view path) const;
    void Destroy() override
    {
        delete this;
    }
    BASE_NS::string basePath_;
};
CORE_END_NAMESPACE()

#endif // CORE__IO__STD_FILESYSTEM_H
