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

#ifndef META_INTERFACE_IENUM_H
#define META_INTERFACE_IENUM_H

#include <cstdint>

#include <meta/interface/intf_info.h>

META_BEGIN_NAMESPACE()

enum class EnumType { SINGLE_VALUE, BIT_FIELD };

struct EnumValue {
    BASE_NS::string_view name; /// Name of the value
    BASE_NS::string_view desc; /// Description of the value
};

/**
 * @brief Interface for enum like types
 */
class IEnum : public IInfo {
    META_INTERFACE(IInfo, IEnum, "bf116c56-a949-45d5-a12a-5394369861fb")
public:
    /// Get which type of enum this is
    virtual EnumType GetEnumType() const = 0;
    /// Get array of the possible enum values
    virtual BASE_NS::array_view<const EnumValue> GetValues() const = 0;
    /// Get index of the current value, returns -1 for values not returned by GetValues (like bit fields)
    virtual size_t GetValueIndex() const = 0;
    /// Set current value based on the index
    virtual bool SetValueIndex(size_t index) = 0;
    /// Return true if the value with given index is set
    virtual bool IsValueSet(size_t index) const = 0;
    /// Set value (bit) in given index to on or off, works only for bit fields
    virtual bool FlipValue(size_t index, bool isSet) = 0;
};

META_END_NAMESPACE()

#endif
