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

#ifndef META_EXT_ENGINE_INTERNAL_ACCESS_H
#define META_EXT_ENGINE_INTERNAL_ACCESS_H

#include <core/property/scoped_handle.h>

#include <meta/interface/detail/any.h>
#include <meta/interface/engine/intf_engine_value.h>
#include <meta/interface/interface_helpers.h>

META_BEGIN_NAMESPACE()

template<typename Type, typename AnyType, typename AccessType = Type>
class EngineInternalValueAccessImpl : public IntroduceInterfaces<IEngineInternalValueAccess> {
public:
    IAny::Ptr CreateAny() const override
    {
        return IAny::Ptr(new AnyType);
    }
    bool IsCompatible(const CORE_NS::PropertyTypeDecl& type) const override
    {
        return MetaType<Type>::coreType == type;
    }
    AnyReturnValue SyncToEngine(const IAny& value, const EnginePropertyParams& params) const override
    {
        CORE_NS::ScopedHandle<Type> guard { params.handle };
        return guard ? value.GetData(UidFromType<AccessType>(), (void*)((uintptr_t) & *guard + params.Offset()),
                           sizeof(Type)) /*NOLINT(bugprone-sizeof-expression)*/
                     : AnyReturn::FAIL;
    }
    AnyReturnValue SyncFromEngine(const EnginePropertyParams& params, IAny& out) const override
    {
        CORE_NS::ScopedHandle<const Type> guard { params.handle };
        return guard ? out.SetData(UidFromType<AccessType>(), (const void*)((uintptr_t) & *guard + params.Offset()),
                           sizeof(Type)) /*NOLINT(bugprone-sizeof-expression)*/
                     : AnyReturn::FAIL;
    }
};

template<typename Type>
class EngineInternalValueAccess : public EngineInternalValueAccessImpl<Type, Any<Type>> {};
template<typename Type>
class EngineInternalValueAccess<BASE_NS::vector<Type>>
    : public EngineInternalValueAccessImpl<BASE_NS::vector<Type>, ArrayAny<Type>> {};

template<typename Type>
class EngineInternalArrayValueAccess : public IntroduceInterfaces<IEngineInternalValueAccess> {
public:
    using InternalType = BASE_NS::vector<Type>;

    IAny::Ptr CreateAny() const override
    {
        return IAny::Ptr(new ArrayAny<Type>);
    }
    bool IsCompatible(const CORE_NS::PropertyTypeDecl& type) const override
    {
        return MetaType<Type[]>::coreType == type;
    }
    AnyReturnValue SyncToEngine(const IAny& value, const EnginePropertyParams& params) const override
    {
        AnyReturnValue res = AnyReturn::FAIL;
        CORE_NS::ScopedHandle<Type[]> guard { params.handle };
        if (guard && params.property.metaData.containerMethods) {
            BASE_NS::vector<Type> vec;
            res = value.GetData(UidFromType<InternalType>(), &vec, sizeof(InternalType));
            if (res) {
                if (params.property.type.isArray) {
                    size_t size = params.property.count < vec.size() ? params.property.count : vec.size();
                    for (size_t i = 0; i != size; ++i) {
                        ((Type*)((uintptr_t) & *guard + params.Offset()))[i] = vec[i];
                    }
                } else {
                    auto cont = params.property.metaData.containerMethods;
                    cont->resize(params.Offset(), vec.size());
                    for (size_t i = 0; i != vec.size(); ++i) {
                        *((Type*)cont->get(params.Offset(), i)) = vec[i];
                    }
                }
            }
        }
        return res;
    }
    AnyReturnValue SyncFromEngine(const EnginePropertyParams& params, IAny& out) const override
    {
        AnyReturnValue res = AnyReturn::FAIL;
        CORE_NS::ScopedHandle<const Type[]> guard { params.handle };
        if (guard && params.property.metaData.containerMethods) {
            BASE_NS::vector<Type> vec;
            if (params.property.type.isArray) {
                vec.resize(params.property.count);
                for (size_t i = 0; i != vec.size(); ++i) {
                    vec[i] = ((const Type*)((uintptr_t) & *guard + params.Offset()))[i];
                }
            } else {
                auto cont = params.property.metaData.containerMethods;
                vec.resize(cont->size(params.Offset()));
                for (size_t i = 0; i != vec.size(); ++i) {
                    vec[i] = *((const Type*)cont->get(params.Offset(), i));
                }
            }
            res = out.SetData(UidFromType<InternalType>(), &vec, sizeof(InternalType));
        }
        return res;
    }
};

META_END_NAMESPACE()

#endif