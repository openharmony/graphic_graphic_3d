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
#include "base_object.h"

#include <base/util/uid_util.h>
#include <core/plugin/intf_class_factory.h>

#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_proxy_object.h>
#include <meta/interface/property/intf_property_internal.h>

#include "ref_uri_util.h"

META_BEGIN_NAMESPACE()
namespace Internal {

// IObject
InstanceId BaseObject::GetInstanceId() const
{
    return instanceId_;
}

BASE_NS::string BaseObject::GetName() const
{
    return GetInstanceId().ToString();
}

IObject::Ptr BaseObject::Resolve(const RefUri& uri) const
{
    return DefaultResolveObject(me_.lock(), uri);
}

IObject::Ptr BaseObject::GetSelf() const
{
    return me_.lock();
}

BASE_NS::vector<BASE_NS::Uid> BaseObject::GetInterfaces() const
{
    return IntroduceInterfaces::GetInterfacesVector();
}

ObjectFlagBitsValue BaseObject::GetObjectFlags() const
{
    return flags_;
}

void BaseObject::SetObjectFlags(const ObjectFlagBitsValue& value)
{
    flags_ = value;
}

ObjectFlagBitsValue BaseObject::GetObjectDefaultFlags() const
{
    return ObjectFlagBits::DEFAULT_FLAGS;
}

// IDerived
void BaseObject::SetSuperInstance(const IObject::Ptr& aggr, const IObject::Ptr& super)
{
    me_ = interface_pointer_cast<IObjectInstance>(aggr);
}
BASE_NS::Uid BaseObject::GetSuperClassUid() const
{
    // empty uid is "invalid", which means do not create a sub object ...
    return {};
}

// ILifecycle
bool BaseObject::Build(const IMetadata::Ptr&)
{
    // Set object flags to class default.
    SetObjectFlags(GetObjectDefaultFlags());
    return true;
}
void BaseObject::SetInstanceId(InstanceId uid)
{
    instanceId_ = uid;
}
void BaseObject::Destroy()
{
    GetObjectRegistry().DisposeObject(instanceId_);
}

IObjectRegistry& BaseObject::GetObjectRegistry() const
{
    return META_NS::GetObjectRegistry();
}

} // namespace Internal

META_END_NAMESPACE()
