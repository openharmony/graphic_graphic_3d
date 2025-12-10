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

#include <meta/ext/engine/core_any.h>
#include <meta/ext/engine/core_enum_any.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/engine/intf_engine_value.h>
#include <meta/interface/interface_helpers.h>

META_BEGIN_NAMESPACE()

namespace Internal {
template<typename T>
struct IsArray {
    static constexpr const bool VALUE = false;
    using ItemType = T;
};

template<typename T>
struct IsArray<BASE_NS::vector<T>> {
    static constexpr const bool VALUE = true;
    using ItemType = T;
};

template<typename Type, typename... CompatType>
IAny::Ptr ConstructCoreAny(const CORE_NS::Property& p)
{
    if constexpr (Internal::IsArray<Type>::VALUE) {
        using ItemType = typename Internal::IsArray<Type>::ItemType;
        if constexpr ((is_enum_v<Type> || ... || is_enum_v<CompatType>)) {
            return IAny::Ptr(new ArrayCoreEnumAny<ItemType, StaticCastConv, CompatType...>(p));
        }
        return IAny::Ptr(new ArrayCoreAny<ItemType, StaticCastConv, CompatType...>(p));
    } else {
        if constexpr ((is_enum_v<Type> || ... || is_enum_v<CompatType>)) {
            return IAny::Ptr(new CoreEnumAny<Type, StaticCastConv, CompatType...>(p));
        }
        return IAny::Ptr(new CoreAny<Type, StaticCastConv, CompatType...>(p));
    }
}
} // namespace Internal

/// Class that encapsulates the reading and writing to the core property and the any type used
template<typename Type, typename AccessType = Type>
class EngineInternalValueAccess : public IntroduceInterfaces<IEngineInternalValueAccess> {
public:
    IAny::Ptr CreateAny(const CORE_NS::Property& p) const override
    {
        if constexpr (BASE_NS::is_same_v<Type, AccessType>) {
            return Internal::ConstructCoreAny<AccessType>(p);
        } else {
            return Internal::ConstructCoreAny<AccessType, Type>(p);
        }
    }
    IAny::Ptr CreateSerializableAny() const override
    {
        if constexpr (Internal::IsArray<AccessType>::VALUE) {
            using ItemType = typename Internal::IsArray<AccessType>::ItemType;
            return IAny::Ptr(new ArrayAnyType<ItemType> {});
        } else {
            return IAny::Ptr(new AnyType<AccessType> {});
        }
    }
    bool IsCompatible(const CORE_NS::PropertyTypeDecl& type) const override
    {
        return MetaType<Type>::coreType == type;
    }
    template<typename T>
    static T* CalcLocation(T* data, const EnginePropertyParams& params)
    {
        uintptr_t offset = uintptr_t(data) + params.Offset();
        auto cont = params.containerMethods;
        if (cont) {
            auto arroffset = uintptr_t(data) + params.arraySubsOffset;
            if (params.index >= cont->size(arroffset)) {
                return nullptr;
            }
            offset = uintptr_t(cont->get(arroffset, params.index)) + params.Offset();
        }
        return (T*)offset;
    }

    using SyncTempType = BASE_NS::conditional_t<Internal::IsArray<Type>::VALUE,
        ArrayAny<typename Internal::IsArray<Type>::ItemType>, Any<Type>>;

    AnyReturnValue SyncToEngine(const IAny& value, const EnginePropertyParams& params) const override
    {
        SyncTempType temp;
        AnyReturnValue res = temp.CopyFrom(value) ? SyncFromEngine(params, temp) : AnyReturn::FAIL;
        if (res == AnyReturn::SUCCESS) {
            if (CORE_NS::ScopedHandle<Type> guard { params.handle.Handle() }) {
                if (Type* location = CalcLocation(&*guard, params)) {
                    return value.GetData(
                        UidFromType<Type>(), location, sizeof(Type)); /*NOLINT(bugprone-sizeof-expression)*/
                }
            }
            res = AnyReturn::FAIL;
        }
        return res;
    }
    AnyReturnValue SyncFromEngine(const EnginePropertyParams& params, IAny& out) const override
    {
        if (CORE_NS::ScopedHandle<const Type> guard { params.handle.Handle() }) {
            if (const Type* location = CalcLocation(&*guard, params)) {
                return out.SetData(UidFromType<Type>(), location, sizeof(Type)); /*NOLINT(bugprone-sizeof-expression)*/
            }
        }
        return AnyReturn::FAIL;
    }
};

/// Class that encapsulates the reading and writing to the core array property and the any type used
template<typename Type>
class EngineInternalArrayValueAccess : public IntroduceInterfaces<IEngineInternalValueAccess> {
public:
    using InternalType = BASE_NS::vector<Type>;

    IAny::Ptr CreateAny(const CORE_NS::Property& p) const override
    {
        return Internal::ConstructCoreAny<InternalType>(p);
    }
    IAny::Ptr CreateSerializableAny() const override
    {
        return IAny::Ptr(new ArrayAnyType<Type> {});
    }
    bool IsCompatible(const CORE_NS::PropertyTypeDecl& type) const override
    {
        return MetaType<Type[]>::coreType == type;
    }
    AnyReturnValue SyncToEngine(const IAny& value, const EnginePropertyParams& params) const override
    {
        AnyReturnValue res = AnyReturn::FAIL;
        ArrayAny<Type> temp;
        if (SyncFromEngine(params, temp) && (res = temp.CopyFrom(value)) == AnyReturn::SUCCESS) {
            CORE_NS::ScopedHandle<Type[]> guard { params.handle.Handle() };
            if (guard && params.property.metaData.containerMethods) {
                const BASE_NS::vector<Type>& vec = temp.InternalGetValue();
                if (params.property.type.isArray) {
                    size_t size = params.property.count < vec.size() ? params.property.count : vec.size();
                    for (size_t i = 0; i != size; ++i) {
                        ((Type*)((uintptr_t)(&(*guard)) + params.Offset()))[i] = vec[i];
                    }
                } else {
                    auto cont = params.property.metaData.containerMethods;
                    cont->resize((uintptr_t)(&(*guard)) + params.Offset(), vec.size());
                    for (size_t i = 0; i != vec.size(); ++i) {
                        *((Type*)cont->get((uintptr_t)(&(*guard)) + params.Offset(), i)) = vec[i];
                    }
                }
            }
        }
        return res;
    }
    AnyReturnValue SyncFromEngine(const EnginePropertyParams& params, IAny& out) const override
    {
        AnyReturnValue res = AnyReturn::FAIL;
        CORE_NS::ScopedHandle<const Type[]> guard { params.handle.Handle() };
        if (guard && params.property.metaData.containerMethods) {
            BASE_NS::vector<Type> vec;
            if (params.property.type.isArray) {
                vec.resize(params.property.count);
                for (size_t i = 0; i != vec.size(); ++i) {
                    vec[i] = ((const Type*)((uintptr_t)(&(*guard)) + params.Offset()))[i];
                }
            } else {
                auto cont = params.property.metaData.containerMethods;
                vec.resize(cont->size((uintptr_t)(&(*guard)) + params.Offset()));
                for (size_t i = 0; i != vec.size(); ++i) {
                    vec[i] = *((const Type*)cont->get((uintptr_t)(&(*guard)) + params.Offset(), i));
                }
            }
            res = out.SetData(UidFromType<InternalType>(), &vec, sizeof(InternalType));
        }
        return res;
    }
};

META_END_NAMESPACE()

#endif
