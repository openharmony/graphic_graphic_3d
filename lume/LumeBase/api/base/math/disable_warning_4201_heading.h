/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
