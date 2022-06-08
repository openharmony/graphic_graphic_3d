/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE__LOADER__JSON_UTIL_H
#define CORE__LOADER__JSON_UTIL_H

#include <cstdlib>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <core/json/json.h>
#include <core/log.h>
#include <core/namespace.h>

#include "util/string_util.h"

CORE3D_BEGIN_NAMESPACE()
template<class T, BASE_NS::enable_if_t<BASE_NS::is_arithmetic_v<T>, bool> = true>
inline bool SafeGetJsonValue(
    const CORE_NS::json::value& jsonData, const BASE_NS::string_view element, BASE_NS::string& error, T& output)
{
    if (auto const pos = jsonData.find(element); pos) {
        if (pos->is_number()) {
            output = pos->as_number<T>();
            return true;
        } else {
            error += element + ": expected number.\n";
        }
    }
    return false;
}

template<class T, BASE_NS::enable_if_t<BASE_NS::is_convertible_v<T, BASE_NS::string_view>, bool> = true>
inline bool SafeGetJsonValue(
    const CORE_NS::json::value& jsonData, const BASE_NS::string_view element, BASE_NS::string& error, T& output)
{
    if (auto const pos = jsonData.find(element); pos) {
        if (pos->is_string()) {
            output = T(pos->string_.data(), pos->string_.size());
            return true;
        } else {
            error += element + ": expected string.\n";
        }
    }
    return false;
}
CORE3D_END_NAMESPACE()

#endif