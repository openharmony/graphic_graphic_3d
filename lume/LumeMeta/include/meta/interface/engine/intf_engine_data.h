/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2024. All rights reserved.
 * Description: meta engine value manager
 * Author: Mikael Kilpeläinen
 * Create: 2024-02-19
 */

#ifndef META_ENGINE_INTERFACE_ENGINE_DATA_H
#define META_ENGINE_INTERFACE_ENGINE_DATA_H

#include <meta/interface/engine/intf_engine_value.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Interface to access and register engine values
 */
class IEngineData : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEngineData, "542b7e13-21bd-4c9c-8d3f-a9527732216c")
public:
    /**
     * @brief Get engine access for core type
     * @param type Core type for engine value
     */
    virtual IEngineInternalValueAccess::Ptr GetInternalValueAccess(const CORE_NS::PropertyTypeDecl& type) const = 0;
    /**
     * @brief Register engine access for core type
     */
    virtual void RegisterInternalValueAccess(
        const CORE_NS::PropertyTypeDecl& type, IEngineInternalValueAccess::Ptr) = 0;
    /**
     * @brief Unregister engine access for core type
     */
    virtual void UnregisterInternalValueAccess(const CORE_NS::PropertyTypeDecl& type) = 0;
};

META_END_NAMESPACE()

#endif
