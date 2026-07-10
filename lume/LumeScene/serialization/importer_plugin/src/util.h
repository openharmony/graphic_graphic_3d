/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SCENE_IMP_SRC_UTIL_H
#define SCENE_IMP_SRC_UTIL_H

#include <limits>

#include "config.h"

SCENE_IMP_BEGIN_NAMESPACE()

template<typename RangeType, typename Type>
bool IsValueInRange(const Type& v)
{
    if constexpr (std::numeric_limits<Type>::is_signed) {
        if (v < 0) {
            if constexpr (!std::numeric_limits<RangeType>::is_signed) {
                return false;
            } else {
                return v >= std::numeric_limits<RangeType>::lowest();
            }
        }
    }
    return v <= std::numeric_limits<RangeType>::max();
}

SCENE_IMP_END_NAMESPACE()

#endif