/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "library_ohos.h"

#include <dlfcn.h>

#include <base/containers/string.h>
#include <core/log.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::string;
using BASE_NS::string_view;

LibraryOHOS::LibraryOHOS(const string_view filename)
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

LibraryOHOS::~LibraryOHOS()
{
    if (libraryHandle_) {
        dlclose(libraryHandle_);
        libraryHandle_ = nullptr;
    }
}

IPlugin* LibraryOHOS::GetPlugin() const
{
    if (!libraryHandle_) {
        return nullptr;
    }

    return reinterpret_cast<IPlugin*>(dlsym(libraryHandle_, "gPluginData"));
}

void LibraryOHOS::Destroy()
{
    delete this;
}

ILibrary::Ptr ILibrary::Load(const string_view filePath)
{
    #define TO_STRING(name) #name
    #define LIB_NAME(name) TO_STRING(name)
    CORE_LOG_E("check load filePath %s %s, %s %s E", filePath.data(), LIB_NAME(LIB_ENGINE_CORE),
        LIB_NAME(LIB_RENDER), LIB_NAME(LIB_CORE3D));

    if (filePath.find(LIB_NAME(LIB_ENGINE_CORE)) == string_view::npos &&
        filePath.find(LIB_NAME(LIB_RENDER)) == string_view::npos &&
        filePath.find(LIB_NAME(LIB_CORE3D)) == string_view::npos) {
        CORE_LOG_E("failed to find the lib %s", filePath.data());
        return ILibrary::Ptr {};
    }

    CORE_LOG_E("do load filePath %s X", filePath.data());
    return ILibrary::Ptr { new LibraryOHOS(filePath) };
}

string_view ILibrary::GetFileExtension()
{
    return ".so";
}
CORE_END_NAMESPACE()

