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

#ifndef META_INTERFACE_STATIC_OBJECT_METADATA_H
#define META_INTERFACE_STATIC_OBJECT_METADATA_H

#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/interface/intf_call_context.h>
#include <meta/interface/intf_event.h>
#include <meta/interface/intf_function.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

namespace Internal {
using PCtor = IProperty::Ptr();
using PMemberInit = bool(void*, const IProperty::Ptr&);
} // namespace Internal
/// Static metadata for property
struct PropertyMetadata {
    /// Name of the property
    BASE_NS::string_view name;
    /// Info of the interface where this property comes from
    InterfaceInfo interfaceInfo;
    /// Flags of the property
    ObjectFlagBitsValue flags;
    /// Uid of the property type
    BASE_NS::Uid typeId;
    /// Function to create property of this type
    Internal::PCtor* create {};
    /// Function to initialise a property for specific object instance.
    Internal::PMemberInit* init {};
};

namespace Internal {
using ECtor = IEvent::Ptr();
using EMemberInit = bool(void*, const IEvent::Ptr&);
} // namespace Internal
/// Static metadata for event
struct EventMetadata {
    // Name of the event
    BASE_NS::string_view name;
    /// Info of the interface where this property comes from
    InterfaceInfo interfaceInfo;
    /// Uid of the event type
    BASE_NS::Uid typeId;
    /// Function to create event of this type
    Internal::ECtor* create {};
    /// Function to initialise a event for specific object instance
    Internal::EMemberInit* init {};
};

namespace Internal {
using FCtor = IFunction::Ptr(void*);
using FContext = ICallContext::Ptr();
} // namespace Internal

/// Static metadata for function
struct FunctionMetadata {
    /// Name of the function
    BASE_NS::string_view name;
    /// Info of the interface where this property comes from
    InterfaceInfo interfaceInfo;
    /// Function to create function of this type
    Internal::FCtor* create {};
    Internal::FContext* context {};
};

/// Static metadata for single object class
struct StaticObjectMetadata {
    const META_NS::ClassInfo& classInfo;
    const StaticObjectMetadata* baseclass {};
    BASE_NS::vector<PropertyMetadata> properties;
    BASE_NS::vector<EventMetadata> events;
    BASE_NS::vector<FunctionMetadata> functions;
};

META_END_NAMESPACE()

#endif
