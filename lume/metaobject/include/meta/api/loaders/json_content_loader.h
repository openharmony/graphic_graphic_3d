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
#ifndef META_API_LOADERS_JSON_CONTENT_LOADER_H
#define META_API_LOADERS_JSON_CONTENT_LOADER_H

#include <meta/api/internal/object_api.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/loaders/intf_file_content_loader.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Wrapper for META_NS::ClassId::JsonContentLoader which provides functionality to load json files.
 */
class JsonContentLoader : public Internal::ObjectInterfaceAPI<JsonContentLoader, ClassId::JsonContentLoader> {
    META_API(JsonContentLoader)
    META_API_OBJECT_CONVERTIBLE(IFileContentLoader)
    META_API_OBJECT_CONVERTIBLE(IDynamicContentLoader)
    META_API_OBJECT_CONVERTIBLE(IContentLoader)

    META_API_CACHE_INTERFACE(IFileContentLoader, FileContentLoader)
    META_API_CACHE_INTERFACE(IDynamicContentLoader, DynamicContentLoader)

public:
    META_API_INTERFACE_PROPERTY_CACHED(FileContentLoader, Cached, bool)

    /** See IFileContentLoader::SetFile */
    JsonContentLoader& SetFile(CORE_NS::IFile::Ptr file)
    {
        META_API_CACHED_INTERFACE(FileContentLoader)->SetFile(BASE_NS::move(file));
        return *this;
    }

    /** See IDynamicContentLoader::SetFile */
    void Reload()
    {
        META_API_CACHED_INTERFACE(DynamicContentLoader)->Reload();
    }
};

META_END_NAMESPACE()

#endif
