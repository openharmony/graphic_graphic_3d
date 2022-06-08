/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_BASE_MATH_WARNING_DISABLE_FOOTER_H
#define API_BASE_MATH_WARNING_DISABLE_FOOTER_H

// We use headers instead of macros because # cannot be used within macros
// NOTE: warning C4201: nonstandard extension used: nameless struct/union
#if _MSC_VER
#pragma warning(pop)
#elif defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif // API_BASE_MATH_WARNING_DISABLE_FOOTER_H
