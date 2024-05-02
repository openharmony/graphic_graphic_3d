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
#ifndef META_SRC_NUMBER_H
#define META_SRC_NUMBER_H

#include <variant>

#include <meta/api/number.h>
#include <meta/interface/intf_any.h>

#include "base_object.h"

META_BEGIN_NAMESPACE()
namespace Internal {

class Number : public META_NS::Internal::BaseObjectFwd<Number, META_NS::ClassId::Number, IAny> {
public:
    using VariantType = std::variant<int64_t, uint64_t, float>;

    explicit Number(VariantType v = {});

    const BASE_NS::array_view<const TypeId> GetCompatibleTypes(CompatibilityDirection) const override;
    AnyReturnValue GetData(const TypeId& uid, void* data, size_t size) const override;
    AnyReturnValue SetData(const TypeId& uid, const void* data, size_t size) override;
    AnyReturnValue CopyFrom(const IAny& any) override;
    IAny::Ptr Clone(const AnyCloneOptions& options) const override;
    TypeId GetTypeId(TypeIdRole role) const override;
    BASE_NS::string GetTypeIdString() const override;

private:
    VariantType value_;
};

} // namespace Internal
META_END_NAMESPACE()

#endif
