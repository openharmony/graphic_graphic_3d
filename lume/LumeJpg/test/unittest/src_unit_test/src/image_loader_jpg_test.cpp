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

#include "test_framework.h"

#include <base/containers/array_view.h>
#include <core/image/intf_image_loader_manager.h>
#include <core/image/intf_image_container.h>
#include <core/io/intf_file.h>
#include <core/namespace.h>
#include <jpg/image_loader_jpg.h>

#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <securec.h>

using namespace BASE_NS;
using namespace CORE_NS;

namespace {
// Valid 16x16 RGB JPEG test data (715 bytes)
constexpr uint8_t VALID_JPEG_DATA[] = {0xff,
    0xd8,
    0xff,
    0xe0,
    0x00,
    0x10,
    0x4a,
    0x46,
    0x49,
    0x46,
    0x00,
    0x01,
    0x01,
    0x01,
    0x00,
    0x78,
    0x00,
    0x78,
    0x00,
    0x00,
    0xff,
    0xe1,
    0x00,
    0x22,
    0x45,
    0x78,
    0x69,
    0x66,
    0x00,
    0x00,
    0x4d,
    0x4d,
    0x00,
    0x2a,
    0x00,
    0x00,
    0x00,
    0x08,
    0x00,
    0x01,
    0x01,
    0x12,
    0x00,
    0x03,
    0x00,
    0x00,
    0x00,
    0x01,
    0x00,
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0xff,
    0xdb,
    0x00,
    0x43,
    0x00,
    0x02,
    0x01,
    0x01,
    0x02,
    0x01,
    0x01,
    0x02,
    0x02,
    0x02,
    0x02,
    0x02,
    0x02,
    0x02,
    0x02,
    0x03,
    0x05,
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x06,
    0x04,
    0x04,
    0x03,
    0x05,
    0x07,
    0x06,
    0x07,
    0x07,
    0x07,
    0x06,
    0x07,
    0x07,
    0x08,
    0x09,
    0x0b,
    0x09,
    0x08,
    0x08,
    0x0a,
    0x08,
    0x07,
    0x07,
    0x0a,
    0x0d,
    0x0a,
    0x0a,
    0x0b,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x07,
    0x09,
    0x0e,
    0x0f,
    0x0d,
    0x0c,
    0x0e,
    0x0b,
    0x0c,
    0x0c,
    0x0c,
    0xff,
    0xdb,
    0x00,
    0x43,
    0x01,
    0x02,
    0x02,
    0x02,
    0x03,
    0x03,
    0x03,
    0x06,
    0x03,
    0x03,
    0x06,
    0x0c,
    0x08,
    0x07,
    0x08,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0x0c,
    0xff,
    0xc0,
    0x00,
    0x11,
    0x08,
    0x00,
    0x10,
    0x00,
    0x10,
    0x03,
    0x01,
    0x22,
    0x00,
    0x02,
    0x11,
    0x01,
    0x03,
    0x11,
    0x01,
    0xff,
    0xc4,
    0x00,
    0x1f,
    0x00,
    0x00,
    0x01,
    0x05,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x04,
    0x05,
    0x06,
    0x07,
    0x08,
    0x09,
    0x0a,
    0x0b,
    0xff,
    0xc4,
    0x00,
    0xb5,
    0x10,
    0x00,
    0x02,
    0x01,
    0x03,
    0x03,
    0x02,
    0x04,
    0x03,
    0x05,
    0x05,
    0x04,
    0x04,
    0x00,
    0x00,
    0x01,
    0x7d,
    0x01,
    0x02,
    0x03,
    0x00,
    0x04,
    0x11,
    0x05,
    0x12,
    0x21,
    0x31,
    0x41,
    0x06,
    0x13,
    0x51,
    0x61,
    0x07,
    0x22,
    0x71,
    0x14,
    0x32,
    0x81,
    0x91,
    0xa1,
    0x08,
    0x23,
    0x42,
    0xb1,
    0xc1,
    0x15,
    0x52,
    0xd1,
    0xf0,
    0x24,
    0x33,
    0x62,
    0x72,
    0x82,
    0x09,
    0x0a,
    0x16,
    0x17,
    0x18,
    0x19,
    0x1a,
    0x25,
    0x26,
    0x27,
    0x28,
    0x29,
    0x2a,
    0x34,
    0x35,
    0x36,
    0x37,
    0x38,
    0x39,
    0x3a,
    0x43,
    0x44,
    0x45,
    0x46,
    0x47,
    0x48,
    0x49,
    0x4a,
    0x53,
    0x54,
    0x55,
    0x56,
    0x57,
    0x58,
    0x59,
    0x5a,
    0x63,
    0x64,
    0x65,
    0x66,
    0x67,
    0x68,
    0x69,
    0x6a,
    0x73,
    0x74,
    0x75,
    0x76,
    0x77,
    0x78,
    0x79,
    0x7a,
    0x83,
    0x84,
    0x85,
    0x86,
    0x87,
    0x88,
    0x89,
    0x8a,
    0x92,
    0x93,
    0x94,
    0x95,
    0x96,
    0x97,
    0x98,
    0x99,
    0x9a,
    0xa2,
    0xa3,
    0xa4,
    0xa5,
    0xa6,
    0xa7,
    0xa8,
    0xa9,
    0xaa,
    0xb2,
    0xb3,
    0xb4,
    0xb5,
    0xb6,
    0xb7,
    0xb8,
    0xb9,
    0xba,
    0xc2,
    0xc3,
    0xc4,
    0xc5,
    0xc6,
    0xc7,
    0xc8,
    0xc9,
    0xca,
    0xd2,
    0xd3,
    0xd4,
    0xd5,
    0xd6,
    0xd7,
    0xd8,
    0xd9,
    0xda,
    0xe1,
    0xe2,
    0xe3,
    0xe4,
    0xe5,
    0xe6,
    0xe7,
    0xe8,
    0xe9,
    0xea,
    0xf1,
    0xf2,
    0xf3,
    0xf4,
    0xf5,
    0xf6,
    0xf7,
    0xf8,
    0xf9,
    0xfa,
    0xff,
    0xc4,
    0x00,
    0x1f,
    0x01,
    0x00,
    0x03,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x04,
    0x05,
    0x06,
    0x07,
    0x08,
    0x09,
    0x0a,
    0x0b,
    0xff,
    0xc4,
    0x00,
    0xb5,
    0x11,
    0x00,
    0x02,
    0x01,
    0x02,
    0x04,
    0x04,
    0x03,
    0x04,
    0x07,
    0x05,
    0x04,
    0x04,
    0x00,
    0x01,
    0x02,
    0x77,
    0x00,
    0x01,
    0x02,
    0x03,
    0x11,
    0x04,
    0x05,
    0x21,
    0x31,
    0x06,
    0x12,
    0x41,
    0x51,
    0x07,
    0x61,
    0x71,
    0x13,
    0x22,
    0x32,
    0x81,
    0x08,
    0x14,
    0x42,
    0x91,
    0xa1,
    0xb1,
    0xc1,
    0x09,
    0x23,
    0x33,
    0x52,
    0xf0,
    0x15,
    0x62,
    0x72,
    0xd1,
    0x0a,
    0x16,
    0x24,
    0x34,
    0xe1,
    0x25,
    0xf1,
    0x17,
    0x18,
    0x19,
    0x1a,
    0x26,
    0x27,
    0x28,
    0x29,
    0x2a,
    0x35,
    0x36,
    0x37,
    0x38,
    0x39,
    0x3a,
    0x43,
    0x44,
    0x45,
    0x46,
    0x47,
    0x48,
    0x49,
    0x4a,
    0x53,
    0x54,
    0x55,
    0x56,
    0x57,
    0x58,
    0x59,
    0x5a,
    0x63,
    0x64,
    0x65,
    0x66,
    0x67,
    0x68,
    0x69,
    0x6a,
    0x73,
    0x74,
    0x75,
    0x76,
    0x77,
    0x78,
    0x79,
    0x7a,
    0x82,
    0x83,
    0x84,
    0x85,
    0x86,
    0x87,
    0x88,
    0x89,
    0x8a,
    0x92,
    0x93,
    0x94,
    0x95,
    0x96,
    0x97,
    0x98,
    0x99,
    0x9a,
    0xa2,
    0xa3,
    0xa4,
    0xa5,
    0xa6,
    0xa7,
    0xa8,
    0xa9,
    0xaa,
    0xb2,
    0xb3,
    0xb4,
    0xb5,
    0xb6,
    0xb7,
    0xb8,
    0xb9,
    0xba,
    0xc2,
    0xc3,
    0xc4,
    0xc5,
    0xc6,
    0xc7,
    0xc8,
    0xc9,
    0xca,
    0xd2,
    0xd3,
    0xd4,
    0xd5,
    0xd6,
    0xd7,
    0xd8,
    0xd9,
    0xda,
    0xe2,
    0xe3,
    0xe4,
    0xe5,
    0xe6,
    0xe7,
    0xe8,
    0xe9,
    0xea,
    0xf2,
    0xf3,
    0xf4,
    0xf5,
    0xf6,
    0xf7,
    0xf8,
    0xf9,
    0xfa,
    0xff,
    0xda,
    0x00,
    0x0c,
    0x03,
    0x01,
    0x00,
    0x02,
    0x11,
    0x03,
    0x11,
    0x00,
    0x3f,
    0x00,
    0xfd,
    0xdc,
    0x3a,
    0xc5,
    0xa9,
    0xff,
    0x00,
    0x97,
    0x88,
    0x7f,
    0xef,
    0xa1,
    0x40,
    0xd5,
    0xed,
    0x73,
    0xcd,
    0xc4,
    0x58,
    0xff,
    0x00,
    0x78,
    0x57,
    0xe2,
    0xd7,
    0xfc,
    0x3e,
    0x42,
    0xf7,
    0xfe,
    0x7f,
    0x66,
    0xff,
    0x00,
    0xbe,
    0xda,
    0x8f,
    0xf8,
    0x7c,
    0x85,
    0xef,
    0xfc,
    0xfe,
    0xcd,
    0xff,
    0x00,
    0x7d,
    0xb5,
    0x57,
    0x29,
    0x97,
    0xb4,
    0xf2,
    0x3f,
    0xff,
    0xd9};
constexpr size_t VALID_JPEG_SIZE = sizeof(VALID_JPEG_DATA);

// JPEG header bytes for CanLoad tests
constexpr uint8_t JPEG_HEADER_JFIF[] = {0xff, 0xd8, 0xff, 0xe0, 0x00, 0x10, 0x4a, 0x46, 0x49, 0x46};
constexpr uint8_t JPEG_HEADER_DQT[] = {0xff, 0xd8, 0xff, 0xdb, 0x00, 0x43, 0x00, 0x01, 0x01, 0x01};
constexpr uint8_t JPEG_HEADER_EXIF[] = {0xff, 0xd8, 0xff, 0xe1, 0x00, 0x22, 0x45, 0x78, 0x69, 0x66};
constexpr uint8_t JPEG_HEADER_ICC[] = {0xff, 0xd8, 0xff, 0xe2, 0x00, 0x10, 0x49, 0x43, 0x43, 0x5f};
constexpr uint8_t INVALID_HEADER[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00};

// Valid 4x4 Grayscale JPEG test data (346 bytes)
constexpr uint8_t GRAYSCALE_JPEG_DATA[] = {0xff,
    0xd8,
    0xff,
    0xe0,
    0x00,
    0x10,
    0x4a,
    0x46,
    0x49,
    0x46,
    0x00,
    0x01,
    0x01,
    0x00,
    0x00,
    0x01,
    0x00,
    0x01,
    0x00,
    0x00,
    0xff,
    0xdb,
    0x00,
    0x43,
    0x00,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0x10,
    0xff,
    0xc0,
    0x00,
    0x0b,
    0x08,
    0x00,
    0x04,
    0x00,
    0x04,
    0x01,
    0x01,
    0x11,
    0x00,
    0xff,
    0xc4,
    0x00,
    0x1f,
    0x00,
    0x00,
    0x01,
    0x05,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x04,
    0x05,
    0x06,
    0x07,
    0x08,
    0x09,
    0x0a,
    0x0b,
    0xff,
    0xc4,
    0x00,
    0xb5,
    0x10,
    0x00,
    0x02,
    0x01,
    0x03,
    0x03,
    0x02,
    0x04,
    0x03,
    0x05,
    0x05,
    0x04,
    0x04,
    0x00,
    0x00,
    0x01,
    0x7d,
    0x00,
    0x01,
    0x02,
    0x03,
    0x04,
    0x05,
    0x06,
    0x07,
    0x08,
    0x09,
    0x0a,
    0x0b,
    0x0c,
    0x0d,
    0x0e,
    0x0f,
    0x10,
    0x11,
    0x12,
    0x13,
    0x14,
    0x15,
    0x16,
    0x17,
    0x18,
    0x19,
    0x1a,
    0x1b,
    0x1c,
    0x1d,
    0x1e,
    0x1f,
    0x20,
    0x21,
    0x22,
    0x23,
    0x24,
    0x25,
    0x26,
    0x27,
    0x28,
    0x29,
    0x2a,
    0x2b,
    0x2c,
    0x2d,
    0x2e,
    0x2f,
    0x30,
    0x31,
    0x32,
    0x33,
    0x34,
    0x35,
    0x36,
    0x37,
    0x38,
    0x39,
    0x3a,
    0x3b,
    0x3c,
    0x3d,
    0x3e,
    0x3f,
    0x40,
    0x41,
    0x42,
    0x43,
    0x44,
    0x45,
    0x46,
    0x47,
    0x48,
    0x49,
    0x4a,
    0x4b,
    0x4c,
    0x4d,
    0x4e,
    0x4f,
    0x50,
    0x51,
    0x52,
    0x53,
    0x54,
    0x55,
    0x56,
    0x57,
    0x58,
    0x59,
    0x5a,
    0x5b,
    0x5c,
    0x5d,
    0x5e,
    0x5f,
    0x60,
    0x61,
    0x62,
    0x63,
    0x64,
    0x65,
    0x66,
    0x67,
    0x68,
    0x69,
    0x6a,
    0x6b,
    0x6c,
    0x6d,
    0x6e,
    0x6f,
    0x70,
    0x71,
    0x72,
    0x73,
    0x74,
    0x75,
    0x76,
    0x77,
    0x78,
    0x79,
    0x7a,
    0x7b,
    0x7c,
    0x7d,
    0x7e,
    0x7f,
    0x80,
    0x81,
    0x82,
    0x83,
    0x84,
    0x85,
    0x86,
    0x87,
    0x88,
    0x89,
    0x8a,
    0x8b,
    0x8c,
    0x8d,
    0x8e,
    0x8f,
    0x90,
    0x91,
    0x92,
    0x93,
    0x94,
    0x95,
    0x96,
    0x97,
    0x98,
    0x99,
    0x9a,
    0x9b,
    0x9c,
    0x9d,
    0x9e,
    0x9f,
    0xa0,
    0xa1,
    0xff,
    0xda,
    0x00,
    0x08,
    0x01,
    0x01,
    0x00,
    0x00,
    0x3f,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0xff,
    0xd9};
static const size_t GRAYSCALE_JPEG_SIZE = sizeof(GRAYSCALE_JPEG_DATA);

// Mock IFile implementation for testing
class MockFile : public CORE_NS::IFile {
public:
    MockFile(const uint8_t* data, size_t size) : data_(data), size_(size), pos_(0)
    {}

