/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef NAPI_ERROR_UTIL_H
#define NAPI_ERROR_UTIL_H

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <string>
#include <napi_api.h>
#include <core/intf_logger.h>
#include "3d_widget_adapter_log.h"

namespace NapiErrorUtil {
enum Code : int32_t {
    OK = 0,

    SCENE_REF_LOST             = 10001,
    SCENE_NOT_INITIALIZED      = 10002,

    NATIVE_OBJ_NULL            = 20001,
    NATIVE_OBJ_TYPE_MISMATCH   = 20002,
    NATIVE_OBJ_DESTROYED       = 20003,

    INVALID_ARGUMENTS          = 30001,
    INVALID_ARGUMENT_TYPE      = 30002,
    INVALID_ARGUMENT_COUNT     = 30003,
    MISSING_REQUIRED_ARGUMENT  = 30004,

    INVALID_INSTANCE           = 40001,
    INTERFACE_NOT_SUPPORTED    = 40002,
    INSTANCE_EXPIRED           = 40003,

    PROPERTY_NOT_FOUND         = 50001,
    PROPERTY_READ_FAILED       = 50002,
    PROPERTY_WRITE_FAILED      = 50003,

    ASYNC_OPERATION_FAILED     = 60001,
    ASYNC_LOAD_FAILED          = 60002,
    ASYNC_CREATE_FAILED        = 60003,
    ASYNC_INVALID_UID          = 60004,

    INTERNAL_ERROR             = 70001,

    TRANSFER_FAILED            = 80001,
};

const char* ToString(Code code);

std::string FormatStringV(const char* fmt, va_list ap);

inline BASE_NS::string FormatErrorMessage(Code code, const BASE_NS::string& msg = {})
{
    BASE_NS::string result = BASE_NS::string("[").append(BASE_NS::to_string(static_cast<int32_t>(code))).append("] ");
    if (msg.empty()) {
        result.append(ToString(code));
    } else {
        result.append(ToString(code)).append(": ").append(msg);
    }
    return result;
}

FORMAT_FUNC(3, 4)
inline void ThrowBusinessError(napi_env env, Code code, const char* fmt, ...)
{
    if (!fmt) {
        WIDGET_LOGW("ThrowBusinessError: fmt is null");
        napi_throw_business_error(env, static_cast<int32_t>(code), ToString(code));
        return;
    }
    va_list ap;
    va_start(ap, fmt);
    std::string buf = FormatStringV(fmt, ap);
    va_end(ap);
    napi_throw_business_error(env, static_cast<int32_t>(code), buf.c_str());
}
} // namespace NapiErrorUtil

#endif // NAPI_ERROR_UTIL_H
