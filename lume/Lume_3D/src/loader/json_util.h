/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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