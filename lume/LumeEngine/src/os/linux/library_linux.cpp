/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "library_linux.h"


#include <dlfcn.h>

#include <base/containers/string.h>
#include <core/log.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::string;
using BASE_NS::string_view;

LibraryLinux::LibraryLinux(const string_view filename)
{
    string tmp(filename);
    libraryHandle_ = dlopen(tmp.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!libraryHandle_) {
        const char* errorMessage = dlerror();
        if (errorMessage != NULL) {
            CORE_LOG_E("Loading dynamic library '%s' failed: %s", filename.data(), errorMessage);
        }
    }
}

LibraryLinux::~LibraryLinux()
{
    if (libraryHandle_) {
        dlclose(libraryHandle_);
        libraryHandle_ = nullptr;
    }
}

IPlugin* LibraryLinux::GetPlugin() const
{
    if (!libraryHandle_) {
        return nullptr;
    }

    return reinterpret_cast<IPlugin*>(dlsym(libraryHandle_, "gPluginData"));
}

void LibraryLinux::Destroy()
{
    delete this;
}

ILibrary::Ptr ILibrary::Load(const string_view filePath)
{
    return ILibrary::Ptr { new LibraryLinux(filePath) };
}

string_view ILibrary::GetFileExtension()
{
    return ".so";
}
CORE_END_NAMESPACE()

