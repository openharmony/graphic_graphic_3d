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

#include <scene/base/namespace.h>
#include <scene/interface/intf_postprocess.h>
#include <3d/ecs/components/post_process_component.h>

#include <meta/base/namespace.h>
#include <meta/ext/any_builder.h>
#include <meta/ext/engine/internal_access.h>
#include <meta/interface/engine/intf_engine_data.h>
#include <meta/interface/intf_object_registry.h>

using RENDER_NS::BloomConfiguration;
using RENDER_NS::ColorFringeConfiguration;
using RENDER_NS::TonemapConfiguration;
using RENDER_NS::VignetteConfiguration;
CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(TonemapConfiguration);
DECLARE_PROPERTY_TYPE(TonemapConfiguration::TonemapType);
DECLARE_PROPERTY_TYPE(BloomConfiguration);
DECLARE_PROPERTY_TYPE(BloomConfiguration::BloomType);
DECLARE_PROPERTY_TYPE(BloomConfiguration::BloomQualityType);
DECLARE_PROPERTY_TYPE(ColorFringeConfiguration);
DECLARE_PROPERTY_TYPE(VignetteConfiguration);
CORE_END_NAMESPACE()

META_TYPE(RENDER_NS::TonemapConfiguration);
META_TYPE(RENDER_NS::TonemapConfiguration::TonemapType);
META_TYPE(RENDER_NS::BloomConfiguration);
META_TYPE(RENDER_NS::BloomConfiguration::BloomType);
META_TYPE(RENDER_NS::BloomConfiguration::BloomQualityType);
META_TYPE(RENDER_NS::ColorFringeConfiguration);
META_TYPE(RENDER_NS::VignetteConfiguration);

SCENE_BEGIN_NAMESPACE()

namespace Internal {

template<typename Prop>
void RegisterEngineAccessImpl()
{
    static_assert(CORE_NS::PropertySystem::is_defined<Prop>().value);
    META_NS::GetObjectRegistry().GetEngineData().RegisterInternalValueAccess(
        META_NS::MetaType<Prop>::coreType, CreateShared<META_NS::EngineInternalValueAccess<Prop>>());
}

template<typename Prop, typename AccessType>
void RegisterMapEngineAccessImpl()
{
    static_assert(CORE_NS::PropertySystem::is_defined<Prop>().value);

    auto& r = META_NS::GetObjectRegistry();
    r.GetEngineData().RegisterInternalValueAccess(
        META_NS::MetaType<Prop>::coreType, CreateShared<META_NS::EngineInternalValueAccess<Prop, AccessType>>());
}

template<typename Prop>
void UnregisterEngineAccessImpl()
{
    META_NS::GetObjectRegistry().GetEngineData().UnregisterInternalValueAccess(META_NS::MetaType<Prop>::coreType);
}

void RegisterPostProcessEngineAccess()
{
    RegisterEngineAccessImpl<BloomConfiguration>();
    RegisterEngineAccessImpl<ColorFringeConfiguration>();
    RegisterEngineAccessImpl<TonemapConfiguration>();
    RegisterEngineAccessImpl<VignetteConfiguration>();

    RegisterMapEngineAccessImpl<TonemapConfiguration::TonemapType, TonemapType>();
    RegisterMapEngineAccessImpl<BloomConfiguration::BloomType, BloomType>();
    RegisterMapEngineAccessImpl<BloomConfiguration::BloomQualityType, EffectQualityType>();
}

void UnregisterPostProcessEngineAccess()
{
    UnregisterEngineAccessImpl<TonemapConfiguration::TonemapType>();
    UnregisterEngineAccessImpl<BloomConfiguration::BloomType>();
    UnregisterEngineAccessImpl<BloomConfiguration::BloomQualityType>();
    UnregisterEngineAccessImpl<BloomConfiguration>();
    UnregisterEngineAccessImpl<ColorFringeConfiguration>();
    UnregisterEngineAccessImpl<VignetteConfiguration>();
}

} // namespace Internal
SCENE_END_NAMESPACE()
