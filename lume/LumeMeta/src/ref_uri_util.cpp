/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: Serialization reference uri util
 * Author: Mikael Kilpeläinen
 * Create: 2022-11-22
 */

#include "ref_uri_util.h"

#include <meta/api/util.h>
#include <meta/base/ref_uri.h>
#include <meta/ext/resolve_helper.h>
#include <meta/interface/intf_containable.h>
#include <meta/interface/intf_container.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

static IObjectInstance::ConstPtr FindParentObject(const IObjectInstance::ConstPtr& obj)
{
    auto containee = interface_cast<IContainable>(obj);
    return containee ? interface_pointer_cast<IObjectInstance>(containee->GetParent()) : nullptr;
}

static IObjectInstance::ConstPtr FindRootObject(IObjectInstance::ConstPtr obj)
{
    auto prev = obj;
    while ((obj = FindParentObject(obj))) {
        prev = obj;
    }
    return prev;
}

static IObjectInstance::ConstPtr FindChildObject(const IObjectInstance::ConstPtr& obj, BASE_NS::string_view name)
{
    auto cont = interface_cast<IContainer>(obj);
    return cont ? interface_pointer_cast<IObjectInstance>(cont->FindByName(name)) : nullptr;
}

static IObject::Ptr ResolvePropertySegment(
    BASE_NS::string_view propName, const IObjectInstance::Ptr& base, const RefUri& ref)
{
    // get the property and see if it contains object to continue the resolve
    IMetadata* meta = interface_cast<IMetadata>(base);
    if (!meta) {
        return nullptr;
    }

    auto prop = meta->GetProperty(propName);
    if (prop) {
        if (ref.GetAbsoluteInterpretation() && ref.IsEmpty()) {
            return interface_pointer_cast<IObject>(prop);
        }
        if (auto obj = GetPointer<IObjectInstance>(prop)) {
            return obj->Resolve(ref);
        }
    }
    return nullptr;
}

static IObject::Ptr ResolveSegment(const IObjectInstance::Ptr& base, RefUri ref)
{
    auto node = ref.TakeFirstNode();
    if (node.type == RefUri::Node::OBJECT) {
        IObjectInstance::ConstPtr obj = (node.name == "..") ? FindParentObject(base) : FindChildObject(base, node.name);
        return obj ? obj->Resolve(ref) : nullptr;
    }

    if (node.type == RefUri::Node::SPECIAL) {
        if (node.name != "@Context") {
            return nullptr;
        }
        // ask the object to resolve the special segment
        auto obj = base->Resolve<IObjectInstance>(RefUri::ContextUri());
        return obj ? obj->Resolve(ref) : nullptr;
    }

    return ResolvePropertySegment(node.name, base, ref);
}

IObject::Ptr DefaultResolveObject(const IObjectInstance::Ptr& base, const RefUri& uri)
{
    if (!CheckValidResolve(base, uri)) {
        return nullptr;
    }

    RefUri ref { uri.RelativeUri() };
    if (ref.IsEmpty()) {
        return base;
    }

    if (ref.StartsFromRoot()) {
        ref.SetStartsFromRoot(false);
        auto obj = FindRootObject(base);
        return obj ? obj->Resolve(BASE_NS::move(ref)) : nullptr;
    }

    return ResolveSegment(base, BASE_NS::move(ref));
}

META_END_NAMESPACE()
