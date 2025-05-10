
#include "instantiating_object_provider.h"

META_BEGIN_NAMESPACE()

bool InstantiatingObjectProvider::SetObjectClassId(const ObjectId& id)
{
    id_ = id;
    return id_.IsValid();
}

IObject::Ptr InstantiatingObjectProvider::Construct(const IMetadata::Ptr& data)
{
    return GetObjectRegistry().Create(id_, IObjectRegistry::CreateInfo {}, data);
}

META_END_NAMESPACE()
