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
#include "csv_string_resource_loader.h"

#include <cctype>

#include <meta/api/object.h>

#include "csv_parser.h"

META_BEGIN_NAMESPACE()

bool CsvStringResourceLoader::Build(const IMetadata::Ptr& data)
{
    return true;
}

void CsvStringResourceLoader::Reload()
{
    Reset();
    META_ACCESS_EVENT(ContentChanged)->Invoke();
}

void CsvStringResourceLoader::Reset()
{
    cachedContent_.clear();
}

IObject::Ptr CsvStringResourceLoader::Create(const IObject::Ptr& params)
{
    CsvContentType csv;
    ResourceObjectOptions options;

    // Check the loader parameters object for parameters which can be used to affect functionality
    if (auto meta = interface_cast<IMetadata>(params)) {
        if (const auto prop = meta->GetPropertyByName<BASE_NS::string>("KeyHeaderColumnName")) {
            options.keyHeaderColumnName = prop->GetValue();
        }
        if (const auto prop = meta->GetPropertyByName<BASE_NS::string>("TargetPropertyName")) {
            options.targetPropertyName = prop->GetValue();
        }
        if (const auto prop = meta->GetPropertyByName<bool>("KeysToLower")) {
            options.keysToLower = prop->GetValue();
        }
    }

    if (cachedContent_.empty() || !META_ACCESS_PROPERTY(Cached)->GetValue()) {
        if (file_) {
            auto size = file_->GetLength();
            file_->Seek(0);
            BASE_NS::string content;
            content.resize(size);
            if (file_->Read(content.data(), size) < size) {
                CORE_LOG_E("Failed to read");
                Reset();
            } else {
                csv = ParseCsv(content, options);
            }
        } else {
            Reset();
        }
    } else {
        csv.swap(cachedContent_);
    }

    IObject::Ptr result = CreateStringResourceObject(csv, options);

    if (META_ACCESS_PROPERTY(Cached)->GetValue()) {
        cachedContent_.swap(csv);
    }

    return result;
}

IObject::Ptr CsvStringResourceLoader::CreateStringResourceObject(
    const CsvContentType& content, const ResourceObjectOptions& options)
{
    if (content.empty()) {
        return {};
    }

    Object strings;
    const BASE_NS::vector<BASE_NS::string>* keys = nullptr;

    // Find the column which contains our keys
    for (const auto& item : content) {
        if (item.first == options.keyHeaderColumnName) {
            keys = &item.second;
            break;
        }
    }

    if (!keys) {
        CORE_LOG_E("Cannot find column '%s' from CSV header", options.keyHeaderColumnName.c_str());
        return {};
    }
    auto& objectRegistry = META_NS::GetObjectRegistry();
    // Create IObject property for each column, containing the items as properties of type BASE_NS::string
    for (const auto& column : content) {
        if (&column.second == keys) {
            continue; // ignore the column whose header is <KeyHeaderColumn>
        }
        Object item;
        for (size_t i = 0; i < column.second.size(); i++) {
            item.Metadata().AddProperty(ConstructProperty<BASE_NS::string>(keys->at(i), column.second[i]));
        }
        strings.Metadata().AddProperty(ConstructProperty<IObject::Ptr>(column.first, item));
    }

    Object object;
    object.MetaProperty(ConstructProperty<IObject::Ptr>(options.targetPropertyName, strings));
    return object;
}

CsvStringResourceLoader::CsvContentType CsvStringResourceLoader::ParseCsv(
    BASE_NS::string_view csv, const ResourceObjectOptions& options)
{
    CsvParser parser(csv);
    CsvContentType csvContent;
    CsvParser::CsvRow header;

    if (parser.GetRow(header); header.empty()) {
        CORE_LOG_E("Failed to parse header");
        return {};
    }

    for (auto& item : header) {
        // Make sure that our column headers are in lower case
        csvContent.push_back(CsvContentItemType { options.keysToLower ? item.lower() : item, {} });
    }

    CsvParser::CsvRow items;
    unsigned long long lineNum = 0;
    while (parser.GetRow(items)) {
        lineNum++;
        if (items.size() != header.size()) {
            CORE_LOG_E("Number of items on line %llu does not match the number of CSV header items (%zu vs %zu)",
                lineNum, items.size(), header.size());
            return {};
        }
        for (size_t i = 0; i < csvContent.size(); i++) {
            csvContent[i].second.push_back(items[i]);
        }
    }

    return csvContent;
}

bool CsvStringResourceLoader::SetFile(CORE_NS::IFile::Ptr file)
{
    file_ = BASE_NS::move(file);
    Reload();
    return true;
}

META_END_NAMESPACE()
