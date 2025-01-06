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
