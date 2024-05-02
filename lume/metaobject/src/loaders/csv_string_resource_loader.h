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

#ifndef META_SRC_LOADERS_CSV_STRING_RESOURCE_LOADER_H
#define META_SRC_LOADERS_CSV_STRING_RESOURCE_LOADER_H

#include <base/containers/unordered_map.h>

#include <meta/api/property/property_event_handler.h>
#include <meta/ext/event_impl.h>
#include <meta/ext/object.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/loaders/intf_file_content_loader.h>

META_BEGIN_NAMESPACE()

class CsvStringResourceLoader final : public META_NS::ObjectFwd<CsvStringResourceLoader,
                                          ClassId::CsvStringResourceLoader, ClassId::Object, IFileContentLoader> {
    META_IMPLEMENT_INTERFACE_PROPERTY(IFileContentLoader, bool, Cached)
public:
    bool Build(const IMetadata::Ptr& data) override;

public: // IDynamicContentLoader
    META_IMPLEMENT_INTERFACE_EVENT(IDynamicContentLoader, IOnChanged, ContentChanged)
    void Reload() override;

public: // IFileContentLoader
    bool SetFile(CORE_NS::IFile::Ptr file) override;
    IObject::Ptr Create(const IObject::Ptr& params) override;

private:
    static constexpr const BASE_NS::string_view KEY_HEADER_COLUMN = "key";
    static constexpr const BASE_NS::string_view TARGET_PROPERTY_NAME = "strings";

    struct ResourceObjectOptions {
        BASE_NS::string keyHeaderColumnName { KEY_HEADER_COLUMN };
        BASE_NS::string targetPropertyName { CsvStringResourceLoader::TARGET_PROPERTY_NAME };
        bool keysToLower { true };
    };

    using CsvContentItemType = BASE_NS::pair<BASE_NS::string, BASE_NS::vector<BASE_NS::string>>;
    using CsvContentType = BASE_NS::vector<CsvContentItemType>;

    void Reset();
    IObject::Ptr CreateStringResourceObject(const CsvContentType& content, const ResourceObjectOptions& options);
    CsvContentType ParseCsv(BASE_NS::string_view csv, const ResourceObjectOptions& options);

    CORE_NS::IFile::Ptr file_;
    CsvContentType cachedContent_;
};

META_END_NAMESPACE()

#endif
