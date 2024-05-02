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

#ifndef META_SRC_ANY_H
#define META_SRC_ANY_H

#include <meta/interface/intf_any.h>

META_BEGIN_NAMESPACE()

class DummyAny : public IntroduceInterfaces<IAny, IValue> {
public:
    ObjectId GetClassId() const override
    {
        return {};
    }
    const BASE_NS::array_view<const TypeId> GetCompatibleTypes(CompatibilityDirection dir) const override
    {
        return {};
    }
    AnyReturnValue GetData(const TypeId& id, void* data, size_t size) const override
    {
        return AnyReturn::NOT_SUPPORTED;
    }
    AnyReturnValue SetData(const TypeId& id, const void* data, size_t size) override
    {
        return AnyReturn::NOT_SUPPORTED;
    }
    AnyReturnValue CopyFrom(const IAny& any) override
    {
        return AnyReturn::NOT_SUPPORTED;
    }
    IAny::Ptr Clone(const AnyCloneOptions& options) const override
    {
        if (options.role == TypeIdRole::ARRAY) {
            CORE_LOG_E("DummyAny: cloning into an array not supported.");
            return {};
        }
        return IAny::Ptr(new DummyAny);
    }
    TypeId GetTypeId(TypeIdRole) const override
    {
        return {};
    }
    BASE_NS::string GetTypeIdString() const override
    {
        return "";
    }
    AnyReturnValue SetValue(const IAny& value) override
    {
        return AnyReturn::NOT_SUPPORTED;
    }
    const IAny& GetValue() const override
    {
        return *this;
    }
    bool IsCompatible(const TypeId& id) const override
    {
        return false;
    }
};

inline const DummyAny INVALID_ANY {};

META_END_NAMESPACE()

#endif
