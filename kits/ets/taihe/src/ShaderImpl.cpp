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

#include "ShaderImpl.h"

#include "ImageImpl.h"
#include "PropertyProxy.h"
#include "Vec2Impl.h"
#include "Vec2Proxy.h"
#include "Vec3Impl.h"
#include "Vec3Proxy.h"
#include "Vec4Impl.h"
#include "Vec4Proxy.h"

namespace OHOS::Render3D::KITETS {
ShaderImpl::ShaderImpl(const std::shared_ptr<ShaderETS> &shader)
    : SceneResourceImpl(SceneResources::SceneResourceType::key_t::SHADER, shader), shaderETS_(shader)
{
}

ShaderImpl::~ShaderImpl()
{
    shaderETS_.reset();
}

void ShaderImpl::destroy()
{
    if (shaderETS_) {
        shaderETS_->Destroy();
        shaderETS_.reset();
    }
}

int32_t ShaderImpl::getInputsSize()
{
    if (shaderETS_) {
        return shaderETS_->GetInputsSize();
    }
    return 0;
}

::taihe::array<::taihe::string> ShaderImpl::getInputsKeys()
{
    if (shaderETS_) {
        std::vector<std::string> inputKeys = shaderETS_->GetInputsKeys();
        std::vector<::taihe::string> keys;
        keys.reserve(inputKeys.size());
        for (size_t i = 0; i < inputKeys.size(); ++i) {
            keys.emplace_back(::taihe::string(inputKeys[i]));
        }
        return ::taihe::array_view<::taihe::string>(keys);
    }
    return {};
}

::taihe::optional<::SceneResources::ShaderInputType> ShaderImpl::getInput(::taihe::string_view key)
{
    if (!shaderETS_) {
        WIDGET_LOGE("get input failed, internal shader is null");
        return std::nullopt;
    }
    std::shared_ptr<IPropertyProxy> input = shaderETS_->GetInput(std::string(key));
    if (!input) {
        return std::nullopt;
    }
    META_NS::IProperty::Ptr prop = input->GetPropertyPtr();
    if (!prop) {
        return std::nullopt;
    }
    if (META_NS::IsCompatibleWith<bool>(prop)) {
        // 1.1 interface declaration do not include bool type, For compatibility, use int32_t instead.
        std::shared_ptr<PropertyProxy<bool>> proxy = static_pointer_cast<PropertyProxy<bool>>(input);
        const bool bValue = proxy->GetValue();
        return ::taihe::optional<::SceneResources::ShaderInputType>(
            std::in_place, ::SceneResources::ShaderInputType::make_t_i32(bValue ? 1 : 0));
    } else if (META_NS::IsCompatibleWith<int32_t>(prop)) {
        std::shared_ptr<PropertyProxy<int32_t>> proxy = static_pointer_cast<PropertyProxy<int32_t>>(input);
        return ::taihe::optional<::SceneResources::ShaderInputType>(
            std::in_place, ::SceneResources::ShaderInputType::make_t_i32(proxy->GetValue()));
    } else if (META_NS::IsCompatibleWith<float>(prop)) {
        std::shared_ptr<PropertyProxy<float>> proxy = static_pointer_cast<PropertyProxy<float>>(input);
        return ::taihe::optional<::SceneResources::ShaderInputType>(
            std::in_place, ::SceneResources::ShaderInputType::make_t_f64(proxy->GetValue()));
    } else if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec2>(prop)) {
        std::shared_ptr<Vec2Proxy> proxy = static_pointer_cast<Vec2Proxy>(input);
        ::SceneTypes::Vec2 result = ::taihe::make_holder<Vec2Impl, ::SceneTypes::Vec2>(proxy);
        return ::taihe::optional<::SceneResources::ShaderInputType>(
            std::in_place, ::SceneResources::ShaderInputType::make_t_vec2(result));
    } else if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec3>(prop)) {
        std::shared_ptr<Vec3Proxy> proxy = static_pointer_cast<Vec3Proxy>(input);
        ::SceneTypes::Vec3 result = ::taihe::make_holder<Vec3Impl, ::SceneTypes::Vec3>(proxy);
        return ::taihe::optional<::SceneResources::ShaderInputType>(
            std::in_place, ::SceneResources::ShaderInputType::make_t_vec3(result));
    } else if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec4>(prop)) {
        std::shared_ptr<Vec4Proxy> proxy = static_pointer_cast<Vec4Proxy>(input);
        ::SceneTypes::Vec4 result = ::taihe::make_holder<Vec4Impl, ::SceneTypes::Vec4>(proxy);
        return ::taihe::optional<::SceneResources::ShaderInputType>(
            std::in_place, ::SceneResources::ShaderInputType::make_t_vec4(result));
    } else if (META_NS::IsCompatibleWith<SCENE_NS::IImage::Ptr>(prop)) {
        std::shared_ptr<PropertyProxy<SCENE_NS::IImage::Ptr>> proxy =
            static_pointer_cast<PropertyProxy<SCENE_NS::IImage::Ptr>>(input);
        std::shared_ptr<ImageETS> imageETS = std::make_shared<ImageETS>(proxy->GetValue());
        ::SceneResources::Image result = ::taihe::make_holder<ImageImpl, ::SceneResources::Image>(imageETS);
        return ::taihe::optional<::SceneResources::ShaderInputType>(
            std::in_place, ::SceneResources::ShaderInputType::make_t_image(result));
    }
    auto any = META_NS::GetInternalAny(prop);
    WIDGET_LOGE("Unsupported property type [%{public}s]", any ? any->GetTypeIdString().c_str() : "<Unknown>");
    return std::nullopt;
}

