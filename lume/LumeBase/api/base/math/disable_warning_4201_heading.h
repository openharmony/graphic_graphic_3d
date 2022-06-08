/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_BASE_MATH_WARNING_DISABLE_HEADING_H
#define API_BASE_MATH_WARNING_DISABLE_HEADING_H

// We use headers instead of macros because # cannot be used within macros
// NOTE: warning C4201: nonstandard extension used: nameless struct/union
#if _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4201)
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

#endif // API_BASE_MATH_WARNING_DISABLE_HEADING_H
