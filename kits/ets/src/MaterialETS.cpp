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

#include "MaterialETS.h"

namespace OHOS::Render3D {
MaterialETS::MaterialETS(const SCENE_NS::IMaterial::Ptr mat)
    : SceneResourceETS(SceneResourceETS::SceneResourceType::MATERIAL), material_(mat)
{
}

MaterialETS::MaterialETS(const SCENE_NS::IMaterial::Ptr mat, const std::string &name, const std::string &uri)
    : SceneResourceETS(SceneResourceETS::SceneResourceType::MATERIAL), material_(mat)
{
    SetName(name);
    SetUri(uri);
}

MaterialETS::~MaterialETS()
{
    shader_.reset();
    material_.reset();
}

void MaterialETS::Destroy()
{
    if (shader_) {
        shader_->DetachFromMaterial();
    }
    shader_.reset();
    material_.reset();
}

META_NS::IObject::Ptr MaterialETS::GetNativeObj() const
{
    return interface_pointer_cast<META_NS::IObject>(material_);
}

MaterialETS::MaterialType MaterialETS::GetMaterialType()
{
    if (!material_) {
        CORE_LOG_E("get material type failed, invalid material");
        return MaterialType::SHADER;
    }
    SCENE_NS::MaterialType nativeType = META_NS::GetValue(material_->Type());
    if (nativeType == SCENE_NS::MaterialType::METALLIC_ROUGHNESS) {
        return MaterialType::METALLIC_ROUGHNESS;
    } else if (nativeType == SCENE_NS::MaterialType::UNLIT) {
        return MaterialType::UNLIT;
    } else {
        return MaterialType::SHADER;
    }
}

bool MaterialETS::GetShadowReceiver()
{
    if (!material_) {
        CORE_LOG_E("get shadow receiver failed, invalid material");
        return false;
    }
    uint32_t flags = uint32_t(META_NS::GetValue(material_->LightingFlags()));
    return (flags & uint32_t(SCENE_NS::LightingFlags::SHADOW_RECEIVER_BIT));
}

void MaterialETS::SetShadowReceiver(bool value)
{
    if (!material_) {
        CORE_LOG_E("set shadow receiver failed, invalid material");
        return;
    }
    uint32_t flags = uint32_t(META_NS::GetValue(material_->LightingFlags()));
    if (value) {
        flags |= uint32_t(SCENE_NS::LightingFlags::SHADOW_RECEIVER_BIT);
    } else {
        flags &= ~uint32_t(SCENE_NS::LightingFlags::SHADOW_RECEIVER_BIT);
    }
    META_NS::SetValue(material_->LightingFlags(), static_cast<SCENE_NS::LightingFlags>(flags));
}

MaterialETS::CullMode MaterialETS::GetCullMode()
{
    if (!material_) {
        CORE_LOG_E("get cull mode failed, invalid material");
        return CullMode::NONE;
    }
    SCENE_NS::IShader::Ptr shader = META_NS::GetValue(material_->MaterialShader());
    if (!shader) {
        CORE_LOG_E("get cull mode failed, invalid shader");
        return CullMode::NONE;
    }
    SCENE_NS::CullModeFlags flags = META_NS::GetValue(shader->CullMode());
    CullMode mode = CullMode::NONE;
    if (flags == SCENE_NS::CullModeFlags::FRONT_BIT) {
        mode = CullMode::FRONT;
    } else if (flags == SCENE_NS::CullModeFlags::BACK_BIT) {
        mode = CullMode::BACK;
    } else if (flags == SCENE_NS::CullModeFlags::FRONT_AND_BACK) {
        // not export now
    }
    return mode;
}

void MaterialETS::SetCullMode(MaterialETS::CullMode mode)
{
    if (!material_) {
        CORE_LOG_E("set cull mode failed, invalid material");
        return;
    }
    SCENE_NS::IShader::Ptr shader = META_NS::GetValue(material_->MaterialShader());
    if (!shader) {
        CORE_LOG_E("set cull mode failed, invalid shader");
        return;
    }
    SCENE_NS::CullModeFlags flags = SCENE_NS::CullModeFlags::NONE;
    if (mode == CullMode::FRONT) {
        flags = SCENE_NS::CullModeFlags::FRONT_BIT;
    } else if (mode == CullMode::BACK) {
        flags = SCENE_NS::CullModeFlags::BACK_BIT;
    }
    META_NS::SetValue(shader->CullMode(), flags);
    // need to forcefully refresh the material, otherwise render will ignore the changes
    auto val = META_NS::GetValue(material_->MaterialShader());
    META_NS::SetValue(material_->MaterialShader(), val);
}

MaterialETS::PolygonMode MaterialETS::GetPolygonMode()
{
    PolygonMode mode = PolygonMode::FILL;
    if (!material_) {
        CORE_LOG_E("get polygon mode failed, invalid material");
        return mode;
    }
    SCENE_NS::IShader::Ptr shader = META_NS::GetValue(material_->MaterialShader());
    if (!shader) {
        CORE_LOG_E("get polygon mode failed, invalid shader");
        return mode;
    }

    SCENE_NS::PolygonMode innerMode = META_NS::GetValue(shader->PolygonMode());
    if (innerMode == SCENE_NS::PolygonMode::FILL) {
        mode = PolygonMode::FILL;
    } else if (innerMode == SCENE_NS::PolygonMode::LINE) {
        mode = PolygonMode::LINE;
    } else if (innerMode == SCENE_NS::PolygonMode::POINT) {
        mode = PolygonMode::POINT;
    }
    return mode;
}

void MaterialETS::SetPolygonMode(const MaterialETS::PolygonMode mode)
{
    if (!material_) {
        CORE_LOG_E("set polygon mode failed, invalid material");
        return;
    }
    SCENE_NS::IShader::Ptr shader = META_NS::GetValue(material_->MaterialShader());
    if (!shader) {
        CORE_LOG_E("set polygon mode failed, invalid shader");
        return;
    }
    auto polyMode = static_cast<SCENE_NS::PolygonMode>(mode);
    META_NS::SetValue(shader->PolygonMode(), polyMode);
    // need to forcefully refresh the material, otherwise render will ignore the changes
    auto val = META_NS::GetValue(material_->MaterialShader());
    META_NS::SetValue(material_->MaterialShader(), val);
}

bool MaterialETS::GetBlend()
{
    if (!material_) {
        CORE_LOG_E("get blend failed, invalid material");
        return false;
    }
    SCENE_NS::IShader::Ptr shader = META_NS::GetValue(material_->MaterialShader());
    if (!shader) {
        CORE_LOG_E("get blend failed, invalid shader");
        return false;
    }
    return META_NS::GetValue(shader->Blend());
}

void MaterialETS::SetBlend(const bool blend)
{
    if (!material_) {
        CORE_LOG_E("set blend failed, invalid material");
        return;
    }
    SCENE_NS::IShader::Ptr shader = META_NS::GetValue(material_->MaterialShader());
    if (!shader) {
        CORE_LOG_E("set blend failed, invalid shader");
        return;
    }
    META_NS::SetValue(shader->Blend(), blend);
}

float MaterialETS::GetAlphaCutoff()
{
    if (!material_) {
        CORE_LOG_E("get alpha cutoff failed, invalid material");
        return 0.0F;
    }
    return META_NS::GetValue(material_->AlphaCutoff());
}

void MaterialETS::SetAlphaCutoff(const float cutoff)
{
    if (!material_) {
        CORE_LOG_E("set alpha cutoff failed, invalid material");
        return;
    }
    META_NS::SetValue(material_->AlphaCutoff(), cutoff);
}

MaterialETS::RenderSort MaterialETS::GetRenderSort()
{
    if (!material_) {
        CORE_LOG_E("get render sort failed, invalid material");
        return {};
    }
    SCENE_NS::RenderSort renderSort = META_NS::GetValue(material_->RenderSort());
    return RenderSort{renderSort.renderSortLayer, renderSort.renderSortLayerOrder};
}

void MaterialETS::SetRenderSort(const MaterialETS::RenderSort &renderSort)
{
    if (!material_) {
        CORE_LOG_E("set render sort failed, invalid material");
        return;
    }
    SCENE_NS::RenderSort nativeRenderSort;
    nativeRenderSort.renderSortLayer = renderSort.renderSortLayer;
    nativeRenderSort.renderSortLayerOrder = renderSort.renderSortLayerOrder;
    META_NS::SetValue(material_->RenderSort(), nativeRenderSort);
}

std::shared_ptr<ShaderETS> MaterialETS::GetColorShader()
{
    if (!material_) {
        CORE_LOG_E("get color shader failed, invalid material");
        shader_.reset();
        return nullptr;
    }
    if (!shader_) {
        // no shader set yet..
        // see if we have one on the native side.
        // and create the "bound shader" object from it.

        // check native side..
        SCENE_NS::IShader::Ptr shader = material_->MaterialShader()->GetValue();
        if (!shader) {
            // no shader in native also.
            CORE_LOG_E("get color shader failed, no shader set");
            return nullptr;
        }
        shader_ = std::make_shared<ShaderETS>(shader, material_);
    }
    return shader_;
}

void MaterialETS::SetColorShader(const std::shared_ptr<ShaderETS> shader)
{
    if (!material_) {
        CORE_LOG_E("set color shader failed, invalid material");
        shader_.reset();
        return;
    }
    SCENE_NS::IShader::Ptr nativeShader = shader ? shader->GetNativeShader() : nullptr;
    SCENE_NS::MaterialType type =
        nativeShader ? SCENE_NS::MaterialType::CUSTOM : SCENE_NS::MaterialType::METALLIC_ROUGHNESS;
    META_NS::SetValue(material_->Type(), type);

    // bind it to material (in native)
    material_->MaterialShader()->SetValue(nativeShader);

    if (!nativeShader) {
        shader_.reset();
        return;
    }
    if (shader) {
        shader->BindToMaterial(material_);
    }
    shader_ = shader;
}

std::shared_ptr<MaterialPropertyETS> MaterialETS::GetProperty(const size_t index)
{
    if (!material_) {
        CORE_LOG_E("get property failed, material is null");
        return nullptr;
    }
    SCENE_NS::ITexture::Ptr texture = material_->Textures()->GetValueAt(index);
    if (!texture) {
        CORE_LOG_E("get property failed, texture at %zu is null", index);
        return nullptr;
    }
    return std::make_shared<MaterialPropertyETS>(texture);
}

void MaterialETS::SetProperty(const size_t index, const std::shared_ptr<MaterialPropertyETS> property)
{
    if (!material_) {
        CORE_LOG_E("set property failed, material is null");
        return;
    }
    SCENE_NS::ITexture::Ptr texture = material_->Textures()->GetValueAt(index);
    if (!texture) {
        CORE_LOG_E("set property failed, texture at %zu is null", index);
        return;
    }
    if (property) {
        if (auto inTex = property->GetNativeTexture()) {
            // Copy the exposed data from the given texture
            META_NS::SetValue(texture->Image(), META_NS::GetValue(inTex->Image()));
            META_NS::SetValue(texture->Factor(), META_NS::GetValue(inTex->Factor()));
        }
    }
}

}  // namespace OHOS::Render3D