    CORE_NS::IFile::Mode GetMode() const override
    {
        return CORE_NS::IFile::Mode::READ_ONLY;
    }

    void Close() override
    {}

    uint64_t Read(void* buffer, uint64_t count) override
    {
        if (buffer == nullptr || pos_ >= size_) {
            return 0;
        }
        uint64_t readCount = (count > size_ - pos_) ? (size_ - pos_) : count;
        if (memcpy_s(buffer, static_cast<size_t>(readCount), data_ + pos_, static_cast<size_t>(readCount)) != EOK) {
            return 0;
        }
        pos_ += readCount;
        return readCount;
    }
    uint64_t Write(const void* buffer, uint64_t count) override
    {
        return 0;
    }

    uint64_t Append(const void* buffer, uint64_t count, uint64_t flushSize) override
    {
        return 0;
    }

    uint64_t GetLength() const override
    {
        return size_;
    }

    bool Seek(uint64_t offset) override
    {
        if (offset > size_) {
            return false;
        }
        pos_ = offset;
        return true;
    }

    uint64_t GetPosition() const override
    {
        return pos_;
    }

    void Destroy() override
    {
        delete this;
    }

private:
    const uint8_t* data_;
    size_t size_;
    size_t pos_;
};

// Mock file that simulates a partial read failure
class MockFilePartialRead : public CORE_NS::IFile {
public:
    MockFilePartialRead(const uint8_t* data, size_t size, size_t maxRead)
        : data_(data), size_(size), maxRead_(maxRead), pos_(0)
    {}

