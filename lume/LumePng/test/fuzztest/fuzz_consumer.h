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

#ifndef FUZZTEST_FUZZ_CONSUMER_H
#define FUZZTEST_FUZZ_CONSUMER_H

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>

#if defined(LUME_PNG_FUZZ_STANDALONE) && !defined(__OHOS__) && !defined(__ANDROID__)
// On a stock host toolchain securec is not available; fall back to memcpy.
#include <cerrno>
#ifndef EOK
#define EOK 0
#endif
static inline int fuzz_memcpy_s(void* dest, size_t destSize, const void* src, size_t count)
{
    if (dest == nullptr || src == nullptr || count > destSize) {
        return EINVAL;
    }
    std::memcpy(dest, src, count);
    return EOK;
}
#define FUZZ_MEMCPY_S(d, ds, s, c) fuzz_memcpy_s((d), (ds), (s), (c))
#else
#include <securec.h>
#define FUZZ_MEMCPY_S(d, ds, s, c) memcpy_s((d), (ds), (s), (c))
#endif

class FuzzConsumer {
public:
    FuzzConsumer(const uint8_t* data, size_t size) : data_(data), size_(size), pos_(0)
    {}

    template<typename T>
    bool Consume(T& out)
    {
        static_assert(
            std::is_trivially_copyable<T>::value, "FuzzConsumer::Consume<T> requires T to be trivially copyable");
        if (pos_ + sizeof(T) > size_) {
            return false;
        }
        if (FUZZ_MEMCPY_S(&out, sizeof(T), data_ + pos_, sizeof(T)) != EOK) {
            return false;
        }
        pos_ += sizeof(T);
        return true;
    }

    bool ConsumeBytes(uint8_t* dst, size_t dstCapacity, size_t count)
    {
        if (dst == nullptr || count == 0) {
            return false;
        }
        if (count > dstCapacity) {
            return false;
        }
        if (pos_ + count > size_) {
            return false;
        }
        if (FUZZ_MEMCPY_S(dst, dstCapacity, data_ + pos_, count) != EOK) {
            return false;
        }
        pos_ += count;
        return true;
    }

    size_t Remaining() const
    {
        return (pos_ < size_) ? (size_ - pos_) : 0;
    }

    const uint8_t* Tail() const
    {
        return (pos_ < size_) ? (data_ + pos_) : nullptr;
    }

private:
    const uint8_t* data_;
    size_t size_;
    size_t pos_;
};

#endif  // FUZZTEST_FUZZ_CONSUMER_H
