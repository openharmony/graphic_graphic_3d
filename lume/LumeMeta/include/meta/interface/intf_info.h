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

#ifndef META_INTERFACE_IINFO_H
#define META_INTERFACE_IINFO_H

#include <cstdint>

#include <core/plugin/intf_interface.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Information for entity
 */
class IInfo : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IInfo, "4fc7656f-f6f2-43a9-a1f6-10f03d393d1f")
public:
    /// Name of entity
    virtual BASE_NS::string_view GetName() const = 0;
    /// Description of entity
    virtual BASE_NS::string_view GetDescription() const = 0;
};

META_END_NAMESPACE()

#endif
