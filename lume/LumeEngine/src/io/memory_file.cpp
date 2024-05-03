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

#include "io/memory_file.h"

#include <cstdint>
#include <cstring>
#include <memory>

#include <base/containers/allocator.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/io/intf_file.h>
#include <core/log.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::CloneData;

uint64_t MemoryFileStorage::Write(uint64_t index, const void* buffer, uint64_t count)
{
    if (index >= buffer_.size()) {
        return 0;
    }
    if (CloneData(buffer_.data() + index, buffer_.size() - index, buffer, static_cast<size_t>(count))) {
        return count;
    }
    return 0;
}

MemoryFile::MemoryFile(std::shared_ptr<MemoryFileStorage>&& buffer) : buffer_(move(buffer)) {}

IFile::Mode MemoryFile::GetMode() const
{
    return IFile::Mode::READ_ONLY;
}

void MemoryFile::Close() {}

uint64_t MemoryFile::Read(void* buffer, uint64_t count)
{
    uint64_t toRead = count;
    if ((index_ + toRead) > buffer_->GetStorage().size()) {
        toRead = buffer_->GetStorage().size() - index_;
    }

    if (toRead > 0) {
        if (toRead <= SIZE_MAX) {
            if (CloneData(buffer, static_cast<size_t>(count), &(buffer_->GetStorage().data()[index_]),
                    static_cast<size_t>(toRead))) {
                index_ += toRead;
            }
        } else {
            CORE_ASSERT_MSG(false, "Unable to read chunks bigger than (SIZE_MAX) bytes.");
            toRead = 0;
        }
    }

    return toRead;
}

uint64_t MemoryFile::Write(const void* buffer, uint64_t count)
{
    buffer_->Resize(size_t(count));
    return buffer_->Write(0, buffer, count);
}

uint64_t MemoryFile::GetLength() const
{
    return buffer_->GetStorage().size();
}

bool MemoryFile::Seek(uint64_t aOffset)
{
    if (aOffset < buffer_->GetStorage().size()) {
        index_ = aOffset;
        return true;
    }

    return false;
}

uint64_t MemoryFile::GetPosition() const
{
    return index_;
}
CORE_END_NAMESPACE()
