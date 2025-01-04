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

#ifndef META_INTERFACE_ICOMPARABLE_H
#define META_INTERFACE_ICOMPARABLE_H

#include <core/plugin/intf_interface.h>

#include <meta/base/types.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IComparable, "aae8a240-8a99-4edc-869e-3b87f42e37a0")

/**
 * @brief The IComparable interface to indicate entity can be compared
 */
class IComparable : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IComparable)
public:
    /**
     * @brief Compare given object to this one.
     *        0 on equal, negative if this is less than, positive if this is greater than.
     */
    virtual int Compare(const IComparable::Ptr&) const = 0;
};

META_END_NAMESPACE()

#endif
