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

#include "render_data_store_weather.h"

#include <core/intf_engine.h>
#include <core/log.h>
#include <render/intf_render_context.h>

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

CORE3D_BEGIN_NAMESPACE()
RenderDataStoreWeather::RenderDataStoreWeather(const IRenderContext& renderContext, const BASE_NS::string_view name)
    : name_(name), renderContex_(renderContext)
{}

void RenderDataStoreWeather::PostRender()
{
    Clear();
}

void RenderDataStoreWeather::Clear()
{
    // clear every frame
    waterEffectData_.clear();

    // NOTE: weather settings are not cleared at the moment
}

void RenderDataStoreWeather::SetWeatherSettings(uint64_t id, const WeatherSettings& settings)
{
    settings_ = settings;
}

RenderDataStoreWeather::WeatherSettings RenderDataStoreWeather::GetWeatherSettings() const
{
    return settings_;
}

void RenderDataStoreWeather::SetWaterEffect(uint64_t id, Math::Vec2 offset)
{
    waterEffectData_.push_back({ id, offset });
}

array_view<const RenderDataStoreWeather::WaterEffectData> RenderDataStoreWeather::GetWaterEffectData() const
{
    return { waterEffectData_.data(), waterEffectData_.size() };
}

void RenderDataStoreWeather::Ref()
{
    refcnt_.fetch_add(1, std::memory_order_relaxed);
}

void RenderDataStoreWeather::Unref()
{
    if (std::atomic_fetch_sub_explicit(&refcnt_, 1, std::memory_order_release) == 1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        delete this;
    }
}

int32_t RenderDataStoreWeather::GetRefCount()
{
    return refcnt_;
}

BASE_NS::refcnt_ptr<IRenderDataStore> RenderDataStoreWeather::Create(IRenderContext& renderContext, const char* name)
{
    return refcnt_ptr<IRenderDataStore>(new RenderDataStoreWeather(renderContext, name));
}
CORE3D_END_NAMESPACE()