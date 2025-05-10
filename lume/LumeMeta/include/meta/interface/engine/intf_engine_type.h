/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2024. All rights reserved.
 * Description: meta engine value manager
 * Author: Mikael Kilpeläinen
 * Create: 2024-10-17
 */

#ifndef META_ENGINE_INTERFACE_ENGINE_TYPE_H
#define META_ENGINE_INTERFACE_ENGINE_TYPE_H

#include <core/property/property.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Interface to access engine type info
 */
class IEngineType : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEngineType, "887149a1-8612-43de-98c0-1ed0053acb7e")
public:
    /// Get Core type declaration for associated type
    virtual CORE_NS::PropertyTypeDecl GetTypeDecl() const = 0;
};

META_END_NAMESPACE()

#endif
