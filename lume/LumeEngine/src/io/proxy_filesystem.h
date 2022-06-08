/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE__IO__PROXY_FILESYSTEM_H
#define CORE__IO__PROXY_FILESYSTEM_H

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <core/io/intf_file_system.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class FileManager;

/** File protocol.
 * Protocol implementation that uses given protocol as proxy to destination uri, for example app:// to point in to
 * application working directory.
 */
class ProxyFilesystem final : public IFilesystem {
public:
    ProxyFilesystem(FileManager& fileManager, BASE_NS::string_view destination);

    ProxyFilesystem() = delete;
    ProxyFilesystem(ProxyFilesystem const&) = delete;
    ProxyFilesystem& operator=(ProxyFilesystem const&) = delete;

    ~ProxyFilesystem() override = default;

    void RemoveSearchPath(BASE_NS::string_view destination);

    IDirectory::Entry GetEntry(BASE_NS::string_view uri) override;
    IFile::Ptr OpenFile(BASE_NS::string_view path) override;
    IFile::Ptr CreateFile(BASE_NS::string_view path) override;
    bool DeleteFile(BASE_NS::string_view path) override;

    IDirectory::Ptr OpenDirectory(BASE_NS::string_view path) override;
    IDirectory::Ptr CreateDirectory(BASE_NS::string_view path) override;
    bool DeleteDirectory(BASE_NS::string_view path) override;

    bool Rename(BASE_NS::string_view fromPath, BASE_NS::string_view toPath) override;

    BASE_NS::vector<BASE_NS::string> GetUriPaths(BASE_NS::string_view uri) const override;

    void AppendSearchPath(BASE_NS::string_view uri);
    void PrependSearchPath(BASE_NS::string_view uri);

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    FileManager& fileManager_;
    BASE_NS::vector<BASE_NS::string> destinations_;
};
CORE_END_NAMESPACE()

#endif // CORE__IO__PROXY_FILESYSTEM_H
