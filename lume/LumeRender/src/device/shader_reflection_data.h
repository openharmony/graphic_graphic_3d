/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef DEVICE_SHADER_REFLECTION_DATA_H
#define DEVICE_SHADER_REFLECTION_DATA_H

#include <base/containers/array_view.h>
#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
enum class ImageDimension : uint8_t {
    DIMENSION_1D = 0,
    DIMENSION_2D = 1,
    DIMENSION_3D = 2,
    DIMENSION_CUBE = 3,
    // ignored in higher level, rectangle texture
    DIMENSION_RECT = 4,
    // image load / store
    DIMENSION_BUFFER = 5,
    DIMENSION_SUBPASS = 6,
};

enum ImageFlags {
    IMAGE_DEPTH = 0b00000001,
    IMAGE_ARRAY = 0b00000010,
    IMAGE_MULTISAMPLE = 0b00000100,
    IMAGE_SAMPLED = 0b00001000,
    IMAGE_LOAD_STORE = 0b00010000,
};

class ShaderReflectionData {
public:
    ShaderReflectionData() = default;
    explicit ShaderReflectionData(BASE_NS::array_view<const uint8_t> reflectionData);

    bool IsValid() const;
    ShaderStageFlags GetStageFlags() const;
    PipelineLayout GetPipelineLayout() const;
    BASE_NS::vector<ShaderSpecialization::Constant> GetSpecializationConstants() const;
    BASE_NS::vector<VertexInputDeclaration::VertexInputAttributeDescription> GetInputDescriptions() const;
    BASE_NS::Math::UVec3 GetLocalSize() const;
    const uint8_t* GetPushConstants() const;

private:
    BASE_NS::array_view<const uint8_t> reflectionData_;
    uint16_t size_[5U] {};
};
RENDER_END_NAMESPACE()
#endif
