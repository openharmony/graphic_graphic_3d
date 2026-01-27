/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include <base/containers/vector.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/plugin/intf_plugin.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::string;
using BASE_NS::string_view;

LibraryOHOS::LibraryOHOS(PluginData&& data) : data_(BASE_NS::move(data)) {}

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
        libraryHandle_ = dlopen(data_.filename.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!libraryHandle_) {
            const char* errorMessage = dlerror();
            if (errorMessage != NULL) {
                CORE_LOG_E("Loading dynamic library '%s' failed: %s", data_.filename.data(), errorMessage);
            }
            return nullptr;
        }
    }

    return reinterpret_cast<IPlugin*>(dlsym(libraryHandle_, "gPluginData"));
}

BASE_NS::Uid LibraryOHOS::GetPluginUid() const
{
    return data_.pluginUid;
}

BASE_NS::array_view<const BASE_NS::Uid> LibraryOHOS::GetPluginDependencies() const
{
    return data_.dependencies;
}

void LibraryOHOS::Destroy()
{
    delete this;
}

ILibrary::Ptr ILibrary::Load(const string_view filePath, const IFile::Ptr& filePtr)
{
    auto pluginData = ProcessElf(filePath, filePtr);
    if ((pluginData.pluginUid != BASE_NS::Uid {})) {
        return ILibrary::Ptr { new LibraryOHOS(BASE_NS::move(pluginData)) };
    }
    return {};
}

string_view ILibrary::GetFileExtension()
{
    return ".so";
}
CORE_END_NAMESPACE()