/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

    if (filePath.find(LIB_NAME(LIB_ENGINE_CORE)) == string_view::npos &&
        filePath.find(LIB_NAME(LIB_RENDER)) == string_view::npos &&
        filePath.find(LIB_NAME(LIB_CORE3D)) == string_view::npos &&
        filePath.find("libMotPhysPlugin.z.so") == string_view::npos &&
        filePath.find("libPluginMetaObject") == string_view::npos &&
        filePath.find("libPluginSceneWidget") == string_view::npos) {
        return ILibrary::Ptr {};
    }

    return ILibrary::Ptr { new LibraryOHOS(filePath) };
}

string_view ILibrary::GetFileExtension()
{
    return ".so";
}
CORE_END_NAMESPACE()
