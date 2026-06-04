#ifndef NAPI_ERROR_UTIL_H
#define NAPI_ERROR_UTIL_H

#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>
#include <napi_api.h>
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

inline const char* ToString(Code code)
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
        case TRANSFER_FAILED:            return "Transfer operation failed";
        default:                        return "Undefined error code";
    }
}

template<typename... Args>
inline void ThrowBusinessError(napi_env env, Code code, const char* fmt, Args&&... args)
{
    if (!fmt) {
        WIDGET_LOGW("ThrowBusinessError: fmt is null");
        napi_throw_business_error(env, static_cast<int32_t>(code), ToString(code));
        return;
    }
    int size = std::snprintf(nullptr, 0, fmt, std::forward<Args>(args)...);
    if (size < 0) return;

    std::string buf(size + 1, '\0');
    std::snprintf(buf.data(), buf.size(), fmt, std::forward<Args>(args)...);

    napi_throw_business_error(env, static_cast<int32_t>(code), buf.c_str());
}

inline BASE_NS::string FormatErrorMessage(Code code, const BASE_NS::string& msg = {})
{
    BASE_NS::string result = BASE_NS::string("[")
        .append(BASE_NS::to_string(static_cast<int32_t>(code)))
        .append("] ");
    if (msg.empty()) {
        result.append(ToString(code));
    } else {
        result.append(msg);
    }
    return result;
}
} // namespace NapiErrorUtil

#endif // NAPI_ERROR_UTIL_H