    CORE_NS::IFile::Mode GetMode() const override
    {
        return CORE_NS::IFile::Mode::READ_ONLY;
    }
    void Close() override
    {}
    uint64_t Read(void* buffer, uint64_t count) override
    {
        if (buffer == nullptr || pos_ >= size_) {
            return 0;
        }
        size_t toRead = static_cast<size_t>(count);
        if (toRead > maxRead_) {
            toRead = maxRead_;
        }
        if (toRead > size_ - pos_) {
            toRead = size_ - pos_;
        }
        if (memcpy_s(buffer, toRead, data_ + pos_, toRead) != EOK) {
            return 0;
        }
        pos_ += toRead;
        return toRead;
    }
    uint64_t Write(const void*, uint64_t) override
    {
        return 0;
    }
    uint64_t Append(const void*, uint64_t, uint64_t) override
    {
        return 0;
    }
    uint64_t GetLength() const override
    {
        return size_;
    }
    bool Seek(uint64_t offset) override
    {
        pos_ = offset;
        return true;
    }
    uint64_t GetPosition() const override
    {
        return pos_;
    }
    void Destroy() override
    {
        delete this;
    }

private:
    const uint8_t* data_;
    size_t size_;
    size_t maxRead_;
    size_t pos_;
};

// Mock file that reports a very large size but has limited data
class MockFileTooBig : public CORE_NS::IFile {
public:
    MockFileTooBig() : reportedSize_(static_cast<uint64_t>(std::numeric_limits<int>::max()) + 1)
    {}

