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

#include "render_post_process_bloom.h"

#include <core/property/property_types.h>
#include <core/property_tools/property_api_impl.inl>

#include "util/log.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(BloomConfiguration);
DECLARE_PROPERTY_TYPE(BloomConfiguration::BloomQualityType);
DECLARE_PROPERTY_TYPE(BloomConfiguration::BloomType);
ENUM_TYPE_METADATA(BloomConfiguration::BloomQualityType, ENUM_VALUE(QUALITY_TYPE_LOW, "Low Quality"),
    ENUM_VALUE(QUALITY_TYPE_NORMAL, "Normal Quality"), ENUM_VALUE(QUALITY_TYPE_HIGH, "High Quality"))

ENUM_TYPE_METADATA(BloomConfiguration::BloomType, ENUM_VALUE(TYPE_NORMAL, "Normal"),
    ENUM_VALUE(TYPE_HORIZONTAL, "Horizontal"), ENUM_VALUE(TYPE_VERTICAL, "Vertical"),
    ENUM_VALUE(TYPE_BILATERAL, "Bilateral"))

DATA_TYPE_METADATA(BloomConfiguration, MEMBER_PROPERTY(bloomType, "Type", 0),
    MEMBER_PROPERTY(bloomQualityType, "Quality Type", 0), MEMBER_PROPERTY(thresholdHard, "Threshold Hard", 0),
    MEMBER_PROPERTY(thresholdSoft, "Threshold Soft", 0), MEMBER_PROPERTY(amountCoefficient, "Amount Coefficient", 0),
    MEMBER_PROPERTY(dirtMaskCoefficient, "Dirt Mask Coefficient", 0), MEMBER_PROPERTY(scatter, "Scatter", 0),
    MEMBER_PROPERTY(scaleFactor, "Scale Factor", 0), MEMBER_PROPERTY(useCompute, "Use Compute", 0))

DATA_TYPE_METADATA(RenderPostProcessBloomNode::EffectProperties, MEMBER_PROPERTY(enabled, "Enabled", 0),
    MEMBER_PROPERTY(bloomConfiguration, "Bloom Configuration", 0))
CORE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr size_t PROPERTY_BYTE_SIZE { sizeof(RenderPostProcessBloomNode::EffectProperties) };
}

RenderPostProcessBloom::RenderPostProcessBloom()
    : properties_(
          &propertiesData, array_view(PropertyType::DataType<RenderPostProcessBloomNode::EffectProperties>::properties))
{}

CORE_NS::IPropertyHandle* RenderPostProcessBloom::GetProperties()
{
    return properties_.GetData();
}

void RenderPostProcessBloom::SetData(BASE_NS::array_view<const uint8_t> data)
{
    if (data.size_bytes() == PROPERTY_BYTE_SIZE) {
        BASE_NS::CloneData(&propertiesData, PROPERTY_BYTE_SIZE, data.data(), data.size_bytes());
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    if (data.size_bytes() != PROPERTY_BYTE_SIZE) {
        PLUGIN_LOG_ONCE_E("RenderPostProcessBloom_Size_Mismatch",
            "RENDER_VALIDATION_ENABLED: RenderPostProcessBloom::SetData(), size missmatch (ignored)");
    }
#endif
}

BASE_NS::array_view<const uint8_t> RenderPostProcessBloom::GetData() const
{
    return { reinterpret_cast<const uint8_t*>(&propertiesData), PROPERTY_BYTE_SIZE };
}
RENDER_END_NAMESPACE()
