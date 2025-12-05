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

#ifndef OHOS_3D_MATERIAL_ETS_H
#define OHOS_3D_MATERIAL_ETS_H

#include <scene/interface/intf_material.h>

#include "MaterialPropertyETS.h"
#include "PropertyProxy.h"
#include "SceneResourceETS.h"
#include "ShaderETS.h"

namespace OHOS::Render3D {
class MaterialETS : public SceneResourceETS {
public:
    enum MaterialType {
        SHADER = 1,
        METALLIC_ROUGHNESS = 2,
        UNLIT = 3,
        UNLIT_SHADOW_ALPHA = 100,
        OCCLUSION = 4,
    };
    enum CullMode {
        NONE = 0,
        FRONT = 1,
        BACK = 2,
    };
    enum PolygonMode {
        FILL = 0,
        LINE = 1,
        POINT = 2,
    };
    struct RenderSort {
        uint32_t renderSortLayer;
        uint32_t renderSortLayerOrder;
    };
    enum TextureIndex : size_t {
        BASE_COLOR = 0,
        NORMAL = 1,
        MATERIAL = 2,
        EMISSIVE = 3,
        AMBIENT_OCCLUSION = 4,
        CLEAR_COAT = 5,
        CLEAR_COAT_ROUGHNESS = 6,
        CLEAR_COAT_NORMAL = 7,
        SHEEN = 8,
        SPECULAR = 10,
    };
    MaterialETS(const SCENE_NS::IMaterial::Ptr mat);
    MaterialETS(const SCENE_NS::IMaterial::Ptr mat, const std::string &name, const std::string &uri);
    ~MaterialETS() override;
    void Destroy() override;

    META_NS::IObject::Ptr GetNativeObj() const override;

    MaterialETS::MaterialType GetMaterialType();

    bool GetShadowReceiver();
    void SetShadowReceiver(const bool value);

    MaterialETS::CullMode GetCullMode();
    void SetCullMode(const MaterialETS::CullMode mode);

    MaterialETS::PolygonMode GetPolygonMode();
    void SetPolygonMode(const MaterialETS::PolygonMode mode);

    bool GetBlend();
    void SetBlend(const bool blend);

    float GetAlphaCutoff();
    void SetAlphaCutoff(const float cutoff);

    MaterialETS::RenderSort GetRenderSort();
    void SetRenderSort(const MaterialETS::RenderSort &renderSort);

    std::shared_ptr<ShaderETS> GetColorShader();
    void SetColorShader(const std::shared_ptr<ShaderETS> shader);

    std::shared_ptr<MaterialPropertyETS> GetProperty(const size_t index);
    void SetProperty(const size_t index, const std::shared_ptr<MaterialPropertyETS> property);

    SCENE_NS::IMaterial::Ptr GetNativeMaterial() const
    {
        return material_;
    }

private:
    SCENE_NS::IMaterial::Ptr material_ = nullptr;
    std::shared_ptr<ShaderETS> shader_ = nullptr;
};
}  // namespace OHOS::Render3D
#endif  // OHOS_3D_MATERIAL_ETS_H