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

#include "render_post_process_blur.h"

#include <core/property/property_types.h>
#include <core/property_tools/property_api_impl.inl>

#include "util/log.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(BlurConfiguration);
DECLARE_PROPERTY_TYPE(BlurConfiguration::BlurQualityType);
DECLARE_PROPERTY_TYPE(BlurConfiguration::BlurType);

ENUM_TYPE_METADATA(BlurConfiguration::BlurQualityType, ENUM_VALUE(QUALITY_TYPE_LOW, "Low Quality"),
    ENUM_VALUE(QUALITY_TYPE_NORMAL, "Normal Quality"), ENUM_VALUE(QUALITY_TYPE_HIGH, "High Quality"))
ENUM_TYPE_METADATA(BlurConfiguration::BlurType, ENUM_VALUE(TYPE_NORMAL, "Normal"),
    ENUM_VALUE(TYPE_HORIZONTAL, "Horizontal"), ENUM_VALUE(TYPE_VERTICAL, "Vertical"))

DECLARE_PROPERTY_TYPE(RenderPostProcessBlurNode::BlurShaderType);
ENUM_TYPE_METADATA(RenderPostProcessBlurNode::BlurShaderType, ENUM_VALUE(BLUR_SHADER_TYPE_RGBA, "RGBA"),
    ENUM_VALUE(BLUR_SHADER_TYPE_R, "Red"), ENUM_VALUE(BLUR_SHADER_TYPE_RG, "RG"),
    ENUM_VALUE(BLUR_SHADER_TYPE_RGB, "RGB"), ENUM_VALUE(BLUR_SHADER_TYPE_A, "Alpha"),
    ENUM_VALUE(BLUR_SHADER_TYPE_SOFT_DOWNSCALE_RGB, "Downscale RGB"),
    ENUM_VALUE(BLUR_SHADER_TYPE_DOWNSCALE_RGBA, "Downscale RGBA"),
    ENUM_VALUE(BLUR_SHADER_TYPE_DOWNSCALE_RGBA_ALPHA_WEIGHT, "Downscale RGBA Alpha Weight"),
    ENUM_VALUE(BLUR_SHADER_TYPE_RGBA_ALPHA_WEIGHT, "RGBA Alpha Weight"))

DATA_TYPE_METADATA(BlurConfiguration, MEMBER_PROPERTY(blurType, "Type", 0),
    MEMBER_PROPERTY(blurQualityType, "Quality Type", 0), MEMBER_PROPERTY(filterSize, "Filter Size", 0),
    MEMBER_PROPERTY(maxMipLevel, "Max Mip Level", 0))

DATA_TYPE_METADATA(RenderPostProcessBlurNode::EffectProperties, MEMBER_PROPERTY(enabled, "Enabled", 0),
    MEMBER_PROPERTY(blurConfiguration, "Blur Configuration", 0), MEMBER_PROPERTY(blurShaderType, "Blur Shader Type", 0),
    MEMBER_PROPERTY(blurShaderScaleType, "Blur Shader Type", 0))
CORE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr size_t PROPERTY_BYTE_SIZE { sizeof(RenderPostProcessBlurNode::EffectProperties) };
}

RenderPostProcessBlur::RenderPostProcessBlur()
    : properties_(
          &propertiesData, array_view(PropertyType::DataType<RenderPostProcessBlurNode::EffectProperties>::properties))
{}

CORE_NS::IPropertyHandle* RenderPostProcessBlur::GetProperties()
{
    return properties_.GetData();
}

void RenderPostProcessBlur::SetData(BASE_NS::array_view<const uint8_t> data)
{
    if (data.size_bytes() == PROPERTY_BYTE_SIZE) {
        BASE_NS::CloneData(&propertiesData, PROPERTY_BYTE_SIZE, data.data(), data.size_bytes());
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    if (data.size_bytes() != PROPERTY_BYTE_SIZE) {
        PLUGIN_LOG_ONCE_E("RenderPostProcessBlur_Size_Mismatch",
            "RENDER_VALIDATION_ENABLED: RenderPostProcessBlur::SetData(), size missmatch (ignored)");
    }
#endif
}

BASE_NS::array_view<const uint8_t> RenderPostProcessBlur::GetData() const
{
    return { reinterpret_cast<const uint8_t*>(&propertiesData), PROPERTY_BYTE_SIZE };
}
RENDER_END_NAMESPACE()
