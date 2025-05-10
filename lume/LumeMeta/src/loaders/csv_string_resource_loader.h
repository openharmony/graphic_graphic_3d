/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: CsvStringResourceLoader class implementation. Loads translation strings from
 *              a CSV file and places them as properties in an IObject, matching the expected
 *              translation resource structure.
 * Author: Lauri Jaaskela
 * Create: 2022-09-05
 */

#ifndef META_SRC_LOADERS_CSV_STRING_RESOURCE_LOADER_H
#define META_SRC_LOADERS_CSV_STRING_RESOURCE_LOADER_H

#include <base/containers/unordered_map.h>

#include <meta/api/property/property_event_handler.h>
#include <meta/base/namespace.h>
#include <meta/ext/event_impl.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/object_fwd.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/loaders/intf_file_content_loader.h>

META_BEGIN_NAMESPACE()

class CsvStringResourceLoader final : public IntroduceInterfaces<ObjectFwd, IFileContentLoader> {
    META_OBJECT(CsvStringResourceLoader, ClassId::CsvStringResourceLoader, IntroduceInterfaces)
public:
    bool Build(const IMetadata::Ptr& data) override;

    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(IFileContentLoader, bool, Cached)
    META_STATIC_EVENT_DATA(IDynamicContentLoader, IOnChanged, ContentChanged)
    META_END_STATIC_DATA()
    META_IMPLEMENT_PROPERTY(bool, Cached)
    META_IMPLEMENT_EVENT(IOnChanged, ContentChanged)
public: // IDynamicContentLoader
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
