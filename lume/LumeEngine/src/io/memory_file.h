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

#ifndef CORE__IO__MEMORY_FILE_H
#define CORE__IO__MEMORY_FILE_H

#include <cstddef>
#include <cstdint>
#include <memory>

#include <base/containers/type_traits.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/io/intf_file.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
using ByteBuffer = BASE_NS::vector<uint8_t>;

class MemoryFileStorage {
public:
    MemoryFileStorage() = default;
    ~MemoryFileStorage() = default;
    explicit MemoryFileStorage(ByteBuffer&& buffer) : buffer_(BASE_NS::move(buffer)) {}

    const ByteBuffer& GetStorage() const
    {
        return buffer_;
    }

    uint64_t Size() const
    {
        return static_cast<uint64_t>(buffer_.size());
    }

    void Resize(size_t newSize)
    {
        buffer_.resize(newSize);
    }

    uint64_t Write(uint64_t index, const void* buffer, uint64_t count);

private:
    ByteBuffer buffer_;
};

/** Read-only memory file. */
class MemoryFile final : public IFile {
public:
    explicit MemoryFile(std::shared_ptr<MemoryFileStorage>&& buffer);

    ~MemoryFile() override = default;

    Mode GetMode() const override;

    void Close() override;

    uint64_t Read(void* buffer, uint64_t count) override;

    uint64_t Write(const void* buffer, uint64_t count) override;

    uint64_t GetLength() const override;

    bool Seek(uint64_t offset) override;

    uint64_t GetPosition() const override;

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    uint64_t index_ { 0 };
    std::shared_ptr<MemoryFileStorage> buffer_;
};
CORE_END_NAMESPACE()

#endif // CORE__IO__MEMORY_FILE_H
