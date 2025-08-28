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

#include "ShaderETS.h"

#include <scene/interface/intf_material.h>
#include <scene/interface/intf_shader.h>
#include <scene/interface/intf_texture.h>

#include "ImageETS.h"
#include "SceneResourceETS.h"
#include "Vec2Proxy.h"
#include "Vec3Proxy.h"
#include "Vec4Proxy.h"

namespace OHOS::Render3D {
// list of default names
constexpr const BASE_NS::string_view DEFAULT_TEXTURE_NAMES[] = {
    "BASE_COLOR",          "NORMAL",           "MATERIAL", "EMISSIVE",      "AO",      "CLEARCOAT",
    "CLEARCOAT_ROUGHNESS", "CLEARCOAT_NORMAL", "SHEEN",    "TRANSM-ISSION", "SPECULAR"};

ShaderETS::ShaderETS(const SCENE_NS::IShader::Ptr &shader, const SCENE_NS::IMaterial::Ptr &material)
    : SceneResourceETS(SceneResourceETS::SceneResourceType::SHADER), shader_(shader)
{
    BindToMaterial(material);
}

ShaderETS::ShaderETS(const SCENE_NS::IShader::Ptr &shader, const std::string &name, const std::string &uri)
    : SceneResourceETS(SceneResourceETS::SceneResourceType::SHADER), shader_(shader)
{
    SetName(name);
    SetUri(uri);
}

void ShaderETS::BindToMaterial(const SCENE_NS::IMaterial::Ptr &material)
{
    if (!material) {
        return;
    }
    keys_.clear();
    proxies_.clear();
    BASE_NS::vector<SCENE_NS::ITexture::Ptr> textures = material->Textures()->GetValue();
    if (!textures.empty()) {
        int index = 0;
        for (auto &texture : textures) {
            BASE_NS::string name;
            auto nn = interface_cast<META_NS::IObject>(texture);
            if (nn) {
                name = nn->GetName();
            } else {
                name = "TextureInfo_" + BASE_NS::to_string(index);
            }
            std::shared_ptr<IPropertyProxy> proxt;
            // factor
            if (proxt = std::make_shared<Vec4Proxy>(texture->Factor())) {
                auto n = (name.empty() ? BASE_NS::string_view("") : name + BASE_NS::string_view("_")) +
                         texture->Factor()->GetName();
                proxies_.insert_or_assign(n.c_str(), std::move(proxt));
            }
            // image
            if (proxt = std::make_shared<PropertyProxy<SCENE_NS::IImage::Ptr>>(texture->Image())) {
                auto n = (name.empty() ? BASE_NS::string_view("") : name + BASE_NS::string_view("_")) +
                         texture->Image()->GetName();
                proxies_.insert_or_assign(n.c_str(), std::move(proxt));
            }

            // create default named property, if it was renamed. Backward compatible API
            if (name != DEFAULT_TEXTURE_NAMES[index]) {
                if (proxt = std::make_shared<Vec4Proxy>(texture->Factor())) {
                    auto n = DEFAULT_TEXTURE_NAMES[index] + "_" + texture->Factor()->GetName();
                    proxies_.insert_or_assign(n.c_str(), std::move(proxt));
                }
                if (proxt = std::make_shared<PropertyProxy<SCENE_NS::IImage::Ptr>>(texture->Image())) {
                    auto n = DEFAULT_TEXTURE_NAMES[index] + "_" + texture->Image()->GetName();
                    proxies_.insert_or_assign(n.c_str(), std::move(proxt));
                }
            }
            index++;
        }
    }
    // default stuff
    {
        if (auto proxt = std::make_shared<PropertyProxy<float>>(material->AlphaCutoff())) {
            proxies_.insert_or_assign(material->AlphaCutoff()->GetName().c_str(), std::move(proxt));
        }
    }
    META_NS::IMetadata::Ptr customProperties = material->GetCustomProperties();
    if (customProperties) {
        for (auto t : customProperties->GetProperties()) {
            if (auto proxt = PropertyToProxy(t)) {
                proxies_.insert_or_assign(t->GetName().c_str(), std::move(proxt));
            }
        }
    }
    keys_.reserve(proxies_.size());
    for (const auto &pair : proxies_) {
        keys_.push_back(pair.first);
    }
}

ShaderETS::~ShaderETS()
{
    keys_.clear();
    proxies_.clear();
    shader_.reset();
}

std::shared_ptr<IPropertyProxy> ShaderETS::GetInput(const std::string &key)
{
    return proxies_[key];
}

void ShaderETS::SetInput(const std::string &key, const std::any &value)
{
    std::shared_ptr<IPropertyProxy> input = proxies_[key];
    if (!input) {
        CORE_LOG_E("set input failed, input '%s' doesn't exist", key.c_str());
        return;
    }
    META_NS::IProperty::Ptr prop = input->GetPropertyPtr();
    if (!prop) {
        CORE_LOG_E("set input failed, input '%s' is invalid", key.c_str());
        return;
    }
    if (META_NS::IsCompatibleWith<bool>(prop)) {
        std::shared_ptr<PropertyProxy<bool>> proxy = static_pointer_cast<PropertyProxy<bool>>(input);
        proxy->SetValue(std::any_cast<bool>(value));
    } else if (META_NS::IsCompatibleWith<int32_t>(prop)) {
        std::shared_ptr<PropertyProxy<int32_t>> proxy = static_pointer_cast<PropertyProxy<int32_t>>(input);
        proxy->SetValue(std::any_cast<int32_t>(value));
    } else if (META_NS::IsCompatibleWith<float>(prop)) {
        std::shared_ptr<PropertyProxy<float>> proxy = static_pointer_cast<PropertyProxy<float>>(input);
        proxy->SetValue(std::any_cast<float>(value));
    } else if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec2>(prop)) {
        std::shared_ptr<Vec2Proxy> proxy = static_pointer_cast<Vec2Proxy>(input);
        proxy->SetValue(std::any_cast<BASE_NS::Math::Vec2>(value));
    } else if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec3>(prop)) {
        std::shared_ptr<Vec3Proxy> proxy = static_pointer_cast<Vec3Proxy>(input);
        proxy->SetValue(std::any_cast<BASE_NS::Math::Vec3>(value));
    } else if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec4>(prop)) {
        std::shared_ptr<Vec4Proxy> proxy = static_pointer_cast<Vec4Proxy>(input);
        proxy->SetValue(std::any_cast<BASE_NS::Math::Vec4>(value));
    } else if (META_NS::IsCompatibleWith<SCENE_NS::IImage::Ptr>(prop)) {
        std::shared_ptr<PropertyProxy<SCENE_NS::IImage::Ptr>> proxy =
            static_pointer_cast<PropertyProxy<SCENE_NS::IImage::Ptr>>(input);
        std::shared_ptr<ImageETS> imageETS = std::any_cast<std::shared_ptr<ImageETS>>(value);
        if (imageETS) {
            proxy->SetValue(imageETS->GetNativeImage());
        }
    }
    auto any = META_NS::GetInternalAny(prop);
    CORE_LOG_E("Unsupported property type [%s]", any ? any->GetTypeIdString().c_str() : "<Unknown>");
}

META_NS::IObject::Ptr ShaderETS::GetNativeObj() const
{
    return interface_pointer_cast<META_NS::IObject>(shader_);
}
}  // namespace OHOS::Render3D