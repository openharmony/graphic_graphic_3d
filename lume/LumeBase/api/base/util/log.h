/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_BASE_UTIL_LOG_H
#define API_BASE_UTIL_LOG_H

#ifndef NDEBUG
#include <cassert>
#define BASE_ASSERT(cond) assert(cond);
#define BASE_ASSERT_MSG(cond, msg) assert((cond) && (msg));

#include <cstdio>
#define BASE_LOG_E(...) ::fprintf(stderr, __VA_ARGS__);
#else
#define BASE_ASSERT(cond)
#define BASE_ASSERT_MSG(cond, msg)
#define BASE_LOG_E(...)
#endif

#endif // API_BASE_UTIL_LOG_H
