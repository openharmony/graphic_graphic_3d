/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SCENE_IMP_SRC_NUMERIC_ANY_H
#define SCENE_IMP_SRC_NUMERIC_ANY_H

#include <scene_importer/base/namespace.h>

#include <meta/interface/detail/multi_type_any.h>

SCENE_IMP_BEGIN_NAMESPACE()

/// IAny that stores Type but supports Get/Set via all CompatType... through static_cast.
template <typename Type, typename... CompatType>
class NumericAny : public META_NS::MultiTypeAny<Type, META_NS::StaticCastConv, CompatType...> {
public:
    explicit NumericAny(Type v = {}) : value_(v)
    {}

    using META_NS::IAny::Clone;
    META_NS::IAny::Ptr Clone(const META_NS::AnyCloneOptions& options) const override
    {
        if (options.role == META_NS::TypeIdRole::ARRAY) {
            return META_NS::IAny::Ptr(new META_NS::ArrayMultiTypeAnyBase<Type>());
        }
        return META_NS::IAny::Ptr(
            new NumericAny{options.value == META_NS::CloneValueType::COPY_VALUE ? value_ : Type{}});
    }

private:
    META_NS::AnyReturnValue InternalSetValue(const Type& value) override
    {
        if (value != value_) {
            value_ = value;
            return META_NS::AnyReturn::SUCCESS;
        }
        return META_NS::AnyReturn::NOTHING_TO_DO;
    }
    const Type& InternalGetValue() const override
    {
        return value_;
    }

    Type value_{};
};

using FloatingAny = NumericAny<double, float>;
using SignedIntAny = NumericAny<int64_t, int32_t, int16_t, int8_t, double, float>;
using UnsignedIntAny =
    NumericAny<uint64_t, uint32_t, uint16_t, uint8_t, int64_t, int32_t, int16_t, int8_t, double, float>;

inline META_NS::IAny::Ptr MakeFloatingAny(double v)
{
    return META_NS::IAny::Ptr(new FloatingAny(v));
}

inline META_NS::IAny::Ptr MakeSignedIntAny(int64_t v)
{
    return META_NS::IAny::Ptr(new SignedIntAny(v));
}

inline META_NS::IAny::Ptr MakeUnsignedIntAny(uint64_t v)
{
    return META_NS::IAny::Ptr(new UnsignedIntAny(v));
}

SCENE_IMP_END_NAMESPACE()

#endif
