/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_OS_WINDOWS_LIBRARY_WINDOWS_H
#define CORE_OS_WINDOWS_LIBRARY_WINDOWS_H


#include <core/namespace.h>

#include "os/intf_library.h"

using HINSTANCE = struct HINSTANCE__*;
using HMODULE = HINSTANCE;

CORE_BEGIN_NAMESPACE()
class LibraryWindows final : public ILibrary {
public:
    explicit LibraryWindows(const BASE_NS::string_view filename);
    ~LibraryWindows() override;

    IPlugin* GetPlugin() const override;

protected:
    void Destroy() override;

private:
    HMODULE libraryHandle_ { nullptr };
};
CORE_END_NAMESPACE()


#endif // CORE_OS_WINDOWS_LIBRARY_WINDOWS_H
