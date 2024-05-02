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
#include "json_content_loader.h"

#include "../serialization/importer.h"
#include "../serialization/json_importer.h"

META_BEGIN_NAMESPACE()

bool JsonContentLoader::Build(const IMetadata::Ptr& data)
{
    cachedChangedHandler_.Subscribe(
        META_ACCESS_PROPERTY(Cached), MakeCallback<IOnChanged>(this, &JsonContentLoader::Reload));
    return true;
}

void JsonContentLoader::Reload()
{
    tree_.reset();
    META_ACCESS_EVENT(ContentChanged)->Invoke();
}

IObject::Ptr JsonContentLoader::Create(const IObject::Ptr&)
{
    if (META_ACCESS_PROPERTY(Cached)->GetValue()) {
        // Try to use a cached file
        return LoadCached();
    }
    if (file_) {
        file_->Seek(0);
        Serialization::JsonImporter importer;
        if (auto result = importer.Import(*file_)) {
            return result;
        }
        CORE_LOG_E("Importing object hierarchy failed.");
    }
    return nullptr;
}

IObject::Ptr JsonContentLoader::LoadCached()
{
    if (!UpdateTree()) {
        CORE_LOG_E("Instantiating object failed");
        // No object
        return {};
    }

    Serialization::Importer imp;
    return imp.Import(tree_);
}

bool JsonContentLoader::UpdateTree()
{
    if (tree_) {
        return true;
    }
    if (!file_) {
        return false;
    }

    file_->Seek(0);

    Serialization::JsonImporter importer;
    tree_ = importer.ImportAsTree(*file_);

    return tree_ != nullptr;
}

bool JsonContentLoader::SetFile(CORE_NS::IFile::Ptr file)
{
    file_ = BASE_NS::move(file);
    Reload();
    return true;
}

META_END_NAMESPACE()
