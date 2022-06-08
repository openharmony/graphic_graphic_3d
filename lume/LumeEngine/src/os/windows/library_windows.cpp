/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */


#include "library_windows.h"

#include <windows.h>

#include <base/containers/string.h>
#include <core/log.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::string;
using BASE_NS::string_view;

LibraryWindows::LibraryWindows(const string_view filename)
{
    string tmp(filename);
    // fix the slashes. (without the correct backslashes the LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR wont work)
    for (auto& t : tmp) {
        if (t == '/') {
            t = '\\';
        }
    }
    libraryHandle_ =
        LoadLibraryEx(tmp.c_str(), NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!libraryHandle_) {
        DWORD errorCode = GetLastError();
        CORE_LOG_E("Loading dynamic library '%s' failed: 0x%lx", tmp.c_str(), errorCode);
    }
}

LibraryWindows::~LibraryWindows()
{
    if (libraryHandle_) {
        FreeLibrary(libraryHandle_);
        libraryHandle_ = nullptr;
    }
}

IPlugin* LibraryWindows::GetPlugin() const
{
    if (!libraryHandle_) {
        return nullptr;
    }

    return reinterpret_cast<IPlugin*>(GetProcAddress(libraryHandle_, "gPluginData"));
}

void LibraryWindows::Destroy()
{
    delete this;
}

ILibrary::Ptr ILibrary::Load(const string_view filePath)
{
    return ILibrary::Ptr { new LibraryWindows(filePath) };
}

string_view ILibrary::GetFileExtension()
{
    return ".dll";
}
CORE_END_NAMESPACE()

