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

#include "render_post_process_combined.h"

#include <core/property/property_types.h>
#include <core/property_tools/property_api_impl.inl>

#include "util/log.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
CORE_BEGIN_NAMESPACE()
DATA_TYPE_METADATA(RenderPostProcessCombinedNode::EffectProperties, MEMBER_PROPERTY(enabled, "Enabled", 0))
CORE_END_NAMESPACE()
RENDER_BEGIN_NAMESPACE()

namespace {
constexpr size_t PROPERTY_BYTE_SIZE { sizeof(RenderPostProcessCombinedNode::EffectProperties) };
}

RenderPostProcessCombined::RenderPostProcessCombined() {}

CORE_NS::IPropertyHandle* RenderPostProcessCombined::GetProperties()
{
    return properties_.GetData();
}

void RenderPostProcessCombined::SetData(BASE_NS::array_view<const uint8_t> data)
{
    if (data.size_bytes() == PROPERTY_BYTE_SIZE) {
        BASE_NS::CloneData(&propertiesData, PROPERTY_BYTE_SIZE, data.data(), data.size_bytes());
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    if (data.size_bytes() != PROPERTY_BYTE_SIZE) {
        PLUGIN_LOG_ONCE_E("RenderPostProcessCombined_Size_Mismatch",
            "RENDER_VALIDATION_ENABLED: RenderPostProcessCombined::SetData(), size missmatch (ignored)");
    }
#endif
}

BASE_NS::array_view<const uint8_t> RenderPostProcessCombined::GetData() const
{
    return { reinterpret_cast<const uint8_t*>(&propertiesData), PROPERTY_BYTE_SIZE };
}
RENDER_END_NAMESPACE()
