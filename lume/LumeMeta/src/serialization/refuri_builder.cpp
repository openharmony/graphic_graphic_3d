/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "refuri_builder.h"

#include <meta/interface/intf_attach.h>
#include <meta/interface/intf_object_context.h>
#include <meta/interface/serialization/intf_exporter_state.h>

META_BEGIN_NAMESPACE()
namespace Serialization {

IObject::Ptr RefUriBuilder::ResolveProperty(
    const IProperty* property, RefUri& uri, const IObject::ConstPtr& previous) const
{
    uint32_t index = -1;
    if (ArrayPropertyLock lock { property }) {
        for (size_t i = 0; i != lock->GetSize(); ++i) {
            if (interface_pointer_cast<IObject>(GetConstPointer(lock->GetAnyAt(i))) == previous) {
                index = i;
                break;
            }
        }
    }
    uri.PushPropertySegment(property->GetName(), index);
    auto owner = interface_pointer_cast<IObject>(property->GetOwner());
    if (!owner) {
        CORE_LOG_E("No Owner for property '%s' when exporting weak ptr", property->GetName().c_str());
    }
    return owner;
}

IObject::Ptr RefUriBuilder::ResolveObjectInstance(
    const IObject::ConstPtr& ptr, const IObjectInstance& instance, RefUri& uri, const IObject::ConstPtr& previous) const
{
    if (!ptr) {
        return {};
    }
    const auto parent = ptr->Resolve(RefUri::ParentUri());
    if (auto i = interface_cast<META_NS::IProperty>(parent)) {
        return parent;
    }
    if (auto i = interface_cast<IAttach>(parent)) {
        if (auto cont = i->GetAttachmentContainer(false)) {
            if (ContainsObject(cont, ptr)) {
                uri.PushAttachmentSegment(instance.GetName());
                return parent;
            }
        }
    }
    if (auto context = interface_cast<IObjectContext>(ptr)) {
        uri.PushObjectContextSegment();
    } else {
        uri.PushObjectSegment(instance.GetName());
    }
    return parent;
}

IObject::Ptr RefUriBuilder::ResolveUriSegment(
    const IObject::ConstPtr& ptr, RefUri& uri, const IObject::ConstPtr& previous) const
{
    if (auto property = interface_cast<IProperty>(ptr)) {
        return ResolveProperty(property, uri, previous);
    }
    if (auto instance = interface_cast<IObjectInstance>(ptr)) {
        return ResolveObjectInstance(ptr, *instance, uri, previous);
    }
    return nullptr;
}

bool RefUriBuilder::IsAnchorType(const ObjectId& id) const
{
    for (auto&& v : anchorTypes_) {
        if (v == id) {
            return true;
        }
    }
    return false;
}

RefUri RefUriBuilder::BuildRefUri(IExporterState& state, const IObject::ConstPtr& object)
{
    auto userContext = state.GetUserContext();
    auto ops = state.GetOptions();
    IObject::ConstPtr p = object;
    auto original = p;
    IObject::ConstPtr previous;
    RefUri uri;
    auto handleInstance = [&](const IObjectInstance* obj) -> bool {
        if (!obj) {
            return false;
        }
        auto iid = state.ConvertInstanceId(obj->GetInstanceId());
        if (userContext && ops.useRelativeRefuriIfContextSet) {
            if (userContext.get() == obj) {
                uri.SetStartsFromRoot(true);
                return true;
            }
        }
        if (IsAnchorType(obj->GetClassId())) {
            uri.SetStartsFromRoot(true);
            return true;
        }
        if (state.HasBeenExported(iid) || state.IsGlobalObject(obj->GetInstanceId())) {
            uri.SetBaseObjectUid(iid.ToUid());
            return true;
        }
        return false;
    };

    while (p) {
        if (handleInstance(interface_cast<IObjectInstance>(p))) {
            return uri;
        }
        auto newp = ResolveUriSegment(p, uri, previous);
        previous = p;
        p = newp;
    }
    return {};
}

} // namespace Serialization
META_END_NAMESPACE()
