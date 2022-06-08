/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE__IO__PROXY_DIRECTORY_H
#define CORE__IO__PROXY_DIRECTORY_H

#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <core/io/intf_directory.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class ProxyFilesystem;

class ProxyDirectory final : public IDirectory {
public:
    explicit ProxyDirectory(BASE_NS::vector<IDirectory::Ptr>&& directories);
    ~ProxyDirectory() override = default;

    void Close() override;

    BASE_NS::vector<Entry> GetEntries() const override;

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    BASE_NS::vector<IDirectory::Ptr> directories_;
};
CORE_END_NAMESPACE()

#endif // CORE__IO__PROXY_DIRECTORY_H
