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
#ifndef META_BASE_ALGORITHMS_H
#define META_BASE_ALGORITHMS_H

#include <stdint.h>

#include <base/containers/type_traits.h>
#include <base/containers/vector.h>

#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()

constexpr size_t NPOS = -1;

template<typename Type>
size_t FindFirstOf(const BASE_NS::vector<Type>& vec, const Type& value, size_t pos = 0)
{
    for (; pos < vec.size(); ++pos) {
        if (vec[pos] == value) {
            return pos;
        }
    }
    return NPOS;
}

META_END_NAMESPACE()

#endif
