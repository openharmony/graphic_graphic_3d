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

#ifndef UTIL_ALIGN_UTIL_H
#define UTIL_ALIGN_UTIL_H

#include <cstdint>

#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()

constexpr uint32_t Align(uint32_t value, uint32_t align) noexcept
{
    if (value == 0 || align == 0) {
        return value;
    }
    const uint64_t temp = static_cast<uint64_t>(value) + align - 1U;
    const uint64_t aligned = (temp / align) * align;
    return (aligned <= UINT32_MAX) ? static_cast<uint32_t>(aligned) : UINT32_MAX;
}

RENDER_END_NAMESPACE()

#endif  // UTIL_ALIGN_UTIL_H