void ShaderImpl::setInput(::taihe::string_view key, ::SceneResources::ShaderInputType const &value)
{
    if (!shaderETS_) {
        WIDGET_LOGE("set input failed, internal shader is null");
        return;
    }
    const std::string name = std::string(key);
    if (value.holds_t_i32()) {
        shaderETS_->SetInput(name, value.get_t_i32_ref());
    } else if (value.holds_t_f64()) {
        shaderETS_->SetInput(name, static_cast<float>(value.get_t_f64_ref()));
    } else if (value.holds_t_vec2()) {
        ::SceneTypes::Vec2 vec2 = value.get_t_vec2_ref();
        shaderETS_->SetInput(name, BASE_NS::Math::Vec2(vec2->getX(), vec2->getY()));
    } else if (value.holds_t_vec3()) {
        ::SceneTypes::Vec3 vec3 = value.get_t_vec3_ref();
        shaderETS_->SetInput(name, BASE_NS::Math::Vec3(vec3->getX(), vec3->getY(), vec3->getZ()));
    } else if (value.holds_t_vec4()) {
        ::SceneTypes::Vec4 vec4 = value.get_t_vec4_ref();
        shaderETS_->SetInput(name, BASE_NS::Math::Vec4(vec4->getX(), vec4->getY(), vec4->getZ(), vec4->getW()));
    } else if (value.holds_t_image()) {
        ::SceneResources::Image img = value.get_t_image_ref();
        if (img.is_error()) {
            return;
        }
        taihe::optional<int64_t> implOp = static_cast<::SceneResources::SceneResource>(img)->getImpl();
        if (!implOp) {
            WIDGET_LOGE("failed to set input, image is not initialized");
            return;
        }
        ImageImpl *imgImpl = reinterpret_cast<ImageImpl *>(implOp.value());
        if (imgImpl == nullptr) {
            WIDGET_LOGE("failed to set input, image is not initialized");
            return;
        }
        std::shared_ptr<ImageETS> imgETS = imgImpl->getInternalImage();
        shaderETS_->SetInput(name, imgETS);
    }
}
}  // namespace OHOS::Render3D::KITETS