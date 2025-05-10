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

// clang-format off


// compile and version specific, warning available since vs 2022
#if defined(__clang__) || defined(__GNUC__)
#define W_SUPPRESS_UNUSED _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"")
#elif defined(_MSC_VER) && _MSC_VER > 1934
#define W_SUPPRESS_UNUSED _Pragma ("warning(disable : 5264)")
#elif defined(_MSC_VER) && _MSC_VER <= 1934
#define W_SUPPRESS_UNUSED
#else
#define W_SUPPRESS_UNUSED
#endif

// compile and version specific, warning available since vs 2017
#if defined(__clang__) || defined(__GNUC__)
#define W_THIS_USED_BASE_INITIALIZER_LIST
#elif defined(_MSC_VER) && _MSC_VER > 1934
#define W_THIS_USED_BASE_INITIALIZER_LIST _Pragma ("warning(disable : 4355)")
#elif defined(_MSC_VER) && _MSC_VER <= 1934
#define W_THIS_USED_BASE_INITIALIZER_LIST __pragma(warning(disable:4355))
#else
#define W_THIS_USED_BASE_INITIALIZER_LIST
#endif

// compile and version specific push and pop scopes
#if defined(__clang__) || defined(__GNUC__)
#define WARNING_SCOPE_START(...)    \
    _Pragma("GCC diagnostic push"); \
    __VA_ARGS__;
#define WARNING_SCOPE_END() _Pragma("GCC diagnostic pop");
#elif defined(_MSC_VER) && _MSC_VER > 1916
#define WARNING_SCOPE_START(...) \
    _Pragma("warning(push)");    \
    ##__VA_ARGS__;
#define WARNING_SCOPE_END() _Pragma("warning(pop)");
#elif defined(_MSC_VER) && _MSC_VER <= 1916
#define WARNING_SCOPE_START(...) \
    __pragma(warning(push)) ;    \
    ##__VA_ARGS__;
#define WARNING_SCOPE_END() __pragma(warning(pop)) ;
#else
#define WARNING_SCOPE_START(...)
#define WARNING_SCOPE_END()
#endif
// clang-format on