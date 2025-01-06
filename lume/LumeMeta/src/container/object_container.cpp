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

#include "object_container.h"

#include <algorithm>

#include <meta/base/interface_utils.h>
#include <meta/interface/intf_containable.h>
#include <meta/interface/intf_content.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

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

META_END_NAMESPACE()
