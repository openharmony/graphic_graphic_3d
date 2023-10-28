/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */
#include <custom/shader_input_buffer.h>

#include <securec.h>

#include <3d_widget_adapter_log.h>

namespace OHOS::Render3D {
bool ShaderInputBuffer::Alloc(uint32_t floatSize)
{
    if (floatSize > max_) {
        WIDGET_LOGE("%s alloc too much memory", __func__);
        return false;
    }

    if (IsValid() && (floatSize_ == floatSize)) {
        return true;
    }

    Delete();

    buffer_ = new float[floatSize];
    floatSize_ = floatSize;
    return true;
}

const float* ShaderInputBuffer::Map(uint32_t floatSize) const
{
    if (!IsValid()) {
        return nullptr;
    }

    if (floatSize > floatSize_) {
        return nullptr;
    }

    return buffer_;
}

uint32_t ShaderInputBuffer::FloatSize() const
{
    return floatSize_;
}

uint32_t ShaderInputBuffer::ByteSize() const
{
    return floatSize_ * floatToByte_;
}

void ShaderInputBuffer::Delete()
{
    if (IsValid()) {
        delete[] buffer_;
    }
    buffer_ = nullptr;
    floatSize_ = 0u;
}

void ShaderInputBuffer::Update(float *buffer, uint32_t floatSize)
{
    if (!Map(floatSize)) {
        WIDGET_LOGE("Update(buffer, size) map error!");
        return;
    }

    auto ret = memcpy_s(reinterpret_cast<void *>(buffer_), floatSize,
        reinterpret_cast<void *>(buffer), floatSize_);
    if (ret != EOK) {
        WIDGET_LOGE("ShaderInputBuffer Update memory copy error");
    }
}

void ShaderInputBuffer::Update(float value, uint32_t index)
{
    if (!IsValid()) {
        WIDGET_LOGE("shader input buffer invalid");
    }

    if (index >= floatSize_) {
        WIDGET_LOGE("shader input buffer update index exceed the max size");
        return;
    }

    buffer_[index] = value;
}


bool ShaderInputBuffer::IsValid() const
{
    return buffer_ && floatSize_;
}

ShaderInputBuffer::~ShaderInputBuffer()
{
    Delete();
}
} // namespace OHOS::Render3D
