/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE__IO__STD_DIRECTORY_H
#define CORE__IO__STD_DIRECTORY_H

#include <base/containers/unique_ptr.h>
#include <core/io/intf_directory.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
struct DirImpl;

class StdDirectory final : public IDirectory {
public:
    StdDirectory();
    ~StdDirectory() override;

    bool Open(BASE_NS::string_view path);
    void Close() override;

    BASE_NS::vector<Entry> GetEntries() const override;

    // NOTE: Helper function for resolving paths does not really belong here but it's here
    // so that msdirent.h is not needed in multiple compilation units.
    static BASE_NS::string ResolveAbsolutePath(BASE_NS::string_view path, bool isDirectory);
    static void FormatPath(BASE_NS::string& path, bool isDirectory);
    static BASE_NS::string GetDirName(BASE_NS::string_view path);
    static BASE_NS::string GetBaseName(BASE_NS::string_view path);

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    BASE_NS::unique_ptr<DirImpl> dir_;
};
CORE_END_NAMESPACE()

#endif // CORE__IO__STD_DIRECTORY_H
