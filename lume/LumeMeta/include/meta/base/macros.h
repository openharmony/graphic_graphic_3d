/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Common helper macros
 * Author: Mikael Kilpeläinen
 * Create: 2023-08-29
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
