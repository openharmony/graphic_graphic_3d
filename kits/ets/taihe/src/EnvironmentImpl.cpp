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

#include "EnvironmentImpl.h"
#include "ImageImpl.h"
#include "Vec3Impl.h"
#include "Vec4Impl.h"
#include "EnvironmentJS.h"

namespace OHOS::Render3D::KITETS {
EnvironmentImpl::EnvironmentImpl(const std::shared_ptr<EnvironmentETS> envETS)
    : SceneResourceImpl(SceneResources::SceneResourceType::key_t::ENVIRONMENT, envETS), envETS_(envETS)
{
    WIDGET_LOGD("EnvironmentImpl ++");
}

std::shared_ptr<EnvironmentETS> EnvironmentImpl::GetEnvETS()
{
    return envETS_;
}

::SceneResources::EnvironmentBackgroundType EnvironmentImpl::getBackgroundType()
{
    RETURN_IF_NULL_WITH_VALUE(envETS_, SceneResources::EnvironmentBackgroundType(
        SceneResources::EnvironmentBackgroundType::key_t::BACKGROUND_NONE));
    return static_cast<SceneResources::EnvironmentBackgroundType::key_t>(envETS_->GetBackgroundType());
}

void EnvironmentImpl::setBackgroundType(::SceneResources::EnvironmentBackgroundType type)
{
    RETURN_IF_NULL(envETS_);
    auto envType = static_cast<EnvironmentETS::EnvironmentBackgroundType>(type.get_value());
    envETS_->SetBackgroundType(envType);
}

::SceneTypes::Vec4 EnvironmentImpl::getIndirectDiffuseFactor()
{
    RETURN_IF_NULL_WITH_VALUE(envETS_, SceneTypes::Vec4({nullptr, nullptr}));
    return taihe::make_holder<Vec4Impl, ::SceneTypes::Vec4>(envETS_->GetIndirectDiffuseFactor());
}

void EnvironmentImpl::setIndirectDiffuseFactor(::SceneTypes::weak::Vec4 factor)
{
    RETURN_IF_NULL(envETS_);
    BASE_NS::Math::Vec4 diffuseFactor{factor->getX(), factor->getY(), factor->getZ(), factor->getW()};
    envETS_->SetIndirectDiffuseFactor(diffuseFactor);
}

::SceneTypes::Vec4 EnvironmentImpl::getIndirectSpecularFactor()
{
    RETURN_IF_NULL_WITH_VALUE(envETS_, SceneTypes::Vec4({nullptr, nullptr}));
    return taihe::make_holder<Vec4Impl, ::SceneTypes::Vec4>(envETS_->GetIndirectSpecularFactor());
}

void EnvironmentImpl::setIndirectSpecularFactor(::SceneTypes::weak::Vec4 factor)
{
    RETURN_IF_NULL(envETS_);
    BASE_NS::Math::Vec4 specularFactor{factor->getX(), factor->getY(), factor->getZ(), factor->getW()};
    envETS_->SetIndirectSpecularFactor(specularFactor);
}

::SceneTypes::Vec4 EnvironmentImpl::getEnvironmentMapFactor()
{
    RETURN_IF_NULL_WITH_VALUE(envETS_, SceneTypes::Vec4({nullptr, nullptr}));
    return taihe::make_holder<Vec4Impl, ::SceneTypes::Vec4>(envETS_->GetEnvironmentMapFactor());
}

void EnvironmentImpl::setEnvironmentMapFactor(::SceneTypes::weak::Vec4 factor)
{
    RETURN_IF_NULL(envETS_);
    BASE_NS::Math::Vec4 envMapFactor{factor->getX(), factor->getY(), factor->getZ(), factor->getW()};
    envETS_->SetEnvironmentMapFactor(envMapFactor);
}

::SceneResources::ImageOrNullOrUndefined EnvironmentImpl::getEnvironmentImage()
{
    RETURN_IF_NULL_WITH_VALUE(envETS_, ::SceneResources::ImageOrNullOrUndefined::make_uValue());
    auto imageETS = envETS_->GetEnvironmentImage();
    if (!imageETS) {
        return ::SceneResources::ImageOrNullOrUndefined::make_nValue();
    }
    auto image = taihe::make_holder<ImageImpl, ::SceneResources::Image>(imageETS);
    return ::SceneResources::ImageOrNullOrUndefined::make_image(image);
}

void EnvironmentImpl::setEnvironmentImage(::SceneResources::ImageOrNullOrUndefined const& image)
{
    RETURN_IF_NULL(envETS_);
    if (image.holds_image()) {
        ::SceneResources::Image img = image.get_image_ref();
        if (img.is_error()) {
            return;
        }
        taihe::optional<int64_t> imageImplOp = static_cast<::SceneResources::SceneResource>(img)->getImpl();
        if (imageImplOp.has_value()) {
            if (auto imageImpl = reinterpret_cast<ImageImpl *>(imageImplOp.value())) {
                envETS_->SetEnvironmentImage(imageImpl->getInternalImage());
                return;
            }
        }
    }
    WIDGET_LOGW("setEnvironmentImage null input");
    envETS_->SetEnvironmentImage(nullptr);
}

::SceneResources::ImageOrNullOrUndefined EnvironmentImpl::getRadianceImage()
{
    RETURN_IF_NULL_WITH_VALUE(envETS_, ::SceneResources::ImageOrNullOrUndefined::make_uValue());
    auto imageETS = envETS_->GetRadianceImage();
    if (!imageETS) {
        return ::SceneResources::ImageOrNullOrUndefined::make_nValue();
    }
    auto image = taihe::make_holder<ImageImpl, ::SceneResources::Image>(imageETS);
    return ::SceneResources::ImageOrNullOrUndefined::make_image(image);
}

void EnvironmentImpl::setRadianceImage(::SceneResources::ImageOrNullOrUndefined const& image)
{
    RETURN_IF_NULL(envETS_);
    if (image.holds_image()) {
        ::SceneResources::Image img = image.get_image_ref();
        if (img.is_error()) {
            return;
        }
        taihe::optional<int64_t> imageImplOp = static_cast<::SceneResources::SceneResource>(img)->getImpl();
        if (imageImplOp.has_value()) {
            auto imageImpl = reinterpret_cast<ImageImpl*>(imageImplOp.value());
            envETS_->SetRadianceImage(imageImpl->getInternalImage());
            return;
        }
    }
    WIDGET_LOGW("setRadianceImage null input");
    envETS_->SetRadianceImage(nullptr);
}

::taihe::optional<::taihe::array<::SceneTypes::Vec3>> EnvironmentImpl::getIrradianceCoefficients()
{
    RETURN_IF_NULL_WITH_VALUE(envETS_, ::taihe::optional<::taihe::array<::SceneTypes::Vec3>>(std::nullopt));
    auto coeffs = envETS_->GetIrradianceCoefficients();
    std::vector<::SceneTypes::Vec3> coeffsVec;
    for (auto& c : coeffs) {
        coeffsVec.push_back(taihe::make_holder<Vec3Impl, ::SceneTypes::Vec3>(c));
    }
    ::taihe::array<::SceneTypes::Vec3> coeffsArr{coeffsVec};
    return ::taihe::optional<::taihe::array<::SceneTypes::Vec3>>(std::in_place, coeffsArr);
}

void EnvironmentImpl::setIrradianceCoefficients(::taihe::optional_view<::taihe::array<::SceneTypes::Vec3>> coefficients)
{
    RETURN_IF_NULL(envETS_);
    if (coefficients.has_value()) {
        BASE_NS::vector<BASE_NS::Math::Vec3> coeffsVector;
        for (auto& coeff : coefficients.value()) {
            coeffsVector.emplace_back(coeff->getX(), coeff->getY(), coeff->getZ());
        }
        envETS_->SetIrradianceCoefficients(coeffsVector);
    } else {
        WIDGET_LOGE("Invalid coefficients");
    }
}

::SceneResources::Environment environmentTransferStaticImpl(uintptr_t input)
{
    WIDGET_LOGI("environmentTransferStaticImpl");
    ani_object esValue = reinterpret_cast<ani_object>(input);
    void *nativePtr = nullptr;
    if (!arkts_esvalue_unwrap(taihe::get_env(), esValue, &nativePtr) || nativePtr == nullptr) {
        WIDGET_LOGE("environmentTransferStaticImpl failed during arkts_esvalue_unwrap.");
        return SceneResources::Environment({nullptr, nullptr});
    }

    EnvironmentJS *tro = reinterpret_cast<EnvironmentJS *>(nativePtr);
    if (tro == nullptr) {
        WIDGET_LOGE("environmentTransferStaticImpl failed, input is not environment.");
        return SceneResources::Environment({nullptr, nullptr});
    }
    SCENE_NS::IEnvironment::Ptr environment = tro->GetNativeObject<SCENE_NS::IEnvironment>();
    if (environment == nullptr) {
        WIDGET_LOGE("environmentTransferStaticImpl failed during GetNativeObject.");
        return SceneResources::Environment({nullptr, nullptr});
    }

    NapiApi::Object sceneJs = tro->GetSceneWeakRef().GetObject();
    if (!sceneJs) {
        WIDGET_LOGE("environmentTransferStaticImpl failed during GetSceneWeakRef.");
        return SceneResources::Environment({nullptr, nullptr});
    }
    SCENE_NS::IScene::Ptr scene = sceneJs.GetNative<SCENE_NS::IScene>();
    if (!scene) {
        WIDGET_LOGE("environmentTransferStaticImpl Invalid scene.");
        return SceneResources::Environment({nullptr, nullptr});
    }

    return taihe::make_holder<EnvironmentImpl, ::SceneResources::Environment>(
        std::make_shared<EnvironmentETS>(environment, scene));
}

uintptr_t environmentTransferDynamicImpl(::SceneResources::Environment input)
{
    WIDGET_LOGI("environmentTransferDynamicImpl");
    RETURN_IF_NULL_WITH_VALUE(!input.is_error(), 0);
    auto envOptional = static_cast<::SceneResources::SceneResource>(input)->getImpl();
    RETURN_IF_NULL_WITH_VALUE(envOptional.has_value(), 0);
    auto implPtr = reinterpret_cast<EnvironmentImpl*>(envOptional.value());
    RETURN_IF_NULL_WITH_VALUE(implPtr, 0);
    std::shared_ptr<EnvironmentETS> envETS = implPtr->GetEnvETS();
    RETURN_IF_NULL_WITH_VALUE(envETS, 0);

    SCENE_NS::IEnvironment::Ptr environment = interface_pointer_cast<SCENE_NS::IEnvironment>(envETS->GetNativeObj());
    RETURN_IF_NULL_WITH_VALUE(environment, 0);
    napi_env jsenv;
    if (!arkts_napi_scope_open(taihe::get_env(), &jsenv)) {
        WIDGET_LOGE("arkts_napi_scope_open failed");
        return 0;
    }
    if (!CheckNapiEnv(jsenv)) {
        WIDGET_LOGE("CheckNapiEnv failed");
        // An error has occurred, ignoring the function call result.
        arkts_napi_scope_close_n(jsenv, 0, nullptr, nullptr);
        return 0;
    }
    SCENE_NS::IScene::Ptr scene = envETS->GetScene();
    if (!scene) {
        WIDGET_LOGE("INVALID SCENE!");
        // An error has occurred, ignoring the function call result.
        arkts_napi_scope_close_n(jsenv, 0, nullptr, nullptr);
        return 0;
    }

    NapiApi::Object sceneJs = CreateFromNativeInstance(jsenv, scene, PtrType::STRONG, {});
    napi_value args[] = {sceneJs.ToNapiValue(), NapiApi::Object(jsenv).ToNapiValue()};

    NapiApi::Object environmentJs = CreateFromNativeInstance(jsenv, environment, PtrType::WEAK, args);
    napi_value envValue = environmentJs.ToNapiValue();
    ani_ref resAny;
    if (!arkts_napi_scope_close_n(jsenv, 1, &envValue, &resAny)) {
        WIDGET_LOGE("arkts_napi_scope_close_n failed");
        return 0;
    }
    return reinterpret_cast<uintptr_t>(resAny);
}
}  // namespace OHOS::Render3D::KITETS

using namespace OHOS::Render3D::KITETS;
// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_environmentTransferStaticImpl(environmentTransferStaticImpl);
TH_EXPORT_CPP_API_environmentTransferDynamicImpl(environmentTransferDynamicImpl);
// NOLINTEND