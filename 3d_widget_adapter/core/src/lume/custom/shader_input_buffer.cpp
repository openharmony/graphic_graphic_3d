/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
