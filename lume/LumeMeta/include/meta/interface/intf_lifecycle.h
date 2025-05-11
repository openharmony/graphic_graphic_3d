/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Definition of ILifecycle interface
 * Author: Mikael Kilpeläinen
 * Create: 2023-03-10
 */

#ifndef META_INTERFACE_ILIFECYCLE_H
#define META_INTERFACE_ILIFECYCLE_H

#include <core/plugin/intf_interface.h>

#include <meta/base/interface_macros.h>
#include <meta/base/types.h>

META_BEGIN_NAMESPACE()

class IMetadata;

META_REGISTER_INTERFACE(ILifecycle, "643ebb4f-bf7b-43b0-9900-9082b992ca0e")
/**
 * @brief Interface for constructing and destructing objects
 * Constructing:
 *  - Build is called automatically for all objects constructed by Object Registry,
 *    base-classes are build first and then derived.
 *  - Destroy is called automatically just before the object is about to be destroyed if META_IMPLEMENT_REF_COUNT has
 *    been used (This happens for each separate object in the hierarchy, the top most object first).
 */
class ILifecycle : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ILifecycle)
public:
    /**
     * @brief Called by the framework when this object should build its content.
     * @param parameters User provided arbitrary parameters for construction.
     * @return True if successful, otherwise false.
     */
    virtual bool Build(const BASE_NS::shared_ptr<IMetadata>& parameters) = 0;
    /**
     * @brief Assign the instance UID.
     * @warning This should ONLY be called during object creation, since framework automatically
     *          assigns a unique id for each new object.
     * @param id New identity for object.
     */
    virtual void SetInstanceId(InstanceId) = 0;
    /**
     * @brief Called by the framework when this object is about to be destroyed
     */
    virtual void Destroy() = 0;
};

META_END_NAMESPACE()

#endif
