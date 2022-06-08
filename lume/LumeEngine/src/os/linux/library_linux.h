/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_OS_LINUX_LIBRARY_LINUX_H
#define CORE_OS_LINUX_LIBRARY_LINUX_H


#include <core/namespace.h>

#include "os/intf_library.h"

CORE_BEGIN_NAMESPACE()
class LibraryLinux final : public ILibrary {
public:
    explicit LibraryLinux(const BASE_NS::string_view filename);
    ~LibraryLinux() override;

    IPlugin* GetPlugin() const override;

protected:
    void Destroy() override;

private:
    void* libraryHandle_ { nullptr };
};
CORE_END_NAMESPACE()

#endif // CORE_OS_LINUX_LIBRARY_LINUX_H
