/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "MaterialImpl.h"

namespace OHOS::Render3D::KITETS {
MaterialImpl::MaterialImpl(const std::shared_ptr<MaterialETS> mat)
    : SceneResourceImpl(SceneResources::SceneResourceType::key_t::MATERIAL, mat), materialETS_(mat)
{
}

MaterialImpl::~MaterialImpl()
{
    if (materialETS_) {
        materialETS_.reset();
    }
}

::SceneResources::MaterialType MaterialImpl::getMaterialType()
{
    if (!materialETS_) {
        WIDGET_LOGE("get material type failed, invalid material ets");
        return ::SceneResources::MaterialType::key_t::SHADER;
    }
    MaterialETS::MaterialType type = materialETS_->GetMaterialType();
    if (type == MaterialETS::MaterialType::METALLIC_ROUGHNESS) {
        return ::SceneResources::MaterialType::key_t::METALLIC_ROUGHNESS;
    } else if (type == MaterialETS::MaterialType::UNLIT) {
        return ::SceneResources::MaterialType::key_t::UNLIT;
    } else {
        return ::SceneResources::MaterialType::key_t::SHADER;
    }
}

::taihe::optional<bool> MaterialImpl::getShadowReceiver()
{
    if (!materialETS_) {
        WIDGET_LOGE("get shadow receiver failed, invalid material");
        return std::nullopt;
    }
    return ::taihe::optional<bool>(std::in_place, materialETS_->GetShadowReceiver());
}

void MaterialImpl::setShadowReceiver(::taihe::optional_view<bool> value)
{
    if (!materialETS_) {
        WIDGET_LOGE("set shadow receiver failed, invalid material");
        return;
    }
    if (value) {
        materialETS_->SetShadowReceiver(value.value());
    } else {
        materialETS_->SetShadowReceiver(false);
    }
}

::taihe::optional<::SceneResources::CullMode> MaterialImpl::getCullMode()
{
    if (!materialETS_) {
        WIDGET_LOGE("get cull mode failed, invalid material");
        return std::nullopt;
    }
    MaterialETS::CullMode innerMode = materialETS_->GetCullMode();
    ::SceneResources::CullMode mode = ::SceneResources::CullMode::key_t::NONE;
    if (innerMode == MaterialETS::CullMode::FRONT) {
        mode = ::SceneResources::CullMode::key_t::FRONT;
    } else if (innerMode == MaterialETS::CullMode::BACK) {
        mode = ::SceneResources::CullMode::key_t::BACK;
    }
    return ::taihe::optional<::SceneResources::CullMode>(std::in_place, mode);
}

void MaterialImpl::setCullMode(::taihe::optional_view<::SceneResources::CullMode> mode)
{
    if (!materialETS_) {
        WIDGET_LOGE("set cull mode failed, invalid material");
        return;
    }
    MaterialETS::CullMode innerMode = MaterialETS::CullMode::NONE;
    if (mode) {
        ::SceneResources::CullMode modeVal = *mode;
        if (modeVal == ::SceneResources::CullMode::key_t::FRONT) {
            innerMode = MaterialETS::CullMode::FRONT;
        } else if (modeVal == ::SceneResources::CullMode::key_t::BACK) {
            innerMode = MaterialETS::CullMode::BACK;
        }
    }
    materialETS_->SetCullMode(innerMode);
}

::taihe::optional<::SceneResources::PolygonMode> MaterialImpl::getPolygonMode()
{
    if (!materialETS_) {
        WIDGET_LOGE("get polygon mode failed, invalid material");
        return std::nullopt;
    }
    MaterialETS::PolygonMode innerMode = materialETS_->GetPolygonMode();
    ::SceneResources::PolygonMode mode = ::SceneResources::PolygonMode::key_t::FILL;
    if (innerMode == MaterialETS::PolygonMode::FILL) {
        mode = ::SceneResources::PolygonMode::key_t::FILL;
    } else if (innerMode == MaterialETS::PolygonMode::LINE) {
        mode = ::SceneResources::PolygonMode::key_t::LINE;
    } else if (innerMode == MaterialETS::PolygonMode::POINT) {
        mode = ::SceneResources::PolygonMode::key_t::POINT;
    }
    return ::taihe::optional<::SceneResources::PolygonMode>(std::in_place, mode);
}

void MaterialImpl::setPolygonMode(::taihe::optional_view<::SceneResources::PolygonMode> mode)
{
    if (!materialETS_) {
        WIDGET_LOGE("set cull mode failed, invalid material");
        return;
    }
    MaterialETS::PolygonMode innerMode = MaterialETS::PolygonMode::FILL;
    if (mode) {
        ::SceneResources::PolygonMode modeVal = *mode;
        if (modeVal == ::SceneResources::PolygonMode::key_t::FILL) {
            innerMode = MaterialETS::PolygonMode::FILL;
        } else if (modeVal == ::SceneResources::PolygonMode::key_t::LINE) {
            innerMode = MaterialETS::PolygonMode::LINE;
        } else if (modeVal == ::SceneResources::PolygonMode::key_t::POINT) {
            innerMode = MaterialETS::PolygonMode::POINT;
        }
    }
    materialETS_->SetPolygonMode(innerMode);
}

::taihe::optional<::SceneResources::Blend> MaterialImpl::getBlend()
{
    if (!materialETS_) {
        WIDGET_LOGE("get blend failed, invalid material");
        return std::nullopt;
    }
    ::SceneResources::Blend blend{materialETS_->GetBlend()};
    return ::taihe::optional<::SceneResources::Blend>(std::in_place, std::move(blend));
}

void MaterialImpl::setBlend(::taihe::optional_view<::SceneResources::Blend> blend)
{
    if (!materialETS_) {
        WIDGET_LOGE("set blend failed, invalid material");
        return;
    }
    bool blendValue = false;
    if (blend) {
        blendValue = blend.value().enabled;
    }
    materialETS_->SetBlend(blendValue);
}

::taihe::optional<double> MaterialImpl::getAlphaCutoff()
{
    if (!materialETS_) {
        WIDGET_LOGE("get alpha cutoff failed, invalid material ets");
        return std::nullopt;
    }
    float alphaCutoff = materialETS_->GetAlphaCutoff();
    return ::taihe::optional<double>(std::in_place, alphaCutoff);
}

void MaterialImpl::setAlphaCutoff(::taihe::optional_view<double> cutoff)
{
    if (!materialETS_) {
        WIDGET_LOGE("set alpha cutoff failed, invalid material ets");
        return;
    }
    float alphaCutoff = 0.0f;
    if (cutoff) {
        alphaCutoff = cutoff.value();
    }
    materialETS_->SetAlphaCutoff(alphaCutoff);
}

::taihe::optional<::SceneResources::RenderSort> MaterialImpl::getRenderSort()
{
    if (!materialETS_) {
        WIDGET_LOGE("get render sort failed, invalid material ets");
        return std::nullopt;
    }
    MaterialETS::RenderSort innerRenderSort = materialETS_->GetRenderSort();
    ::SceneResources::RenderSort renderSort{
        ::taihe::optional<int32_t>(std::in_place, innerRenderSort.renderSortLayer),
        ::taihe::optional<int32_t>(std::in_place, innerRenderSort.renderSortLayerOrder)};
    return ::taihe::optional<::SceneResources::RenderSort>(std::in_place, std::move(renderSort));
}

void MaterialImpl::setRenderSort(::taihe::optional_view<::SceneResources::RenderSort> renderSort)
{
    if (!materialETS_) {
        WIDGET_LOGE("set render sort failed, invalid material ets");
        return;
    }
    MaterialETS::RenderSort innerRenderSort;
    if (renderSort) {
        ::SceneResources::RenderSort rs = *renderSort;
        if (rs.renderSortLayer) {
            innerRenderSort.renderSortLayer = rs.renderSortLayer.value();
        }
        if (rs.renderSortLayerOrder) {
            innerRenderSort.renderSortLayerOrder = rs.renderSortLayerOrder.value();
        }
    }
    materialETS_->SetRenderSort(innerRenderSort);
}
}  // namespace OHOS::Render3D::KITETS