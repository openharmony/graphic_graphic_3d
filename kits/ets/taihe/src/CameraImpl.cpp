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

#include "CameraImpl.h"

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

#include "ColorImpl.h"
#include "Vec3Impl.h"

CameraImpl::CameraImpl(const std::shared_ptr<CameraETS> cameraETS) : NodeImpl(cameraETS), cameraETS_(cameraETS)
{
    WIDGET_LOGE("CameraImpl ++");
}

CameraImpl::~CameraImpl()
{
    WIDGET_LOGE("CameraImpl --");
    cameraETS_.reset();
}

double CameraImpl::getFov()
{
    if (cameraETS_) {
        return cameraETS_->GetFov();
    } else {
        return 0.0F;
    }
}

void CameraImpl::setFov(double fov)
{
    if (cameraETS_) {
        cameraETS_->SetFov(fov);
    }
}

double CameraImpl::getNearPlane()
{
    if (cameraETS_) {
        return cameraETS_->GetNear();
    } else {
        return 0.0F;
    }
}

void CameraImpl::setNearPlane(double nearPlane)
{
    if (cameraETS_) {
        cameraETS_->SetNear(nearPlane);
    }
}

double CameraImpl::getFarPlane()
{
    if (cameraETS_) {
        return cameraETS_->GetFar();
    } else {
        return 0.0F;
    }
}

void CameraImpl::setFarPlane(double farPlane)
{
    if (cameraETS_) {
        cameraETS_->SetFar(farPlane);
    }
}

bool CameraImpl::getEnabled()
{
    if (cameraETS_) {
        return cameraETS_->GetEnabled();
    } else {
        return false;
    }
}

void CameraImpl::setEnabled(bool enabled)
{
    if (cameraETS_) {
        cameraETS_->SetEnabled(enabled);
    }
}

::SceneNodes::PostProcessSettingsOrNull CameraImpl::getPostProcess()
{
    if (cameraETS_) {
        std::shared_ptr<PostProcessETS> pp = cameraETS_->GetPostProcess();
        if (pp) {
            auto pps = taihe::make_holder<PostProcessSettingsImpl, ScenePostProcessSettings::PostProcessSettings>(pp);
            return SceneNodes::PostProcessSettingsOrNull::make_postProcess(pps);
        }
    }
    return SceneNodes::PostProcessSettingsOrNull::make_nValue();
}

void CameraImpl::setPostProcess(::SceneNodes::PostProcessSettingsOrNull const &process)
{
    if (!cameraETS_) {
        WIDGET_LOGE("The camera is null when set post process settings");
        return;
    }
    if (process.holds_nValue()) {
        WIDGET_LOGI("Set post process to null");
        cameraETS_->SetPostProcess(nullptr);
        return;
    }
    std::shared_ptr<PostProcessETS> pp;
    ScenePostProcessSettings::PostProcessSettings ppValue = process.get_postProcess_ref();
    taihe::optional<int64_t> implOp = ppValue->getImpl();
    bool createNewOne = true;
    if (implOp.has_value()) {
        PostProcessSettingsImpl *ppsi = reinterpret_cast<PostProcessSettingsImpl *>(implOp.value());
        if (ppsi != nullptr) {
            WIDGET_LOGI("a post process settings that assosict with a camera");
            createNewOne = false;
            pp = ppsi->GetInternalSettings();
        }
    }
    if (createNewOne) {
        taihe::optional<ScenePostProcessSettings::ToneMappingSettings> tonemapOp = ppValue->getToneMapping();
        std::shared_ptr<TonemapETS> tonemap;
        if (tonemapOp.has_value()) {
            tonemap = ToneMappingSettingsImpl::CreateInternal(tonemapOp.value());
        } else {
            WIDGET_LOGI("CameraImpl::setPostProcess: disable tonemap");
            tonemap = nullptr;
        }

        taihe::optional<ScenePostProcessSettings::BloomSettings> bloomOp = ppValue->getBloom();
        std::shared_ptr<BloomETS> bloom;
        if (bloomOp.has_value()) {
            bloom = BloomSettingsImpl::CreateInternal(bloomOp.value());
        } else {
            WIDGET_LOGI("CameraImpl::setPostProcess: disable bloom");
            bloom = nullptr;
        }
        pp = PostProcessETS::FromJS(tonemap, bloom);
    }
    cameraETS_->SetPostProcess(pp);
}

::SceneNodes::ColorOrNull CameraImpl::getClearColor()
{
    if (!cameraETS_) {
        return SceneNodes::ColorOrNull::make_nValue();
    }
    InvokeReturn<std::shared_ptr<Vec4Proxy>> color = cameraETS_->GetClearColor();
    if (color.error.empty()) {
        SceneTypes::Color sc = taihe::make_holder<ColorImpl, SceneTypes::Color>(color.value);
        return SceneNodes::ColorOrNull::make_color(sc);
    } else {
        return SceneNodes::ColorOrNull::make_nValue();
    }
}

void CameraImpl::setClearColor(::SceneNodes::ColorOrNull const &color)
{
    if (cameraETS_) {
        if (color.holds_color()) {
            const SceneTypes::Color colorValue = color.get_color_ref();
            cameraETS_->SetClearColor(true,
                BASE_NS::Math::Vec4(colorValue->getR(), colorValue->getG(), colorValue->getB(), colorValue->getA()));
        } else {
            cameraETS_->SetClearColor(false, BASE_NS::Math::ZERO_VEC4);
        }
    }
}

::taihe::array<::SceneTH::RaycastResult> CameraImpl::raycastSync(
    ::SceneTypes::weak::Vec2 viewPosition, ::SceneTH::RaycastParameters const &params)
{
    if (!cameraETS_) {
        taihe::set_error("Invalid camera");
        return {};
    }
    BASE_NS::Math::Vec2 vp(viewPosition->getX(), viewPosition->getY());
    taihe::optional<SceneNodes::Node> rootNodeOp = params.rootNode;
    std::shared_ptr<NodeETS> rootNode{nullptr};
    if (rootNodeOp.has_value()) {
        NodeImpl *ni = reinterpret_cast<NodeImpl *>(rootNodeOp.value()->GetImpl());
        if (ni != nullptr) {
            rootNode = ni->GetInternalNode();
        }
    }
    InvokeReturn<std::vector<CameraETS::RaycastResult>> result = cameraETS_->Raycast(vp, rootNode);
    if (result.error.empty()) {
        std::vector<CameraETS::RaycastResult> rrv = result.value;
        taihe::array<SceneTH::RaycastResult> rra(rrv.size(),
            SceneTH::RaycastResult{taihe::make_holder<NodeImpl, SceneNodes::Node>(nullptr),
                0.0F,
                taihe::make_holder<Vec3Impl, SceneTypes::Vec3>(BASE_NS::Math::ZERO_VEC3)});
        for (size_t index = 0; index < rrv.size(); ++index) {
            rra[index] = {taihe::make_holder<NodeImpl, SceneNodes::Node>(rrv[index].node),
                rrv[index].centerDistance,
                taihe::make_holder<Vec3Impl, SceneTypes::Vec3>(rrv[index].hitPosition)};
        }
        return rra;
    } else {
        taihe::set_error(result.error);
        return {};
    }
}