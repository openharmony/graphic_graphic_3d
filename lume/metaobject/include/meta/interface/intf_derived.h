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

#ifndef META_INTERFACE_IDERIVED_H
#define META_INTERFACE_IDERIVED_H

#include <meta/interface/intf_object.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IDerived, "d471a8d7-16fe-4b3e-9ff9-fa8787271e3f");
/**
 * @brief Defines that the object type is derived from another object type.
 *        This is part of the ObjectRegistry object construction machinery.
 */
class IDerived : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IDerived)
public:
    /**
     * @brief Called by the framework. (part of the aggregate part creation). Implementer receives the actual object
     * "aggr" and the requested superclass implementation "impl".
     */
    virtual void SetSuperInstance(const IObject::Ptr& aggr, const IObject::Ptr& impl) = 0;

    /**
     * @brief Called by the framework. (part of the aggregate part creation), if implementer does not need a super class
     * return "empty".
     */
    virtual BASE_NS::Uid GetSuperClassUid() const = 0;
};

META_END_NAMESPACE()

#endif
