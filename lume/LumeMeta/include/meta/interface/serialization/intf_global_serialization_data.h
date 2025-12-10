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

#ifndef META_INTERFACE_SERIALIZATION_IGLOBAL_SERIALISATION_DATA_H
#define META_INTERFACE_SERIALIZATION_IGLOBAL_SERIALISATION_DATA_H

#include <base/containers/unique_ptr.h>
#include <core/plugin/intf_interface.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/interface/intf_any.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/serialization/intf_value_serializer.h>

META_BEGIN_NAMESPACE()

struct SerializationSettings {};

/// Interface to access global serialization data
class IGlobalSerializationData : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IGlobalSerializationData, "f738d4d6-2575-4ae8-bb97-70a23bb6565a")
public:
    /// Get global serialisation settings
    virtual SerializationSettings GetDefaultSettings() const = 0;
    /// Set global serialisation setting
    virtual void SetDefaultSettings(const SerializationSettings& settings) = 0;
    /// Register global object which is then serialised as reference
    virtual void RegisterGlobalObject(const IObject::Ptr& object) = 0;
    /// Unregister global object
    virtual void UnregisterGlobalObject(const IObject::Ptr& object) = 0;
    /// Get global object of given instance id
    virtual IObject::Ptr GetGlobalObject(const InstanceId& id) const = 0;
    /// Register value serializer for exporting and importing
    virtual void RegisterValueSerializer(const IValueSerializer::Ptr&) = 0;
    /// Unregister value serializer
    virtual void UnregisterValueSerializer(const TypeId& id) = 0;
    /// Get value serializer for given type id (that is, the type it serialises)
    virtual IValueSerializer::Ptr GetValueSerializer(const TypeId& id) const = 0;
};

META_INTERFACE_TYPE(META_NS::IGlobalSerializationData)

META_END_NAMESPACE()

#endif
