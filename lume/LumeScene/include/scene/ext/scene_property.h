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

#ifndef SCENE_EXT_SCENE_PROPERTY_H
#define SCENE_EXT_SCENE_PROPERTY_H

#include <scene/base/namespace.h>
#include <scene/ext/intf_engine_property_init.h>

#include <meta/ext/implementation_macros.h>

SCENE_BEGIN_NAMESPACE()

template<typename Type, bool IsDynamic>
META_NS::Internal::MetadataCtor* CreateScenePropertyCtor()
{
    return [](const BASE_NS::shared_ptr<META_NS::IOwner>& owner, const META_NS::StaticMetadata& d) {
        auto engOwner = interface_cast<IEnginePropertyInit>(owner);
        if (!engOwner) {
            CORE_ASSERT("Invalid use of scene property without IEnginePropertyInit");
            return META_NS::SharedPtrIInterface {};
        }
        if (auto prop = META_NS::CreatePropertyImpl<Type>(
                d.name, d.runtimeValue, META_NS::ObjectFlagBitsValue(META_NS::ObjectFlagBits::NATIVE))) {
            const char* component = static_cast<const char*>(d.data);
            if constexpr (IsDynamic) {
                if (engOwner->InitDynamicProperty(prop, component)) {
                    return interface_pointer_cast<CORE_NS::IInterface>(prop);
                }
            } else {
                if (engOwner->AttachEngineProperty(prop, component)) {
                    return interface_pointer_cast<CORE_NS::IInterface>(prop);
                }
            }
            CORE_LOG_E("Failed to attach to engine property %s [%s]", d.name, component);
        }
        return META_NS::SharedPtrIInterface {};
    };
}

//--- PROPERTY DATA
#define SCENE_IMPL_PROPERTY_DATA(intf, type, name, ppath, dynamic)                                                \
    { META_NS::MetadataType::PROPERTY, intf::INTERFACE_INFO, #name,                                               \
        SCENE_NS::CreateScenePropertyCtor<type, dynamic>(), [] { return META_NS::ConstructAnyHelper<type>({}); }, \
        ppath },

#define SCENE_STATIC_PROPERTY_DATA(intf, type, name, ppath) SCENE_IMPL_PROPERTY_DATA(intf, type, name, ppath, false)
#define SCENE_STATIC_DYNINIT_PROPERTY_DATA(intf, type, name, ppath) \
    SCENE_IMPL_PROPERTY_DATA(intf, type, name, ppath, true)
//---

//--- ARRAY PROPERTY DATA
#define SCENE_IMPL_ARRAY_PROPERTY_DATA(intf, type, name, ppath, dynamic) \
    { META_NS::MetadataType::PROPERTY, intf::INTERFACE_INFO, #name,      \
        SCENE_NS::CreateScenePropertyCtor<type[], dynamic>(),            \
        [] { return META_NS::IAny::Ptr(META_NS::ConstructArrayAnyHelper<type>({})); }, ppath },

#define SCENE_STATIC_ARRAY_PROPERTY_DATA(intf, type, name, ppath) \
    SCENE_IMPL_ARRAY_PROPERTY_DATA(intf, type, name, ppath, false)
#define SCENE_STATIC_DYNINIT_ARRAY_PROPERTY_DATA(intf, type, name, ppath) \
    SCENE_IMPL_ARRAY_PROPERTY_DATA(intf, type, name, ppath, true)
//---

SCENE_END_NAMESPACE()

#endif