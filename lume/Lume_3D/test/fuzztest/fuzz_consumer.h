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
#include <securec.h>
#include <string>
#include <type_traits>

#include <base/containers/string_view.h>
#include <base/containers/vector.h>

// Helper to consume typed data from fuzz input.
class FuzzConsumer {
public:
    FuzzConsumer(const uint8_t* data, size_t size) : data_(data), size_(size), pos_(0)
    {}

    // Consume a trivially-copyable type from fuzz data.
    // Non-trivially-copyable types (e.g. with vtable, internal pointers) are rejected at compile time.
    template <typename T>
    bool Consume(T& out)
    {
        static_assert(
            std::is_trivially_copyable<T>::value, "FuzzConsumer::Consume<T> requires T to be trivially copyable");
        if (pos_ + sizeof(T) > size_) {
            return false;
        }
        if (memcpy_s(&out, sizeof(T), data_ + pos_, sizeof(T)) != EOK) {
            return false;
        }
        pos_ += sizeof(T);
        return true;
    }

    // Consume a length-prefixed string from fuzz data.
    bool ConsumeString(std::string& out, size_t maxLen = 256)
    {
        uint16_t len = 0;
        if (!Consume(len)) {
            return false;
        }
        if (maxLen < UINT16_MAX) {
            len = static_cast<uint16_t>(len % (maxLen + 1));
        }
        if (pos_ + len > size_) {
            return false;
        }
        out.assign(reinterpret_cast<const char*>(data_ + pos_), len);
        pos_ += len;
        return true;
    }

    // Consume raw bytes into a vector.
    bool ConsumeBytes(BASE_NS::vector<uint8_t>& out, size_t count)
    {
        if (pos_ + count > size_) {
            return false;
        }
        out.resize(count);
        if (memcpy_s(out.data(), count, data_ + pos_, count) != EOK) {
            return false;
        }
        pos_ += count;
        return true;
    }

    // Consume raw bytes into a raw pointer.
    // dstCapacity: size of the buffer pointed to by dst (in bytes).
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
        if (memcpy_s(dst, dstCapacity, data_ + pos_, count) != EOK) {
            return false;
        }
        pos_ += count;
        return true;
    }

    size_t Remaining() const
    {
        return (pos_ < size_) ? (size_ - pos_) : 0;
    }

private:
    const uint8_t* data_;
    size_t size_;
    size_t pos_;
};

#endif  // FUZZTEST_FUZZ_CONSUMER_H
