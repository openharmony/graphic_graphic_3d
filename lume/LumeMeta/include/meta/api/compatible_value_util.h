/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: any comaptible value helpers
 * Author: Mikael Kilpeläinen
 * Create: 2024-11-12
 */

#ifndef META_API_COMPATIBLE_VALUE_UTIL_H
#define META_API_COMPATIBLE_VALUE_UTIL_H

#include <meta/api/util.h>

META_BEGIN_NAMESPACE()

namespace Internal {
template<typename Type, typename Out>
AnyReturnValue ExtractAnyValue(const IAny& value, Out& out)
{
    Type v {};
    auto res = value.GetValue<Type>(v);
    if (res) {
        out = static_cast<Out>(v);
    }
    return res;
}
} // namespace Internal

/**
 * @brief Try to get value from property using list of known types
 * @param value Any to get value from
 * @param out Output value, type of this has to be compatible with the list of types given (conversion with static_cast)
 * @param TypeList Type list of types which are tried for the extraction
 * @return Result of getting the value.
 */
template<typename Type, typename... Builtins>
AnyReturnValue GetCompatibleValue(const IAny& value, Type& out, TypeList<Builtins...>)
{
    AnyReturnValue res = AnyReturn::FAIL;
    [[maybe_unused]] bool r = ((META_NS::IsCompatible(value, GetTypeId<Builtins>())
                                       ? (res = Internal::ExtractAnyValue<Builtins>(value, out), true)
                                       : false) ||
                               ...);
    return res;
}
/**
 * @brief Try to set value to property using list of known types
 * @param value, type of this has to be compatible with the list of types given (conversion with static_cast)
 * @param any Any to set value to
 * @param TypeList Type list of types which are tried for the insertion
 * @return Result of setting the value.
 */
template<typename Type, typename... Builtins>
AnyReturnValue SetCompatibleValue(const Type& value, IAny& any, TypeList<Builtins...>)
{
    AnyReturnValue res = AnyReturn::FAIL;
    [[maybe_unused]] bool r =
        ((META_NS::IsCompatible(any, GetTypeId<Builtins>()) ? (res = any.SetValue(static_cast<Builtins>(value)), true)
                                                            : false) ||
            ...);
    return res;
}

META_END_NAMESPACE()

#endif
