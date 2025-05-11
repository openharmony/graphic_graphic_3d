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
    Invoke<IOnChanged>(META_ACCESS_EVENT(ContentChanged));
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