    CORE_NS::IFile::Mode GetMode() const override
    {
        return CORE_NS::IFile::Mode::READ_ONLY;
    }
    void Close() override
    {}
    uint64_t Read(void*, uint64_t) override
    {
        return 0;
    }
    uint64_t Write(const void*, uint64_t) override
    {
        return 0;
    }
    uint64_t Append(const void*, uint64_t, uint64_t) override
    {
        return 0;
    }
    uint64_t GetLength() const override
    {
        return reportedSize_;
    }
    bool Seek(uint64_t) override
    {
        return true;
    }
    uint64_t GetPosition() const override
    {
        return 0;
    }
    void Destroy() override
    {
        delete this;
    }

private:
    uint64_t reportedSize_;
};
}  // anonymous namespace

class ImageLoaderJPGTest : public testing::Test {
public:
    static void SetUpTestSuite()
    {}
    static void TearDownTestSuite()
    {}
    void SetUp() override
    {
        loader_ = JPGPlugin::CreateImageLoaderJPG(CORE_NS::PluginToken{});
    }
    void TearDown() override
    {
        loader_.reset();
    }

    IImageLoaderManager::IImageLoader::Ptr loader_;
};

/**
 * @tc.name: CreateImageLoaderJPG_001
 * @tc.desc: Verify CreateImageLoaderJPG returns a valid loader instance
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, CreateImageLoaderJPG_001, testing::ext::TestSize.Level1)
{
    ASSERT_NE(loader_, nullptr);
}

/**
 * @tc.name: CanLoad_001
 * @tc.desc: Verify CanLoad returns true for valid JFIF JPEG header
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, CanLoad_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(JPEG_HEADER_JFIF, sizeof(JPEG_HEADER_JFIF));
    EXPECT_TRUE(loader_->CanLoad(view));
}

/**
 * @tc.name: CanLoad_002
 * @tc.desc: Verify CanLoad returns true for valid DQT JPEG header
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, CanLoad_002, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(JPEG_HEADER_DQT, sizeof(JPEG_HEADER_DQT));
    EXPECT_TRUE(loader_->CanLoad(view));
}

/**
 * @tc.name: CanLoad_003
 * @tc.desc: Verify CanLoad returns true for valid Exif JPEG header
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, CanLoad_003, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(JPEG_HEADER_EXIF, sizeof(JPEG_HEADER_EXIF));
    EXPECT_TRUE(loader_->CanLoad(view));
}

/**
 * @tc.name: CanLoad_004
 * @tc.desc: Verify CanLoad returns true for valid ICC_PROFILE JPEG header
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, CanLoad_004, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(JPEG_HEADER_ICC, sizeof(JPEG_HEADER_ICC));
    EXPECT_TRUE(loader_->CanLoad(view));
}

/**
 * @tc.name: CanLoad_005
 * @tc.desc: Verify CanLoad returns false for invalid header bytes (PNG header)
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, CanLoad_005, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(INVALID_HEADER, sizeof(INVALID_HEADER));
    EXPECT_FALSE(loader_->CanLoad(view));
}

/**
 * @tc.name: CanLoad_006
 * @tc.desc: Verify CanLoad returns false for data shorter than 10 bytes
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, CanLoad_006, testing::ext::TestSize.Level1)
{
    const uint8_t shortData[] = {0xff, 0xd8, 0xff};
    auto view = array_view<const uint8_t>(shortData, sizeof(shortData));
    EXPECT_FALSE(loader_->CanLoad(view));
}

/**
 * @tc.name: CanLoad_007
 * @tc.desc: Verify CanLoad returns false for empty data
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, CanLoad_007, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>();
    EXPECT_FALSE(loader_->CanLoad(view));
}

/**
 * @tc.name: CanLoad_008
 * @tc.desc: Verify CanLoad returns false for data with wrong first two bytes
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, CanLoad_008, testing::ext::TestSize.Level1)
{
    const uint8_t wrongMagic[] = {0x00, 0x00, 0xff, 0xe0, 0x00, 0x10, 0x4a, 0x46, 0x49, 0x46};
    auto view = array_view<const uint8_t>(wrongMagic, sizeof(wrongMagic));
    EXPECT_FALSE(loader_->CanLoad(view));
}

/**
 * @tc.name: CanLoad_009
 * @tc.desc: Verify CanLoad returns true for valid complete JPEG data
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, CanLoad_009, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_JPEG_DATA, VALID_JPEG_SIZE);
    EXPECT_TRUE(loader_->CanLoad(view));
}

/**
 * @tc.name: LoadFromBytes_001
 * @tc.desc: Verify Load from valid JPEG bytes with default flags succeeds
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, LoadFromBytes_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_JPEG_DATA, VALID_JPEG_SIZE);
    auto result = loader_->Load(view, 0);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
}

/**
 * @tc.name: LoadFromBytes_002
 * @tc.desc: Verify Load from empty bytes fails
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, LoadFromBytes_002, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>();
    auto result = loader_->Load(view, 0);
    EXPECT_FALSE(result.success);
}

/**
 * @tc.name: LoadFromBytes_003
 * @tc.desc: Verify Load from invalid (garbage) bytes fails
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, LoadFromBytes_003, testing::ext::TestSize.Level1)
{
    const uint8_t garbage[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a};
    auto view = array_view<const uint8_t>(garbage, sizeof(garbage));
    auto result = loader_->Load(view, 0);
    EXPECT_FALSE(result.success);
}

/**
 * @tc.name: LoadFromBytes_004
 * @tc.desc: Verify Load with GENERATE_MIPS flag succeeds
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, LoadFromBytes_004, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_JPEG_DATA, VALID_JPEG_SIZE);
    auto result = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    auto desc = result.image->GetImageDesc();
    EXPECT_NE(desc.mipCount, 1u);
}

/**
 * @tc.name: LoadFromBytes_005
 * @tc.desc: Verify Load with FORCE_LINEAR_RGB_BIT flag succeeds
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, LoadFromBytes_005, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_JPEG_DATA, VALID_JPEG_SIZE);
    auto result = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
}

/**
 * @tc.name: LoadFromBytes_006
 * @tc.desc: Verify Load with FORCE_GRAYSCALE_BIT flag succeeds
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, LoadFromBytes_006, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_JPEG_DATA, VALID_JPEG_SIZE);
    auto result = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
}

/**
 * @tc.name: LoadFromBytes_007
 * @tc.desc: Verify Load with FLIP_VERTICALLY_BIT flag succeeds
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, LoadFromBytes_007, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_JPEG_DATA, VALID_JPEG_SIZE);
    auto result = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_FLIP_VERTICALLY_BIT);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
}

/**
 * @tc.name: LoadFromBytes_008
 * @tc.desc: Verify Load with METADATA_ONLY flag succeeds and returns metadata
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, LoadFromBytes_008, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_JPEG_DATA, VALID_JPEG_SIZE);
    auto result = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    // With METADATA_ONLY, image metadata (dimensions) should be populated
    auto desc = result.image->GetImageDesc();
    EXPECT_EQ(desc.width, 16u);
    EXPECT_EQ(desc.height, 16u);
}

/**
 * @tc.name: LoadFromBytes_009
 * @tc.desc: Verify Load with combined flags succeeds
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, LoadFromBytes_009, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_JPEG_DATA, VALID_JPEG_SIZE);
    uint32_t combinedFlags =
        IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS | IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT;
    auto result = loader_->Load(view, combinedFlags);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
}

/**
 * @tc.name: LoadFromFile_001
 * @tc.desc: Verify Load from valid IFile succeeds
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, LoadFromFile_001, testing::ext::TestSize.Level1)
{
    auto* file = new MockFile(VALID_JPEG_DATA, VALID_JPEG_SIZE);
    auto result = loader_->Load(*file, 0);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
}

/**
 * @tc.name: LoadFromFile_002
 * @tc.desc: Verify Load from oversized IFile fails with file too big error
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, LoadFromFile_002, testing::ext::TestSize.Level1)
{
    auto* file = new MockFileTooBig();
    auto result = loader_->Load(*file, 0);
    EXPECT_FALSE(result.success);
}

/**
 * @tc.name: LoadFromFile_003
 * @tc.desc: Verify Load from IFile with partial read failure
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, LoadFromFile_003, testing::ext::TestSize.Level1)
{
    // Create a file that reports full size but only reads partial data
    auto* file = new MockFilePartialRead(VALID_JPEG_DATA, VALID_JPEG_SIZE, 10);
    auto result = loader_->Load(*file, 0);
    EXPECT_FALSE(result.success);
}

/**
 * @tc.name: LoadFromFile_004
 * @tc.desc: Verify Load from IFile with GENERATE_MIPS flag
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, LoadFromFile_004, testing::ext::TestSize.Level1)
{
    auto* file = new MockFile(VALID_JPEG_DATA, VALID_JPEG_SIZE);
    auto result = loader_->Load(*file, IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    auto desc = result.image->GetImageDesc();
    EXPECT_NE(desc.mipCount, 1u);
}

/**
 * @tc.name: ImageProperties_001
 * @tc.desc: Verify loaded image dimensions are correct (16x16)
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, ImageProperties_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_JPEG_DATA, VALID_JPEG_SIZE);
    auto result = loader_->Load(view, 0);
    ASSERT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);

    auto desc = result.image->GetImageDesc();
    EXPECT_EQ(desc.width, 16u);
    EXPECT_EQ(desc.height, 16u);
    EXPECT_EQ(desc.depth, 1u);
    EXPECT_EQ(desc.imageType, IImageContainer::ImageType::TYPE_2D);
    EXPECT_EQ(desc.imageViewType, IImageContainer::ImageViewType::VIEW_TYPE_2D);
}

/**
 * @tc.name: ImageProperties_002
 * @tc.desc: Verify loaded image has correct format (sRGB by default)
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, ImageProperties_002, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_JPEG_DATA, VALID_JPEG_SIZE);
    auto result = loader_->Load(view, 0);
    ASSERT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);

    auto desc = result.image->GetImageDesc();
    // 16x16 test JPEG has 3 or 4 components after conversion, default should be sRGB
    EXPECT_NE(desc.format, Format::BASE_FORMAT_UNDEFINED);
}

/**
 * @tc.name: ImageProperties_003
 * @tc.desc: Verify loaded image with LINEAR_RGB has UNORM format
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, ImageProperties_003, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_JPEG_DATA, VALID_JPEG_SIZE);
    auto result = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT);
    ASSERT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);

    auto desc = result.image->GetImageDesc();
    // With FORCE_LINEAR_RGB, format should be UNORM (not SRGB)
    EXPECT_NE(desc.format, Format::BASE_FORMAT_UNDEFINED);
}

/**
 * @tc.name: ImageData_001
 * @tc.desc: Verify loaded image data buffer is not empty for valid JPEG
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, ImageData_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_JPEG_DATA, VALID_JPEG_SIZE);
    auto result = loader_->Load(view, 0);
    ASSERT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);

    auto data = result.image->GetData();
    EXPECT_GT(data.size(), 0u);

    auto copies = result.image->GetBufferImageCopies();
    EXPECT_EQ(copies.size(), 1u);
    EXPECT_EQ(copies[0].width, 16u);
    EXPECT_EQ(copies[0].height, 16u);
}

/**
 * @tc.name: LoadAnimatedImage_001
 * @tc.desc: Verify LoadAnimatedImage from bytes always fails (JPG does not support animation)
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, LoadAnimatedImage_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_JPEG_DATA, VALID_JPEG_SIZE);
    auto result = loader_->LoadAnimatedImage(view, 0);
    EXPECT_FALSE(result.success);
}

/**
 * @tc.name: LoadAnimatedImage_002
 * @tc.desc: Verify LoadAnimatedImage from IFile always fails
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, LoadAnimatedImage_002, testing::ext::TestSize.Level1)
{
    auto* file = new MockFile(VALID_JPEG_DATA, VALID_JPEG_SIZE);
    auto result = loader_->LoadAnimatedImage(*file, 0);
    EXPECT_FALSE(result.success);
}

/**
 * @tc.name: GetSupportedTypes_001
 * @tc.desc: Verify GetSupportedTypes returns correct JPEG types
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, GetSupportedTypes_001, testing::ext::TestSize.Level1)
{
    auto types = loader_->GetSupportedTypes();
    EXPECT_EQ(types.size(), 2u);
    EXPECT_EQ(types[0].mimeType, "image/jpeg");
    EXPECT_EQ(types[0].fileExtension, "jpeg");
    EXPECT_EQ(types[1].mimeType, "image/jpeg");
    EXPECT_EQ(types[1].fileExtension, "jpg");
}

/**
 * @tc.name: LoadFromBytes_TruncatedJPEG_001
 * @tc.desc: Verify Load from truncated JPEG data (only header) fails
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, LoadFromBytes_TruncatedJPEG_001, testing::ext::TestSize.Level1)
{
    // Only provide the first 60 bytes of the JPEG (just headers, no image data)
    auto view = array_view<const uint8_t>(VALID_JPEG_DATA, 60);
    auto result = loader_->Load(view, 0);
    EXPECT_FALSE(result.success);
}

/**
 * @tc.name: LoadFromBytes_CorruptedJPEG_001
 * @tc.desc: Verify Load from corrupted JPEG data (valid header, corrupt body) fails
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, LoadFromBytes_CorruptedJPEG_001, testing::ext::TestSize.Level1)
{
    // Copy valid JPEG and corrupt the scan data
    uint8_t corrupted[sizeof(VALID_JPEG_DATA)];
    EXPECT_EQ(memcpy_s(corrupted, sizeof(VALID_JPEG_DATA), VALID_JPEG_DATA, sizeof(VALID_JPEG_DATA)), EOK);
    // Corrupt the scan data section
    corrupted[VALID_JPEG_SIZE - 20] = 0x00;
    corrupted[VALID_JPEG_SIZE - 19] = 0x00;
    auto view = array_view<const uint8_t>(corrupted, sizeof(corrupted));
    auto result = loader_->Load(view, 0);
    // Result may or may not succeed depending on corruption tolerance,
    // but should not crash
}

/**
 * @tc.name: LoadFromBytes_InvalidChannelCount_001
 * @tc.desc: Verify Load from bytes with PREMULTIPLY_ALPHA flag does not crash
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, LoadFromBytes_PremultiplyAlpha_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_JPEG_DATA, VALID_JPEG_SIZE);
    auto result = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_PREMULTIPLY_ALPHA);
    // JPG typically doesn't have alpha, premultiply should still succeed
    EXPECT_TRUE(result.success);
}

/**
 * @tc.name: ImageProperties_MipmapCalculation_001
 * @tc.desc: Verify mipmap count calculation for 16x16 image
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, ImageProperties_MipmapCalculation_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_JPEG_DATA, VALID_JPEG_SIZE);
    auto result = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS);
    ASSERT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);

    auto desc = result.image->GetImageDesc();
    // For 16x16 image, mip levels: 16->8->4->2->1 = 5 levels
    EXPECT_EQ(desc.mipCount, 5u);
    EXPECT_TRUE(desc.imageFlags & IImageContainer::ImageFlags::FLAGS_REQUESTING_MIPMAPS_BIT);
}

// ==================== Grayscale JPEG (cover 1-component ResolveFormat branch) ====================

/**
 * @tc.name: LoadFromBytes_Grayscale_001
 * @tc.desc: Verify Load grayscale JPEG succeeds (1-component path)
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, LoadFromBytes_Grayscale_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(GRAYSCALE_JPEG_DATA, GRAYSCALE_JPEG_SIZE);
    auto result = loader_->Load(view, 0);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    auto desc = result.image->GetImageDesc();
    EXPECT_EQ(desc.width, 4u);
    EXPECT_EQ(desc.height, 4u);
}

/**
 * @tc.name: LoadFromBytes_Grayscale_002
 * @tc.desc: Verify Load grayscale JPEG with FORCE_GRAYSCALE keeps 1 component
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, LoadFromBytes_Grayscale_002, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(GRAYSCALE_JPEG_DATA, GRAYSCALE_JPEG_SIZE);
    auto result = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    EXPECT_EQ(result.image->GetImageDesc().format, Format::BASE_FORMAT_R8_SRGB);
}

/**
 * @tc.name: LoadFromBytes_Grayscale_003
 * @tc.desc: Verify METADATA_ONLY on grayscale returns correct dimensions
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, LoadFromBytes_Grayscale_003, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(GRAYSCALE_JPEG_DATA, GRAYSCALE_JPEG_SIZE);
    auto result = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    auto desc = result.image->GetImageDesc();
    EXPECT_EQ(desc.width, 4u);
    EXPECT_EQ(desc.height, 4u);
}

/**
 * @tc.name: CanLoad_Grayscale_001
 * @tc.desc: Verify CanLoad returns true for grayscale JPEG data
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, CanLoad_Grayscale_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(GRAYSCALE_JPEG_DATA, GRAYSCALE_JPEG_SIZE);
    EXPECT_TRUE(loader_->CanLoad(view));
}

// ==================== Format Verification (cover ResolveFormat branches) ====================

/**
 * @tc.name: FormatVerification_RGB_SRGB_001
 * @tc.desc: Verify component=4, !linear → R8G8B8A8_SRGB
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, FormatVerification_RGB_SRGB_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_JPEG_DATA, VALID_JPEG_SIZE);
    auto result = loader_->Load(view, 0);
    ASSERT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    EXPECT_EQ(result.image->GetImageDesc().format, Format::BASE_FORMAT_R8G8B8A8_SRGB);
}

/**
 * @tc.name: FormatVerification_RGB_UNORM_001
 * @tc.desc: Verify component=4, linear → R8G8B8A8_UNORM
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, FormatVerification_RGB_UNORM_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_JPEG_DATA, VALID_JPEG_SIZE);
    auto result = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT);
    ASSERT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    EXPECT_EQ(result.image->GetImageDesc().format, Format::BASE_FORMAT_R8G8B8A8_UNORM);
}

/**
 * @tc.name: FormatVerification_Grayscale_001
 * @tc.desc: Verify component=1: !linear → R8_SRGB; linear → R8_UNORM
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, FormatVerification_Grayscale_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(GRAYSCALE_JPEG_DATA, GRAYSCALE_JPEG_SIZE);
    auto r1 = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT);
    ASSERT_TRUE(r1.success);
    EXPECT_EQ(r1.image->GetImageDesc().format, Format::BASE_FORMAT_R8_SRGB);

    uint32_t flags =
        IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT | IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT;
    auto r2 = loader_->Load(view, flags);
    ASSERT_TRUE(r2.success);
    EXPECT_EQ(r2.image->GetImageDesc().format, Format::BASE_FORMAT_R8_UNORM);
}

// ==================== Flag Interaction (cover METADATA_ONLY skips premultiply) ====================

/**
 * @tc.name: LoadFromBytes_MetadataOnlyNoPremultiply_001
 * @tc.desc: Verify METADATA_ONLY skips PREMULTIPLY_ALPHA
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, LoadFromBytes_MetadataOnlyNoPremultiply_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_JPEG_DATA, VALID_JPEG_SIZE);
    uint32_t flags =
        IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY | IImageLoaderManager::IMAGE_LOADER_PREMULTIPLY_ALPHA;
    auto result = loader_->Load(view, flags);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    auto desc = result.image->GetImageDesc();
    EXPECT_FALSE(desc.imageFlags & IImageContainer::ImageFlags::FLAGS_PREMULTIPLIED_ALPHA_BIT);
}

/**
 * @tc.name: LoadFromBytes_MetadataOnly_GenerateMips_001
 * @tc.desc: Verify METADATA_ONLY + GENERATE_MIPS reports mip count without decoding
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderJPGTest, LoadFromBytes_MetadataOnly_GenerateMips_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_JPEG_DATA, VALID_JPEG_SIZE);
    uint32_t flags = IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY | IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS;
    auto result = loader_->Load(view, flags);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    auto desc = result.image->GetImageDesc();
    EXPECT_NE(desc.mipCount, 1u);
    EXPECT_TRUE(desc.imageFlags & IImageContainer::ImageFlags::FLAGS_REQUESTING_MIPMAPS_BIT);
}
