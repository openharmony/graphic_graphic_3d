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
#ifndef META_SRC_RESOURCE_MEMFILE_HEADER
#define META_SRC_RESOURCE_MEMFILE_HEADER

#include <algorithm>
#include <cstring>
#include <securec.h>

#include <base/containers/vector.h>
#include <core/io/intf_file.h>
#include <core/log.h>

META_BEGIN_NAMESPACE()

struct MemFile : public CORE_NS::IFile {
    explicit MemFile(BASE_NS::vector<uint8_t> vec = {}) : data_(BASE_NS::move(vec)) {}

    Mode GetMode() const override
    {
        return Mode::READ_WRITE;
    }
    void Close() override
    {
        data_.clear();
    }
    uint64_t Read(void* buffer, uint64_t count) override
    {
        size_t read = std::min<size_t>(count, data_.size() - pos_);
        if (read) {
            memcpy_s(buffer, read, &data_[pos_], read);
            pos_ += read;
        }
        return read;
    }
    uint64_t Write(const void* buffer, uint64_t count) override
    {
        if (data_.size() < pos_ + count) {
            data_.resize(pos_ + count);
        }
        memcpy_s(&data_[pos_], count, buffer, count);
        pos_ += count;
        return count;
    }
    uint64_t Append(const void* buffer, uint64_t count, uint64_t flushSize) override
    {
        if (data_.size() < pos_ + count) {
            data_.resize(pos_ + count);
        }
        memcpy_s(&data_[pos_], count, buffer, count);
        pos_ += count;
        return count;
    }
    uint64_t GetLength() const override
    {
        return data_.size();
    }
    bool Seek(uint64_t offset) override
    {
        bool ret = offset < data_.size();
        if (ret) {
            pos_ = offset;
        }
        return ret;
    }
    uint64_t GetPosition() const override
    {
        return pos_;
    }
    BASE_NS::vector<uint8_t> Data() const
    {
        return data_;
    }
    const uint8_t* RawData() const
    {
        return data_.data();
    }

private:
    void Destroy() override
    {
        delete this;
    }

    BASE_NS::vector<uint8_t> data_;
    size_t pos_ {};
};

META_END_NAMESPACE()

#endif
