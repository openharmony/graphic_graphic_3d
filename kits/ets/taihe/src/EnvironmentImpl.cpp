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
#include "Vec4Impl.h"

EnvironmentImpl::EnvironmentImpl(const std::shared_ptr<EnvironmentETS> envETS)
    : SceneResourceImpl(SceneResources::SceneResourceType::key_t::ENVIRONMENT, envETS), envETS_(envETS)
{
    WIDGET_LOGI("EnvironmentImpl ++");
}

std::shared_ptr<EnvironmentETS> EnvironmentImpl::GetEnvETS()
{
    return envETS_;
}

::SceneResources::EnvironmentBackgroundType EnvironmentImpl::getBackgroundType()
{
    if (!envETS_) {
        taihe::set_error("Invalid Environment");
        return SceneResources::EnvironmentBackgroundType(
            SceneResources::EnvironmentBackgroundType::key_t::BACKGROUND_NONE);
    }
    return static_cast<SceneResources::EnvironmentBackgroundType::key_t>(envETS_->GetBackgroundType());
}

void EnvironmentImpl::setBackgroundType(::SceneResources::EnvironmentBackgroundType type)
{
    if (!envETS_) {
        taihe::set_error("Invalid Environment");
        return;
    }
    auto envType = static_cast<EnvironmentETS::EnvironmentBackgroundType>(type.get_value());
    envETS_->SetBackgroundType(envType);
}

::SceneTypes::Vec4 EnvironmentImpl::getIndirectDiffuseFactor()
{
    if (!envETS_) {
        taihe::set_error("Invalid Environment");
        return SceneTypes::Vec4({nullptr, nullptr});
    }
    return taihe::make_holder<Vec4Impl, ::SceneTypes::Vec4>(envETS_->GetIndirectDiffuseFactor());
}

void EnvironmentImpl::setIndirectDiffuseFactor(::SceneTypes::weak::Vec4 factor)
{
    if (!envETS_) {
        taihe::set_error("Invalid Environment");
        return;
    }
    BASE_NS::Math::Vec4 diffuseFactor{factor->getX(), factor->getY(), factor->getZ(), factor->getW()};
    envETS_->SetIndirectDiffuseFactor(diffuseFactor);
}

::SceneTypes::Vec4 EnvironmentImpl::getIndirectSpecularFactor()
{
    if (!envETS_) {
        taihe::set_error("Invalid Environment");
        return SceneTypes::Vec4({nullptr, nullptr});
    }
    return taihe::make_holder<Vec4Impl, ::SceneTypes::Vec4>(envETS_->GetIndirectSpecularFactor());
}

void EnvironmentImpl::setIndirectSpecularFactor(::SceneTypes::weak::Vec4 factor)
{
    if (!envETS_) {
        taihe::set_error("Invalid Environment");
        return;
    }
    BASE_NS::Math::Vec4 specularFactor{factor->getX(), factor->getY(), factor->getZ(), factor->getW()};
    envETS_->SetIndirectSpecularFactor(specularFactor);
}
