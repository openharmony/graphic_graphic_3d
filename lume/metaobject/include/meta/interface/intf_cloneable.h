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

#ifndef META_INTERFACE_ICLONEABLE_H
#define META_INTERFACE_ICLONEABLE_H

#include <meta/interface/intf_object.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(ICloneable, "f40a850c-5034-4cb8-87ad-4e9a2ddf587b")

/**
 * @brief The ICloneable interface to indicate entity can be cloned
 */
class ICloneable : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ICloneable)
public:
    /**
     * @brief Clone the current entity.
     */
    virtual BASE_NS::shared_ptr<CORE_NS::IInterface> GetClone() const = 0;
};

META_END_NAMESPACE()

#endif
