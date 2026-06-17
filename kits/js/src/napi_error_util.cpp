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

#include "napi_error_util.h"

namespace NapiErrorUtil {

const char* ToString(Code code)
{
    switch (code) {
        case OK:                        return "OK";
        case SCENE_REF_LOST:            return "Scene reference lost";
        case SCENE_NOT_INITIALIZED:     return "Scene is not initialized";
        case NATIVE_OBJ_NULL:           return "Native object is null";
        case NATIVE_OBJ_TYPE_MISMATCH:  return "Native object type mismatch";
        case NATIVE_OBJ_DESTROYED:      return "Native object has been destroyed";
        case INVALID_ARGUMENTS:         return "Invalid arguments";
        case INVALID_ARGUMENT_TYPE:     return "Invalid argument type";
        case INVALID_ARGUMENT_COUNT:    return "Invalid argument count";
        case MISSING_REQUIRED_ARGUMENT: return "Missing required argument";
        case INVALID_INSTANCE:          return "Invalid instance";
        case INTERFACE_NOT_SUPPORTED:   return "Interface not supported";
        case INSTANCE_EXPIRED:          return "Instance has expired";
        case PROPERTY_NOT_FOUND:        return "Property not found";
        case PROPERTY_READ_FAILED:      return "Property read failed";
        case PROPERTY_WRITE_FAILED:     return "Property write failed";
        case ASYNC_OPERATION_FAILED:    return "Async operation failed";
        case ASYNC_LOAD_FAILED:         return "Async resource load failed";
        case ASYNC_CREATE_FAILED:       return "Async object creation failed";
        case ASYNC_INVALID_UID:         return "Invalid UID string";
        case INTERNAL_ERROR:            return "Internal error";
        case TRANSFER_FAILED:           return "Transfer operation failed";
        default:                        return "Undefined error code";
    }
}

std::string FormatStringV(const char* fmt, va_list ap)
{
    va_list apCopy;
    va_copy(apCopy, ap);
    // vsnprintf_s does not support nullptr with size 0 for size calculation
    int size = vsnprintf(nullptr, 0, fmt, apCopy);
    va_end(apCopy);
    if (size < 0) {
        return {};
    }
    std::string buf(size + 1, '\0');
    va_copy(apCopy, ap);
    int written = vsnprintf_s(buf.data(), buf.size(), buf.size() - 1, fmt, apCopy);
    va_end(apCopy);
    if (written < 0) {
        return {};
    }
    buf.resize(written);
    return buf;
}

} // namespace NapiErrorUtil
