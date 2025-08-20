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

#include "EnvironmentETS.h"

namespace OHOS::Render3D {
EnvironmentETS::EnvironmentETS(SCENE_NS::IEnvironment::Ptr environment, const SCENE_NS::IScene::Ptr scene)
    : EnvironmentETS(environment, scene, "", "")
{}

EnvironmentETS::EnvironmentETS(SCENE_NS::IEnvironment::Ptr environment, const SCENE_NS::IScene::Ptr scene,
    const std::string &name)
    : EnvironmentETS(environment, scene, name, "")
{}

EnvironmentETS::EnvironmentETS(SCENE_NS::IEnvironment::Ptr environment, const SCENE_NS::IScene::Ptr scene,
    const std::string &name, const std::string &uri)
    : SceneResourceETS(SceneResourceETS::SceneResourceType::ENVIRONMENT), environment_(environment), scene_(scene)
{
    CORE_LOG_D("EnvironmentETS ++");
    if (!name.empty()) {
        SetName(name);
    }
    if (!uri.empty()) {
        SetUri(uri);
    }
}

EnvironmentETS::~EnvironmentETS()
{
    CORE_LOG_D("EnvironmentETS --");
    environment_.reset();
    diffuseFactor_.reset();
    specularFactor_.reset();
}

META_NS::IObject::Ptr EnvironmentETS::GetNativeObj() const
{
    return interface_pointer_cast<META_NS::IObject>(environment_);
}

EnvironmentETS::EnvironmentBackgroundType EnvironmentETS::GetBackgroundType()
{
    if (!environment_) {
        CORE_LOG_E("empty env object");
        return EnvironmentBackgroundType::BACKGROUND_NONE;
    }
    return EnvironmentBackgroundType(environment_->Background()->GetValue());
}

void EnvironmentETS::SetBackgroundType(EnvironmentBackgroundType typeE)
{
    if (!environment_) {
        CORE_LOG_E("empty env object");
    }
    SCENE_NS::EnvBackgroundType type;
    switch (typeE) {
        case EnvironmentBackgroundType::BACKGROUND_NONE:
            type = SCENE_NS::EnvBackgroundType::NONE;
            break;
        case EnvironmentBackgroundType::BACKGROUND_IMAGE:
            type = SCENE_NS::EnvBackgroundType::IMAGE;
            break;
        case EnvironmentBackgroundType::BACKGROUND_CUBEMAP:
            type = SCENE_NS::EnvBackgroundType::CUBEMAP;
            break;
        case EnvironmentBackgroundType::BACKGROUND_EQUIRECTANGULAR:
            type = SCENE_NS::EnvBackgroundType::EQUIRECTANGULAR;
            break;
        default:
            type = SCENE_NS::EnvBackgroundType::NONE;
            break;
    }
    environment_->Background()->SetValue(type);
}

std::shared_ptr<Vec4Proxy> EnvironmentETS::GetIndirectDiffuseFactor()
{
    if (!environment_) {
        CORE_LOG_E("empty env object");
    }
    if (!diffuseFactor_) {
        diffuseFactor_ = std::make_shared<Vec4Proxy>(environment_->IndirectDiffuseFactor());
    }
    return diffuseFactor_;
}

void EnvironmentETS::SetIndirectDiffuseFactor(const BASE_NS::Math::Vec4 &factor)
{
    if (!environment_) {
        CORE_LOG_E("empty env object");
    }
    if (!diffuseFactor_) {
        diffuseFactor_ = std::make_shared<Vec4Proxy>(environment_->IndirectDiffuseFactor());
    }
    diffuseFactor_->SetValue(factor);
}

std::shared_ptr<Vec4Proxy> EnvironmentETS::GetIndirectSpecularFactor()
{
    if (!environment_) {
        CORE_LOG_E("empty env object");
    }
    if (!specularFactor_) {
        specularFactor_ = std::make_shared<Vec4Proxy>(environment_->IndirectSpecularFactor());
    }
    return specularFactor_;
}

void EnvironmentETS::SetIndirectSpecularFactor(const BASE_NS::Math::Vec4 &factor)
{
    if (!environment_) {
        CORE_LOG_E("empty env object");
    }
    if (!specularFactor_) {
        specularFactor_ = std::make_shared<Vec4Proxy>(environment_->IndirectSpecularFactor());
    }
    specularFactor_->SetValue(factor);
}
}  // namespace OHOS::Render3D