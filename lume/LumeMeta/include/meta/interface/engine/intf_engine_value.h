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

#ifndef META_ENGINE_INTERFACE_ENGINE_VALUE_H
#define META_ENGINE_INTERFACE_ENGINE_VALUE_H

#include <core/ecs/entity.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/intf_property_handle.h>

#include <meta/interface/intf_value.h>

META_BEGIN_NAMESPACE()

enum class EngineSyncDirection {
    AUTO,       /// Synchronise cached value to engine if changed, otherwise sync engine to cached value
    TO_ENGINE,  /// Synchronise cached value to engine
    FROM_ENGINE /// Synchronise from engine to cached value
};

META_REGISTER_INTERFACE(IEngineValue, "2edaea8a-936d-4bc5-a5eb-ecfdbb50574d")

class IEngineValue : public IValue {
    META_INTERFACE(IValue, IEngineValue)
public:
    virtual BASE_NS::string GetName() const = 0;
    virtual AnyReturnValue Sync(EngineSyncDirection) = 0;
};


struct EnginePropertyHandle {
    CORE_NS::IComponentManager* manager {};
    CORE_NS::Entity entity;
    // in case the property path contains IPropertyHandle*, we need to keep pointer to the most
    // recent value having such handle, so that we are able to update the handle properly
    IValue::Ptr parentValue;

    CORE_NS::IPropertyHandle* Handle() const
    {
        if (parentValue) {
            return GetValue<CORE_NS::IPropertyHandle*>(parentValue->GetValue());
        }
        return manager ? manager->GetData(entity) : nullptr;
    }

    explicit operator bool() const
    {
        return (manager && CORE_NS::EntityUtil::IsValid(entity)) || parentValue;
    }
};

struct EnginePropertyParams {
    EnginePropertyHandle handle;
    CORE_NS::Property property {};
    // offset to the parent property data, this is needed for member properties
    // calculate baseOffset + property.offset to access the data,
    // baseOffset is zero if not a member property
    uintptr_t baseOffset {};

    uintptr_t Offset() const
    {
        return baseOffset + property.offset;
    }

    // to support some old code, we allow to push the value directly to the engine property
    // if this is set, the values can only be set in the engine task queue thread
    bool pushValueToEngineDirectly {};
};

META_REGISTER_INTERFACE(IEngineInternalValueAccess, "f11f1272-e936-4804-9202-3b93606c25ea")
class IEngineInternalValueAccess : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEngineInternalValueAccess)
public:
    virtual IAny::Ptr CreateAny(const CORE_NS::Property&) const = 0;
    virtual bool IsCompatible(const CORE_NS::PropertyTypeDecl& type) const = 0;
    virtual AnyReturnValue SyncToEngine(const IAny& value, const EnginePropertyParams& params) const = 0;
    virtual AnyReturnValue SyncFromEngine(const EnginePropertyParams& params, IAny& out) const = 0;
};

class IEngineValueInternal : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEngineValueInternal, "86afd68c-7924-46b8-8bb8-aeace0029945")
public:
    virtual IAny::Ptr CreateAny() const = 0;
    virtual IEngineInternalValueAccess::ConstPtr GetInternalAccess() const = 0;
    virtual EnginePropertyParams GetPropertyParams() const = 0;
    virtual bool SetPropertyParams(const EnginePropertyParams& p) = 0;
    virtual bool ResetPendingNotify() = 0;
};

META_INTERFACE_TYPE(META_NS::IEngineValue)
META_END_NAMESPACE()

#endif
