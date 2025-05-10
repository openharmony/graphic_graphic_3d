
#include "content_loader_object_provider.h"

META_BEGIN_NAMESPACE()

bool ContentLoaderObjectProvider::SetContentLoader(const IContentLoader::Ptr& loader)
{
    loader_ = loader;
    return true;
}

IObject::Ptr ContentLoaderObjectProvider::Construct(const IMetadata::Ptr& data)
{
    return loader_ ? loader_->Create(nullptr) : nullptr;
}

META_END_NAMESPACE()
