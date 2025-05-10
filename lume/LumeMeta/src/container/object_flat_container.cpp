
#include "object_flat_container.h"

#include <algorithm>

#include <meta/base/interface_utils.h>
#include <meta/interface/intf_containable.h>

META_BEGIN_NAMESPACE()

bool ObjectFlatContainer::Build(const IMetadata::Ptr& data)
{
    bool ret = Super::Build(data);
    if (ret) {
        auto self = GetSelf<IContainer>();
        parent_ = GetSelf<IContainer>();
        SetImplementingIContainer(GetSelf<IObject>().get(), this);
    }
    return ret;
}

void ObjectFlatContainer::Destroy()
{
    InternalRemoveAll();
    Super::Destroy();
}

META_END_NAMESPACE()
