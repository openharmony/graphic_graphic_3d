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

#include "SceneComponentImpl.h"

#include "ImageImpl.h"
#include "property_proxy/ImageProxy.h"
#include "property_proxy/PropertyProxy.h"
#include "Vec2Impl.h"
#include "property_proxy/Vec2Proxy.h"
#include "Vec3Impl.h"
#include "property_proxy/Vec3Proxy.h"
#include "Vec4Impl.h"
#include "property_proxy/Vec4Proxy.h"

namespace OHOS::Render3D::KITETS {
SceneComponentImpl::SceneComponentImpl(std::shared_ptr<SceneComponentETS> sceneComponentETS)
    : sceneComponentETS_(sceneComponentETS)
{}

SceneComponentImpl::~SceneComponentImpl()
{
    sceneComponentETS_.reset();
}

::taihe::string SceneComponentImpl::getName()
{
    if (!sceneComponentETS_) {
        WIDGET_LOGE("empty SceneComponentETS");
        return "";
    }
    return ::taihe::string(sceneComponentETS_->GetName());
}

void SceneComponentImpl::setName(::taihe::string_view name)
{
    if (!sceneComponentETS_) {
        WIDGET_LOGE("empty SceneComponentETS");
        return;
    }
    sceneComponentETS_->SetName(std::string(name));
}

int32_t SceneComponentImpl::getPropertySize()
{
    if (!sceneComponentETS_) {
        WIDGET_LOGE("empty SceneComponentETS");
        return 0;
    }
    return sceneComponentETS_->GetPropertySize();
}

::taihe::array<::taihe::string> SceneComponentImpl::getPropertyKeys()
{
    if (!sceneComponentETS_) {
        WIDGET_LOGE("empty SceneComponentETS");
        return {};
    }
    std::vector<std::string> propertyKeys = sceneComponentETS_->GetPropertyKeys();
    std::vector<::taihe::string> keys;
    keys.reserve(propertyKeys.size());
    for (size_t i = 0; i < propertyKeys.size(); ++i) {
        keys.emplace_back(::taihe::string(propertyKeys[i]));
    }
    return ::taihe::array_view<::taihe::string>(keys);
}

::taihe::optional<::SceneTH::ComponentPropertyType> SceneComponentImpl::getComponentProperty(::taihe::string_view key)
{
    if (!sceneComponentETS_) {
        WIDGET_LOGE("empty SceneComponentETS");
        return std::nullopt;
    }
    std::shared_ptr<IPropertyProxy> propProxy = sceneComponentETS_->GetProperty(std::string(key));
    if (!propProxy) {
        return std::nullopt;
    }
    META_NS::IProperty::Ptr prop = propProxy->GetPropertyPtr();
    if (!prop) {
        return std::nullopt;
    }
    if (META_NS::IsCompatibleWith<BASE_NS::string>(prop)) {
        auto proxy = static_pointer_cast<PropertyProxy<BASE_NS::string>>(propProxy);
        ::taihe::string result{proxy->GetValue().c_str()};
        return ::taihe::optional<::SceneTH::ComponentPropertyType>(
            std::in_place, ::SceneTH::ComponentPropertyType::make_t_string(result));
    }
    if (META_NS::IsCompatibleWith<bool>(prop)) {
        auto proxy = static_pointer_cast<PropertyProxy<bool>>(propProxy);
        return ::taihe::optional<::SceneTH::ComponentPropertyType>(
            std::in_place, ::SceneTH::ComponentPropertyType::make_t_bool(proxy->GetValue()));
    }
    if (META_NS::IsCompatibleWith<int32_t>(prop)) {
        auto proxy = static_pointer_cast<PropertyProxy<int32_t>>(propProxy);
        return ::taihe::optional<::SceneTH::ComponentPropertyType>(
            std::in_place, ::SceneTH::ComponentPropertyType::make_t_i32(proxy->GetValue()));
    }
    if (META_NS::IsCompatibleWith<float>(prop)) {
        auto proxy = static_pointer_cast<PropertyProxy<float>>(propProxy);
        return ::taihe::optional<::SceneTH::ComponentPropertyType>(
            std::in_place, ::SceneTH::ComponentPropertyType::make_t_f64(proxy->GetValue()));
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec2>(prop)) {
        auto proxy = static_pointer_cast<Vec2Proxy>(propProxy);
        ::SceneTypes::Vec2 result = ::taihe::make_holder<Vec2Impl, ::SceneTypes::Vec2>(proxy);
        return ::taihe::optional<::SceneTH::ComponentPropertyType>(
            std::in_place, ::SceneTH::ComponentPropertyType::make_t_vec2(result));
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec3>(prop)) {
        auto proxy = static_pointer_cast<Vec3Proxy>(propProxy);
        ::SceneTypes::Vec3 result = ::taihe::make_holder<Vec3Impl, ::SceneTypes::Vec3>(proxy);
        return ::taihe::optional<::SceneTH::ComponentPropertyType>(
            std::in_place, ::SceneTH::ComponentPropertyType::make_t_vec3(result));
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec4>(prop)) {
        auto proxy = static_pointer_cast<Vec4Proxy>(propProxy);
        ::SceneTypes::Vec4 result = ::taihe::make_holder<Vec4Impl, ::SceneTypes::Vec4>(proxy);
        return ::taihe::optional<::SceneTH::ComponentPropertyType>(
            std::in_place, ::SceneTH::ComponentPropertyType::make_t_vec4(result));
    }
    if (META_NS::IsCompatibleWith<SCENE_NS::IImage::Ptr>(prop)) {
        auto proxy = static_pointer_cast<ImageProxy>(propProxy);
        std::shared_ptr<ImageETS> imageETS = std::make_shared<ImageETS>(proxy->GetValue());
        ::SceneResources::Image result = ::taihe::make_holder<ImageImpl, ::SceneResources::Image>(imageETS);
        return ::taihe::optional<::SceneTH::ComponentPropertyType>(
            std::in_place, ::SceneTH::ComponentPropertyType::make_t_scene_resource(result));
    }
    if (META_NS::IsCompatibleWith<BASE_NS::vector<BASE_NS::string>>(prop)) {
        auto proxy = static_pointer_cast<ArrayPropertyProxy<BASE_NS::string>>(propProxy);
        std::vector<::taihe::string> vec;
        for (auto &s : proxy->GetValue()) {
            vec.emplace_back(s.c_str());
        }
        ::taihe::array<::taihe::string> result{vec};
        return ::taihe::optional<::SceneTH::ComponentPropertyType>(
            std::in_place, ::SceneTH::ComponentPropertyType::make_t_string_arr(result));
    }
    if (META_NS::IsCompatibleWith<BASE_NS::vector<bool>>(prop)) {
        auto proxy = static_pointer_cast<ArrayPropertyProxy<bool>>(propProxy);
        std::vector<int> vec;  // do not use std::vector<bool>
        for (auto &v : proxy->GetValue()) {
            vec.emplace_back(v);
        }
        ::taihe::array<bool> result{vec.data(), vec.size()};
        return ::taihe::optional<::SceneTH::ComponentPropertyType>(
            std::in_place, ::SceneTH::ComponentPropertyType::make_t_bool_arr(result));
    }
    if (META_NS::IsCompatibleWith<BASE_NS::vector<int32_t>>(prop)) {
        auto proxy = static_pointer_cast<ArrayPropertyProxy<int32_t>>(propProxy);
        std::vector<int32_t> vec;
        for (auto &v : proxy->GetValue()) {
            vec.emplace_back(v);
        }
        ::taihe::array<int32_t> result{vec};
        return ::taihe::optional<::SceneTH::ComponentPropertyType>(
            std::in_place, ::SceneTH::ComponentPropertyType::make_t_i32_arr(result));
    }
    if (META_NS::IsCompatibleWith<BASE_NS::vector<float>>(prop)) {
        auto proxy = static_pointer_cast<ArrayPropertyProxy<float>>(propProxy);
        std::vector<double> vec;
        for (auto &v : proxy->GetValue()) {
            vec.emplace_back(v);
        }
        ::taihe::array<double> result{vec};
        return ::taihe::optional<::SceneTH::ComponentPropertyType>(
            std::in_place, ::SceneTH::ComponentPropertyType::make_t_f64_arr(result));
    }
    if (META_NS::IsCompatibleWith<BASE_NS::vector<BASE_NS::Math::Vec2>>(prop)) {
        auto proxy = static_pointer_cast<ArrayPropertyProxy<BASE_NS::Math::Vec2>>(propProxy);
        std::vector<::SceneTypes::Vec2> vec;
        for (auto &v : proxy->GetValue()) {
            vec.push_back(::taihe::make_holder<Vec2Impl, ::SceneTypes::Vec2>(v));
        }
        ::taihe::array<::SceneTypes::Vec2> result{vec};
        return ::taihe::optional<::SceneTH::ComponentPropertyType>(
            std::in_place, ::SceneTH::ComponentPropertyType::make_t_vec2_arr(result));
    }
    if (META_NS::IsCompatibleWith<BASE_NS::vector<BASE_NS::Math::Vec3>>(prop)) {
        auto proxy = static_pointer_cast<ArrayPropertyProxy<BASE_NS::Math::Vec3>>(propProxy);
        std::vector<::SceneTypes::Vec3> vec;
        for (auto &v : proxy->GetValue()) {
            vec.push_back(::taihe::make_holder<Vec3Impl, ::SceneTypes::Vec3>(v));
        }
        ::taihe::array<::SceneTypes::Vec3> result{vec};
        return ::taihe::optional<::SceneTH::ComponentPropertyType>(
            std::in_place, ::SceneTH::ComponentPropertyType::make_t_vec3_arr(result));
    }
    if (META_NS::IsCompatibleWith<BASE_NS::vector<BASE_NS::Math::Vec4>>(prop)) {
        auto proxy = static_pointer_cast<ArrayPropertyProxy<BASE_NS::Math::Vec4>>(propProxy);
        std::vector<::SceneTypes::Vec4> vec;
        for (auto &v : proxy->GetValue()) {
            vec.push_back(::taihe::make_holder<Vec4Impl, ::SceneTypes::Vec4>(v));
        }
        ::taihe::array<::SceneTypes::Vec4> result{vec};
        return ::taihe::optional<::SceneTH::ComponentPropertyType>(
            std::in_place, ::SceneTH::ComponentPropertyType::make_t_vec4_arr(result));
    }
    if (META_NS::IsCompatibleWith<BASE_NS::vector<SCENE_NS::IImage::Ptr>>(prop)) {
        auto proxy = static_pointer_cast<ArrayPropertyProxy<SCENE_NS::IImage::Ptr>>(propProxy);
        std::vector<::SceneResources::SceneResource> vec;
        for (auto &v : proxy->GetValue()) {
            std::shared_ptr<ImageETS> imageETS = std::make_shared<ImageETS>(v);
            auto imageTH = ::taihe::make_holder<ImageImpl, ::SceneResources::Image>(imageETS);
            vec.push_back(static_cast<::SceneResources::SceneResource>(imageTH));
        }
        ::taihe::array<::SceneResources::SceneResource> result{vec};
        return ::taihe::optional<::SceneTH::ComponentPropertyType>(
            std::in_place, ::SceneTH::ComponentPropertyType::make_t_scene_resource_arr(result));
    }
    auto any = META_NS::GetInternalAny(prop);
    WIDGET_LOGE("Unsupported property type [%{public}s]", any ? any->GetTypeIdString().c_str() : "<Unknown>");
    return std::nullopt;
}

void SceneComponentImpl::SetPropertyFromSceneResource(const std::string &name,
                                                      const ::SceneResources::SceneResource &sr)
{
    taihe::optional<int64_t> srOp = sr->getImpl();
    if (!srOp) {
        WIDGET_LOGE("taihe optional does not hold impl");
        return;
    }
    SceneResourceImpl *srImpl = reinterpret_cast<SceneResourceImpl *>(srOp.value());
    if (!srImpl) {
        WIDGET_LOGE("failed to set impl from taihe");
        return;
    }
    auto srType = srImpl->getResourceType();
    switch (srType.get_key()) {
        case ::SceneResources::SceneResourceType::key_t::NODE:
            break;
        case ::SceneResources::SceneResourceType::key_t::ENVIRONMENT:
            break;
        case ::SceneResources::SceneResourceType::key_t::MATERIAL:
            break;
        case ::SceneResources::SceneResourceType::key_t::MESH:
            break;
        case ::SceneResources::SceneResourceType::key_t::ANIMATION:
            break;
        case ::SceneResources::SceneResourceType::key_t::SHADER:
            break;
        case ::SceneResources::SceneResourceType::key_t::IMAGE: {
            std::shared_ptr<ImageETS> imgETS = static_cast<ImageImpl *>(srImpl)->getInternalImage();
            sceneComponentETS_->SetProperty(name, imgETS->GetNativeImage());
            break;
        }
        case ::SceneResources::SceneResourceType::key_t::MESH_RESOURCE:
            break;
        default:
            WIDGET_LOGE("no match SceneResourceType");
            break;
    }
}

void SceneComponentImpl::SetArrayPropertyFromSceneResource(
    const std::string &name, const ::taihe::array<::SceneResources::SceneResource> &srArray)
{
    if (srArray.size() == 0) {
        WIDGET_LOGE("taihe::array<::SceneResources::SceneResource> size is 0");
        return;
    }
    taihe::optional<int64_t> srOp0 = srArray[0]->getImpl();
    if (!srOp0) {
        WIDGET_LOGE("taihe optional does not hold impl");
        return;
    }
    SceneResourceImpl *srImpl0 = reinterpret_cast<SceneResourceImpl *>(srOp0.value());
    if (!srImpl0) {
        WIDGET_LOGE("failed to set impl from taihe");
        return;
    }
    auto srType = srImpl0->getResourceType();
    switch (srType.get_key()) {
        case ::SceneResources::SceneResourceType::key_t::NODE:
            break;
        case ::SceneResources::SceneResourceType::key_t::ENVIRONMENT:
            break;
        case ::SceneResources::SceneResourceType::key_t::MATERIAL:
            break;
        case ::SceneResources::SceneResourceType::key_t::MESH:
            break;
        case ::SceneResources::SceneResourceType::key_t::ANIMATION:
            break;
        case ::SceneResources::SceneResourceType::key_t::SHADER:
            break;
        case ::SceneResources::SceneResourceType::key_t::IMAGE: {
            BASE_NS::vector<SCENE_NS::IImage::Ptr> arr;
            for (auto &sr : srArray) {
                taihe::optional<int64_t> srOp = sr->getImpl();
                if (!srOp.has_value()) {
                    arr.push_back(nullptr);
                    continue;
                }
                SceneResourceImpl *srImpl = reinterpret_cast<SceneResourceImpl *>(srOp.value());
                if (srImpl) {
                    std::shared_ptr<ImageETS> imgETS = static_cast<ImageImpl *>(srImpl)->getInternalImage();
                    arr.push_back(imgETS->GetNativeImage());
                } else {
                    arr.push_back(nullptr);
                }
            }
            sceneComponentETS_->SetArrayProperty(name, arr);
            break;
        }
        case ::SceneResources::SceneResourceType::key_t::MESH_RESOURCE:
            break;
        default:
            WIDGET_LOGE("no match SceneResourceType");
            break;
    }
}

void SceneComponentImpl::setComponentProperty(::taihe::string_view key, ::SceneTH::ComponentPropertyType const &value)
{
    if (!sceneComponentETS_) {
        WIDGET_LOGE("empty SceneComponentETS");
        return;
    }
    const std::string name = std::string(key);
    if (value.holds_t_string()) {
        std::string strValue(value.get_t_string_ref());
        sceneComponentETS_->SetProperty(name, BASE_NS::string(strValue.c_str()));
    } else if (value.holds_t_bool()) {
        sceneComponentETS_->SetProperty(name, value.get_t_bool_ref());
    } else if (value.holds_t_i32()) {
        sceneComponentETS_->SetProperty(name, value.get_t_i32_ref());
    } else if (value.holds_t_f64()) {
        sceneComponentETS_->SetProperty(name, static_cast<float>(value.get_t_f64_ref()));
    } else if (value.holds_t_vec2()) {
        ::SceneTypes::Vec2 vec2 = value.get_t_vec2_ref();
        sceneComponentETS_->SetProperty(name, BASE_NS::Math::Vec2(vec2->getX(), vec2->getY()));
    } else if (value.holds_t_vec3()) {
        ::SceneTypes::Vec3 vec3 = value.get_t_vec3_ref();
        sceneComponentETS_->SetProperty(name, BASE_NS::Math::Vec3(vec3->getX(), vec3->getY(), vec3->getZ()));
    } else if (value.holds_t_vec4()) {
        ::SceneTypes::Vec4 vec4 = value.get_t_vec4_ref();
        sceneComponentETS_->SetProperty(name,
                                        BASE_NS::Math::Vec4(vec4->getX(), vec4->getY(), vec4->getZ(), vec4->getW()));
    } else if (value.holds_t_scene_resource()) {
        SetPropertyFromSceneResource(name, value.get_t_scene_resource_ref());
    } else if (value.holds_t_string_arr()) {
        BASE_NS::vector<BASE_NS::string> arr;
        for (auto &v : value.get_t_string_arr_ref()) {
            arr.emplace_back(v.c_str());
        }
        sceneComponentETS_->SetArrayProperty(name, arr);
    } else if (value.holds_t_bool_arr()) {
        BASE_NS::vector<bool> arr;
        for (auto &v : value.get_t_bool_arr_ref()) {
            arr.emplace_back(v);
        }
        sceneComponentETS_->SetArrayProperty(name, arr);
    } else if (value.holds_t_i32_arr()) {
        BASE_NS::vector<int> arr;
        for (auto &v : value.get_t_i32_arr_ref()) {
            arr.emplace_back(v);
        }
        sceneComponentETS_->SetArrayProperty(name, arr);
    } else if (value.holds_t_f64_arr()) {
        BASE_NS::vector<float> arr;
        for (auto &v : value.get_t_f64_arr_ref()) {
            arr.emplace_back(v);
        }
        sceneComponentETS_->SetArrayProperty(name, arr);
    } else if (value.holds_t_vec2_arr()) {
        BASE_NS::vector<BASE_NS::Math::Vec2> arr;
        for (auto &v : value.get_t_vec2_arr_ref()) {
            arr.emplace_back(v->getX(), v->getY());
        }
        sceneComponentETS_->SetArrayProperty(name, arr);
    } else if (value.holds_t_vec3_arr()) {
        BASE_NS::vector<BASE_NS::Math::Vec3> arr;
        for (auto &v : value.get_t_vec3_arr_ref()) {
            arr.emplace_back(v->getX(), v->getY(), v->getZ());
        }
        sceneComponentETS_->SetArrayProperty(name, arr);
    } else if (value.holds_t_vec4_arr()) {
        BASE_NS::vector<BASE_NS::Math::Vec4> arr;
        for (auto &v : value.get_t_vec4_arr_ref()) {
            arr.emplace_back(v->getX(), v->getY(), v->getZ(), v->getW());
        }
        sceneComponentETS_->SetArrayProperty(name, arr);
    } else if (value.holds_t_scene_resource_arr()) {
        SetArrayPropertyFromSceneResource(name, value.get_t_scene_resource_arr_ref());
    }
}
}  // namespace OHOS::Render3D::KITETS