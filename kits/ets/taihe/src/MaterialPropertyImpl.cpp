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

#include "MaterialPropertyImpl.h"

#include "ImageImpl.h"
#include "Vec4Impl.h"

namespace OHOS::Render3D::KITETS {
MaterialPropertyImpl::MaterialPropertyImpl(const std::shared_ptr<MaterialPropertyETS> matProp)
    : materialPropertyETS_(matProp)
{
}

MaterialPropertyImpl::~MaterialPropertyImpl()
{
    if (materialPropertyETS_) {
        materialPropertyETS_.reset();
    }
}

::SceneResources::ImageOrNull MaterialPropertyImpl::getImage()
{
    if (!materialPropertyETS_) {
        WIDGET_LOGE("get image failed, materialPropertyETS_ is null");
        return ::SceneResources::ImageOrNull::make_nValue();
    }
    std::shared_ptr<ImageETS> internalImage = materialPropertyETS_->GetImage();
    auto image = taihe::make_holder<ImageImpl, ::SceneResources::Image>(internalImage);
    return ::SceneResources::ImageOrNull::make_image(image);
}

void MaterialPropertyImpl::setImage(::SceneResources::ImageOrNull const &img)
{
    if (!materialPropertyETS_) {
        WIDGET_LOGE("set image failed, internal material property is null");
        return;
    }
    std::shared_ptr<ImageETS> internalImage = nullptr;
    if (img.holds_image()) {
        ::SceneResources::Image image = img.get_image_ref();
        ::taihe::optional<int64_t> implOp = static_cast<::SceneResources::SceneResource>(image)->getImpl();
        if (!implOp.has_value()) {
            WIDGET_LOGE("set image failed, can't get internal image");
            return;
        }
        ImageImpl *imageImpl = reinterpret_cast<ImageImpl *>(implOp.value());
        if (imageImpl == nullptr) {
            WIDGET_LOGE("set image failed, can't get image impl");
            return;
        }
        internalImage = imageImpl->getInternalImage();
    }
    materialPropertyETS_->SetImage(internalImage);
}

::SceneTypes::Vec4 MaterialPropertyImpl::getFactor()
{
    if (!materialPropertyETS_) {
        WIDGET_LOGE("get factor failed, can't get internal material property");
        return ::SceneTypes::Vec4({nullptr, nullptr});
    }
    return taihe::make_holder<Vec4Impl, ::SceneTypes::Vec4>(materialPropertyETS_->GetFactor());
}

void MaterialPropertyImpl::setFactor(::SceneTypes::weak::Vec4 const &factor)
{
    if (!materialPropertyETS_) {
        WIDGET_LOGE("set factor failed, can't get internal material property");
        return;
    }
    BASE_NS::Math::Vec4 innerfactor;
    innerfactor.x = factor->getX();
    innerfactor.y = factor->getY();
    innerfactor.z = factor->getZ();
    innerfactor.w = factor->getW();
    materialPropertyETS_->SetFactor(innerfactor);
}

::taihe::optional<::SceneResources::Sampler> MaterialPropertyImpl::getSampler()
{
    if (!materialPropertyETS_) {
        WIDGET_LOGE("get sampler failed, can't get internal material property");
        return std::nullopt;
    }
    auto sampler = ::taihe::make_holder<SamplerImpl, ::SceneResources::Sampler>(materialPropertyETS_->GetSampler());
    return ::taihe::optional<::SceneResources::Sampler>(std::in_place, sampler);
}

void MaterialPropertyImpl::setSampler(::taihe::optional_view<::SceneResources::Sampler> sampler)
{
    if (!materialPropertyETS_) {
        WIDGET_LOGE("set sampler failed, can't get internal material property");
        return;
    }
    if (sampler) {
        SamplerImpl *si = GetImplPointer<SamplerImpl>(sampler.value()->getImpl());
        if (si == nullptr) {
            WIDGET_LOGE("set sampler failed, parameter is invalid");
            return;
        }
        std::shared_ptr<SamplerETS> samplerETS = si->getInternalObject();
        materialPropertyETS_->SetSampler(samplerETS);
    }
}
}  // namespace OHOS::Render3D::KITETS