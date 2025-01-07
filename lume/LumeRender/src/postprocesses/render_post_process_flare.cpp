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

#include "render_post_process_flare.h"

#include <core/property_tools/property_api_impl.inl>

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

CORE_BEGIN_NAMESPACE()
DATA_TYPE_METADATA(RenderPostProcessFlare::EffectProperties, MEMBER_PROPERTY(enabled, "Enabled", 0),
    MEMBER_PROPERTY(flarePos, "Flare Position", 0), MEMBER_PROPERTY(intensity, "Effect Intensity", 0))
CORE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
RenderPostProcessFlare::RenderPostProcessFlare()
    : properties_(
          &propertiesData_, array_view(PropertyType::DataType<RenderPostProcessFlare::EffectProperties>::properties))
{}

CORE_NS::IPropertyHandle* RenderPostProcessFlare::GetProperties()
{
    return properties_.GetData();
}
RENDER_END_NAMESPACE()
