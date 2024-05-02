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
#include "number.h"

#include <limits>

META_BEGIN_NAMESPACE()
namespace Internal {

template<typename T>
struct Type {};

template<typename T>
static Number::VariantType MapToVariant(const T& v)
{
    if constexpr (std::numeric_limits<T>::is_integer) {
        if constexpr (std::numeric_limits<T>::is_signed) {
            return Number::VariantType(static_cast<int64_t>(v));
        }
        return Number::VariantType(static_cast<uint64_t>(v));
    }
    return Number::VariantType(static_cast<float>(v));
}

struct CompType {
    template<typename T>
    constexpr CompType(Type<T>)
        : uid(UidFromType<T>()), size(sizeof(T)), load([](const void* data) {
              T v = *static_cast<const T*>(data);
              return MapToVariant(v);
        }),
        save([](Number::VariantType var, void* data) {
            T v {};
            std::visit([&](const auto& arg) {
                v = static_cast<T>(arg);
            }, var);
            *static_cast<T*>(data) = v;
        }),
        loadAny([](const IAny& any, Number::VariantType& var) {
            T v {};
            bool res = any.GetValue(v);
            if (res) {
                var = MapToVariant(v);
            }
            return res;
        })
    {}

    using LoadFunc = Number::VariantType(const void*);
    using SaveFunc = void(Number::VariantType, void*);
    using LoadAnyFunc = bool(const IAny&, Number::VariantType&);

    const TypeId uid;
    const size_t size;
    LoadFunc* const load;
    SaveFunc* const save;
    LoadAnyFunc* const loadAny;
};

constexpr CompType COMPATIBLES[] = {
    Type<float>(),
    Type<double>(),
    Type<bool>(),
    Type<uint8_t>(),
    Type<uint16_t>(),
    Type<uint32_t>(),
    Type<uint64_t>(),
    Type<int8_t>(),
    Type<int16_t>(),
    Type<int32_t>(),
    Type<int64_t>(),
};

static BASE_NS::vector<TypeId> CompatibleTypes()
{
    BASE_NS::vector<TypeId> res;
    for (auto&& v : COMPATIBLES) {
        res.push_back(v.uid);
    }
    return res;
}

static const CompType* FindCompatible(const TypeId& uid, size_t size)
{
    for (auto&& v : COMPATIBLES) {
        if (v.uid == uid && v.size == size) {
            return &v;
        }
    }
    return nullptr;
}

static const CompType* FindCompatible(const IAny& any)
{
    auto anyComps = any.GetCompatibleTypes(CompatibilityDirection::BOTH);
    for (auto&& v : COMPATIBLES) {
        for (auto&& av : anyComps) {
            if (v.uid == av) {
                return &v;
            }
        }
    }
    return nullptr;
}

Number::Number(VariantType v) : value_(v) {}
const BASE_NS::array_view<const TypeId> Number::GetCompatibleTypes(CompatibilityDirection dir) const
{
    static BASE_NS::vector<TypeId> types = CompatibleTypes();
    return types;
}
AnyReturnValue Number::GetData(const TypeId& uid, void* data, size_t size) const
{
    if (auto c = FindCompatible(uid, size)) {
        c->save(value_, data);
        return AnyReturn::SUCCESS;
    }
    return AnyReturn::INCOMPATIBLE_TYPE;
}
AnyReturnValue Number::SetData(const TypeId& uid, const void* data, size_t size)
{
    if (auto c = FindCompatible(uid, size)) {
        value_ = c->load(data);
        return AnyReturn::SUCCESS;
    }
    return AnyReturn::INCOMPATIBLE_TYPE;
}
AnyReturnValue Number::CopyFrom(const IAny& any)
{
    if (auto c = FindCompatible(any)) {
        c->loadAny(any, value_);
    }
    return AnyReturn::INCOMPATIBLE_TYPE;
}
IAny::Ptr Number::Clone(const AnyCloneOptions& options) const
{
    if (options.role == TypeIdRole::ARRAY) {
        CORE_LOG_E("Number: cloning into an array not supported.");
        return {};
    }
    return IAny::Ptr(new Number(options.value == CloneValueType::COPY_VALUE ? value_ : VariantType {}));
}
TypeId Number::GetTypeId(TypeIdRole role) const
{
    if (role == TypeIdRole::ARRAY) {
        return std::visit([&](auto arg) { return ArrayUidFromType<decltype(arg)>(); }, value_);
    }
    return std::visit([&](auto arg) { return UidFromType<decltype(arg)>(); }, value_);
}

BASE_NS::string Number::GetTypeIdString() const
{
    return std::visit([&](auto arg) { return MetaType<decltype(arg)>::name; }, value_);
}

} // namespace Internal
META_END_NAMESPACE()
