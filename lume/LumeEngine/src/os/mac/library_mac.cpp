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

#include "library_mac.h"

#include <dlfcn.h>

#include <base/containers/string.h>
#include <core/log.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::string;
using BASE_NS::string_view;

LibraryMac::LibraryMac(const string_view filename)
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

LibraryMac::~LibraryMac()
{
    if (libraryHandle_) {
        dlclose(libraryHandle_);
        libraryHandle_ = nullptr;
    }
}

IPlugin* LibraryMac::GetPlugin() const
{
    if (!libraryHandle_) {
        return nullptr;
    }

    return reinterpret_cast<IPlugin*>(dlsym(libraryHandle_, "gPluginData"));
}

void LibraryMac::Destroy()
{
    delete this;
}

ILibrary::Ptr ILibrary::Load(const string_view filePath)
{
    return ILibrary::Ptr { new LibraryMac(filePath) };
}

string_view ILibrary::GetFileExtension()
{
    return ".dylib";
}
CORE_END_NAMESPACE()
