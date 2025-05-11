/**
 *  Copyright (C) 2021 Huawei Technologies Co, Ltd.
 */
#ifndef META_SRC_RESOURCE_MEMFILE_HEADER
#define META_SRC_RESOURCE_MEMFILE_HEADER

#include <algorithm>
#include <cstring>

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
            std::memcpy(buffer, &data_[pos_], read);
            pos_ += read;
        }
        return read;
    }
    uint64_t Write(const void* buffer, uint64_t count) override
    {
        if (data_.size() < pos_ + count) {
            data_.resize(pos_ + count);
        }
        std::memcpy(&data_[pos_], buffer, count);
        pos_ += count;
        return count;
    }
    uint64_t Append(const void* buffer, uint64_t count, uint64_t flushSize) override
    {
        if (data_.size() < pos_ + count) {
            data_.resize(pos_ + count);
        }
        std::memcpy(&data_[pos_], buffer, count);
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
