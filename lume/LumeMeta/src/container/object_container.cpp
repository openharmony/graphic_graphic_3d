/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Object container implementation
 * Author: Lauri Jaaskela
 * Create: 2021-09-20
 */

#include "object_container.h"

#include <meta/base/interface_utils.h>
#include <meta/interface/intf_containable.h>
#include <meta/interface/intf_content.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_INTERNAL_NAMESPACE()

bool ObjectContainer::Build(const IMetadata::Ptr& data)
{
    bool ret = Super::Build(data);
    if (ret) {
        parent_ = GetSelf<IContainer>();
        SetImplementingIContainer(GetSelf<IObject>().get(), this);
    }
    return ret;
}

void ObjectContainer::Destroy()
{
    InternalRemoveAll();
    Super::Destroy();
}

IEvent::Ptr ObjectContainer::GetAboutToChange()
{
    auto ev = GetEvent("ContainerAboutToChange", MetadataQuery::EXISTING);
    return ev && ev->HasHandlers() ? ev : nullptr;
}
bool ObjectContainer::CallAboutToChange(ContainerChangeType type, const IObject::Ptr& object)
{
    bool success = true;
    if (auto ev = GetAboutToChange()) {
        Invoke<IOnChildChanging>(ev, ChildChangedInfo { type, object, parent_ }, success);
    }
    return success;
}

bool ObjectContainer::Add(const IObject::Ptr& object)
{
    return CallAboutToChange(ContainerChangeType::ADDING, object) && Container::Add(object);
}
bool ObjectContainer::Insert(SizeType index, const IObject::Ptr& object)
{
    return CallAboutToChange(ContainerChangeType::ADDING, object) && Container::Insert(index, object);
}
bool ObjectContainer::Replace(const IObject::Ptr& child, const IObject::Ptr& replaceWith, bool addAlways)
{
    return CallAboutToChange(ContainerChangeType::REMOVING, child) &&
           CallAboutToChange(ContainerChangeType::ADDING, replaceWith) &&
           Container::Replace(child, replaceWith, addAlways);
}
bool ObjectContainer::Remove(SizeType index)
{
    auto object = GetAt(index);
    return object && Remove(object);
}
bool ObjectContainer::Remove(const IObject::Ptr& object)
{
    return CallAboutToChange(ContainerChangeType::REMOVING, object) && Container::Remove(object);
}
void ObjectContainer::RemoveAll()
{
    bool success = true;
    auto list = GetAll();
    for (auto it = list.begin(); it != list.end() && success; ++it) {
        success = success && CallAboutToChange(ContainerChangeType::REMOVING, *it);
    }
    if (success) {
        Container::RemoveAll();
    }
}

META_END_INTERNAL_NAMESPACE()
