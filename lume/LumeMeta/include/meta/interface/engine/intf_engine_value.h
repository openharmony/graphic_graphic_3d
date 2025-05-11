/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2024. All rights reserved.
 * Description: meta engine value
 * Author: Mikael Kilpeläinen
 * Create: 2024-02-19
 */

#ifndef META_ENGINE_INTERFACE_ENGINE_VALUE_H
#define META_ENGINE_INTERFACE_ENGINE_VALUE_H

#include <core/ecs/entity.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/intf_property_handle.h>

#include <meta/interface/intf_value.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Direction to which synchronise engine values
 */
enum class EngineSyncDirection {
    AUTO,       /// Synchronise cached value to engine if changed, otherwise sync engine to cached value
    TO_ENGINE,  /// Synchronise cached value to engine
    FROM_ENGINE /// Synchronise from engine to cached value
};

META_REGISTER_INTERFACE(IEngineValue, "2edaea8a-936d-4bc5-a5eb-ecfdbb50574d")

/**
 * @brief Value which can be synchronise with core engine property
 */
class IEngineValue : public IValue {
    META_INTERFACE(IValue, IEngineValue)
public:
    /// Name of this value
    virtual BASE_NS::string GetName() const = 0;
    /**
     * @brief Synchronise value with engine core property
     * @note  Careful where this is called (usually same thread as other core engine calls)
     * @return Result of the synchronisation
     */
    virtual AnyReturnValue Sync(EngineSyncDirection) = 0;
};

/**
 * @brief Internal handle for core engine property
 */
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

/**
 * @brief Internal property parameters
 */
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

    uint32_t index {};
    const CORE_NS::ContainerApi* containerMethods {};
    uintptr_t arraySubsOffset {};

    // to support some old code, we allow to push the value directly to the engine property
    // if this is set, the values can only be set in the engine task queue thread
    bool pushValueToEngineDirectly {};

    EnginePropertyParams() = default;

    EnginePropertyParams(const EnginePropertyHandle& h, const CORE_NS::Property& p, uintptr_t bOffset)
        : handle(h), property(p), baseOffset(bOffset)
    {}

    EnginePropertyParams(const EnginePropertyParams& base, const CORE_NS::Property& p) : EnginePropertyParams(base)
    {
        property = p;
        baseOffset = base.Offset();
    }

    EnginePropertyParams(const EnginePropertyParams& base, const CORE_NS::ContainerApi* capi, uint32_t i)
        : EnginePropertyParams(base)
    {
        arraySubsOffset = base.Offset();
        property = capi->property;
        index = i;
        containerMethods = capi;
        baseOffset = 0;
    }
};

META_REGISTER_INTERFACE(IEngineInternalValueAccess, "f11f1272-e936-4804-9202-3b93606c25ea")

/**
 * @brief Internal access implementation for specific core engine type
 */
class IEngineInternalValueAccess : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEngineInternalValueAccess)
public:
    /// Create any of mapped property type for the core engine type
    virtual IAny::Ptr CreateAny(const CORE_NS::Property&) const = 0;
    /// Create any of compatible type which can be serialised
    virtual IAny::Ptr CreateSerializableAny() const = 0;
    /// Check if given core type is compatible with the type this access implements
    virtual bool IsCompatible(const CORE_NS::PropertyTypeDecl& type) const = 0;
    /// Synchronise value to given engine property
    virtual AnyReturnValue SyncToEngine(const IAny& value, const EnginePropertyParams& params) const = 0;
    /// Synchronise value from given engine property
    virtual AnyReturnValue SyncFromEngine(const EnginePropertyParams& params, IAny& out) const = 0;
};

/**
 * @brief Internal engine value functions to get parameters et al
 */
class IEngineValueInternal : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEngineValueInternal, "86afd68c-7924-46b8-8bb8-aeace0029945")
public:
    /// Create any of the property type
    virtual IAny::Ptr CreateAny() const = 0;
    /// Get synchronisation access type for this engine value
    virtual IEngineInternalValueAccess::ConstPtr GetInternalAccess() const = 0;
    /// Get property parameters for this engine value
    virtual EnginePropertyParams GetPropertyParams() const = 0;
    /// Set property parameters for this engine value
    virtual bool SetPropertyParams(const EnginePropertyParams& p) = 0;
    /// Reset pending notifications, returns true if there were any.
    virtual bool ResetPendingNotify() = 0;
};

META_INTERFACE_TYPE(META_NS::IEngineValue)
META_END_NAMESPACE()

#endif
