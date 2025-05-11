#include "class_content_loader.h"

#include <meta/interface/intf_object_registry.h>

META_BEGIN_NAMESPACE()

bool ClassContentLoader::Build(const IMetadata::Ptr& data)
{
    classIdChangedHandler_.Subscribe(
        META_ACCESS_PROPERTY(ClassId), MakeCallback<IOnChanged>(this, &ClassContentLoader::Reload));
    return true;
}

void ClassContentLoader::Reload()
{
    Invoke<IOnChanged>(META_ACCESS_EVENT(ContentChanged));
}

IObject::Ptr ClassContentLoader::Create(const IObject::Ptr&)
{
    const auto& id = META_ACCESS_PROPERTY(ClassId)->GetValue();

    if (id != BASE_NS::Uid {}) {
        return META_NS::GetObjectRegistry().Create(id);
    }

    return {};
}

META_END_NAMESPACE()
