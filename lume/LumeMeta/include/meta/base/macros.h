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

#ifndef META_BASE_MACROS_H
#define META_BASE_MACROS_H

// older versions of msvc (prior Visual Studio 2019 version 16.5) consider __VA_ARGS__ as single token
// using META_EXPAND is the workaround to threat it as multiple tokens
#ifndef META_EXPAND
#define META_EXPAND(x) x
#endif

// machinery used to implement macro "overloading" based on number of arguments
#define META_GET_MACRO2_IMPL(p0, p1, macro, ...) macro
#define META_GET_MACRO3_IMPL(p0, p1, p2, macro, ...) macro
#define META_GET_MACRO4_IMPL(p0, p1, p2, p3, macro, ...) macro
#define META_GET_MACRO5_IMPL(p0, p1, p2, p3, p4, macro, ...) macro

#endif
