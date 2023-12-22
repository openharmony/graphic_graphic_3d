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

#ifndef OHOS_RENDER_3D_SHADER_INPUT_BUFFER_H
#define OHOS_RENDER_3D_SHADER_INPUT_BUFFER_H

#include <string>
#include <vector>

#include "data_type/constants.h"

namespace OHOS::Render3D {
struct ShaderInputBuffer {
public:
    ShaderInputBuffer() = default;
    ~ShaderInputBuffer();
    const float* Map(uint32_t floatSize) const; // floatSize are for sanity check if exceed the interal buffer size
    bool Alloc(uint32_t floatSize); // do alloc if size are different, or do nothing
    void Delete();
    bool IsValid() const;
    void Update(float *buffer, uint32_t floatSize);
    void Update(float value, uint32_t index);
    uint32_t FloatSize() const;
    uint32_t ByteSize() const;

private:
    float *buffer_ = nullptr;
    uint32_t floatSize_ = 0;
    static constexpr int max_ = 0x100000;
    static constexpr uint32_t floatToByte_ = sizeof(float) / sizeof(uint8_t);
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_SHADER_INPUT_BUFFER_H
