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

#include <scene/interface/intf_layer.h>

#include "CameraETS.h"

namespace OHOS::Render3D {
std::shared_ptr<CameraETS> CameraETS::FromJS(
    const SCENE_NS::ICamera::Ptr camera, const std::string &name, const std::string &uri)
{
    auto cameraETS = std::make_shared<CameraETS>(camera);
    cameraETS->SetName(name);
    cameraETS->SetUri(uri);
    return cameraETS;
}

CameraETS::CameraETS(const SCENE_NS::ICamera::Ptr camera)
    : NodeETS(NodeETS::NodeType::CAMERA, interface_pointer_cast<SCENE_NS::INode>(camera)), camera_(camera)
{
    CORE_LOG_D("CameraETS ++");
}

CameraETS::~CameraETS()
{
    CORE_LOG_D("CameraETS --");
    clearColorProxy_.reset();
    postProcess_.reset();
    camera_.reset();
}

float CameraETS::GetFov()
{
    if (auto camera = camera_.lock()) {
        return camera->FoV()->GetValue();
    } else {
        return 0.0F;
    }
}

void CameraETS::SetFov(const float fov)
{
    if (auto camera = camera_.lock()) {
        camera->FoV()->SetValue(fov);
    }
}

float CameraETS::GetFar()
{
    if (auto camera = camera_.lock()) {
        return camera->FarPlane()->GetValue();
    } else {
        return 0.0F;
    }
}

void CameraETS::SetFar(const float far)
{
    if (auto camera = camera_.lock()) {
        camera->FarPlane()->SetValue(far);
    }
}

float CameraETS::GetNear()
{
    if (auto camera = camera_.lock()) {
        return camera->NearPlane()->GetValue();
    } else {
        return 0.0F;
    }
}

void CameraETS::SetNear(const float near)
{
    if (auto camera = camera_.lock()) {
        camera->NearPlane()->SetValue(near);
    }
}

std::shared_ptr<PostProcessETS> CameraETS::GetPostProcess()
{
    if (auto camera = camera_.lock()) {
        if (!postProcess_) {
            SCENE_NS::IPostProcess::Ptr postProc = camera->PostProcess()->GetValue();
            if (!postProc) {
                if (SCENE_NS::IScene::Ptr scene = interface_cast<SCENE_NS::INode>(camera)->GetScene()) {
                    postProc = interface_pointer_cast<SCENE_NS::IPostProcess>(
                        scene->CreateObject(SCENE_NS::ClassId::PostProcess).GetResult());
                    camera->PostProcess()->SetValue(postProc);
                }
            }
            postProcess_ = std::make_shared<PostProcessETS>(camera, postProc);
        }
        return postProcess_;
    }
    return nullptr;
}

void CameraETS::SetPostProcess(const std::shared_ptr<PostProcessETS> pp)
{
    auto camera = camera_.lock();
    if (!camera) {
        return;
    }
    if (!postProcess_) {
        SCENE_NS::IPostProcess::Ptr postProc = camera->PostProcess()->GetValue();
        if (!postProc) {
            if (SCENE_NS::IScene::Ptr scene = interface_cast<SCENE_NS::INode>(camera)->GetScene()) {
                postProc = interface_pointer_cast<SCENE_NS::IPostProcess>(
                    scene->CreateObject(SCENE_NS::ClassId::PostProcess).GetResult());
                camera->PostProcess()->SetValue(postProc);
            }
        }
        postProcess_ = std::make_shared<PostProcessETS>(camera, postProc);
    }

    if (pp) {
        if (postProcess_->StrictEqual(pp)) {
            // setting the exactly the same postprocess setting. do nothing.
            return;
        }
        if (postProcess_->IsMatch(pp)) {
            postProcess_->SetToneMapping(pp->GetToneMapping());
            postProcess_->SetBloom(pp->GetBloom());
        } else {
            CORE_LOG_F("Invalid state. Can't change the camera of a post process if it already has one");
        }
    } else {
        postProcess_->SetToneMapping(nullptr);
        postProcess_->SetBloom(nullptr);
    }
}

bool CameraETS::GetEnabled()
{
    if (auto camera = camera_.lock()) {
        return camera->IsActive();
    } else {
        return false;
    }
}

void CameraETS::SetEnabled(const bool enabled)
{
    if (auto camera = camera_.lock()) {
        ExecSyncTask([camera, enabled]() {
            if (camera) {
                uint32_t flags = camera->SceneFlags()->GetValue();
                if (enabled) {
                    flags |= uint32_t(SCENE_NS::CameraSceneFlag::MAIN_CAMERA_BIT);
                } else {
                    flags &= ~uint32_t(SCENE_NS::CameraSceneFlag::MAIN_CAMERA_BIT);
                }
                camera->SceneFlags()->SetValue(flags);
                camera->SetActive(enabled);
            }
            return META_NS::IAny::Ptr{};
        });
    }
}

bool CameraETS::GetMSAA()
{
    if (auto camera = camera_.lock()) {
        uint32_t curBits = camera->PipelineFlags()->GetValue();
        return (curBits & static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::MSAA_BIT));
    } else {
        return false;
    }
}

void CameraETS::SetMSAA(const bool msaaEnabled)
{
    msaaEnabled_ = msaaEnabled;
    if (auto camera = camera_.lock()) {
        uint32_t curBits = camera->PipelineFlags()->GetValue();
        if (msaaEnabled) {
            curBits |= static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::MSAA_BIT);
        } else {
            curBits &= ~static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::MSAA_BIT);
        }
        if (clearColorEnabled_) {
            curBits |= static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::CLEAR_COLOR_BIT);
        } else {
            curBits &= ~static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::CLEAR_COLOR_BIT);
        }
        camera->PipelineFlags()->SetValue(curBits);
    }
}

InvokeReturn<std::shared_ptr<Vec4Proxy>> CameraETS::GetClearColor()
{
    // if (!validateSceneRef()) {
    //     return ctx.GetUndefined();
    // }

    auto camera = camera_.lock();
    if (!camera) {
        return InvokeReturn<std::shared_ptr<Vec4Proxy>>({}, "Invalid camera");
    }
    uint32_t curBits = camera->PipelineFlags()->GetValue();
    bool enabled = curBits & static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::CLEAR_COLOR_BIT);
    if (!enabled) {
        return InvokeReturn<std::shared_ptr<Vec4Proxy>>({}, "ClearColor is not enabled");
    }
    if (!clearColorProxy_) {
        clearColorProxy_ = std::make_shared<Vec4Proxy>(camera->ClearColor());
    }
    return InvokeReturn(clearColorProxy_);
}

void CameraETS::SetClearColor(const bool enabled, const BASE_NS::Math::Vec4 &color)
{
    // if (!validateSceneRef()) {
    //     return;
    // }
    clearColorEnabled_ = enabled;
    auto camera = camera_.lock();
    if (!camera) {
        return;
    }
    if (!clearColorProxy_) {
        clearColorProxy_ = std::make_shared<Vec4Proxy>(camera->ClearColor());
    }
    clearColorProxy_->SetValue(color);
    uint32_t curBits = camera->PipelineFlags()->GetValue();
    if (msaaEnabled_) {
        curBits |= static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::MSAA_BIT);
    } else {
        curBits &= ~static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::MSAA_BIT);
    }
    if (clearColorEnabled_) {
        curBits |= static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::CLEAR_COLOR_BIT);
    } else {
        curBits &= ~static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::CLEAR_COLOR_BIT);
    }
    camera->PipelineFlags()->SetValue(curBits);
}

BASE_NS::Math::Vec3 CameraETS::WorldToScreen(const BASE_NS::Math::Vec3 &world)
{
    if (auto rayCast = interface_pointer_cast<SCENE_NS::ICameraRayCast>(camera_)) {
        return rayCast->WorldPositionToScreen(world).GetResult();
    } else {
        return {};
    }
}

BASE_NS::Math::Vec3 CameraETS::ScreenToWorld(const BASE_NS::Math::Vec3 &screen)
{
    if (auto rayCast = interface_pointer_cast<SCENE_NS::ICameraRayCast>(camera_)) {
        return rayCast->ScreenPositionToWorld(screen).GetResult();
    } else {
        return {};
    }
}

InvokeReturn<std::vector<CameraETS::RaycastResult>> CameraETS::Raycast(const BASE_NS::Math::Vec2 &position,
    const std::shared_ptr<NodeETS> rootNode, const std::shared_ptr<NodeETS> layerMaskNode)
{
    auto rayCast = interface_pointer_cast<SCENE_NS::ICameraRayCast>(camera_);
    if (!rayCast) {
        return InvokeReturn<std::vector<CameraETS::RaycastResult>>({}, "Invalid camera");
    }
    auto layerMask = SCENE_NS::NONE_LAYER_MASK;
    // Layer masks are served as Node objects to the JS side.
    if (layerMaskNode) {
        if (auto layerComponent = SCENE_NS::GetComponent<SCENE_NS::ILayer>(layerMaskNode->GetInternalNode())) {
            layerMask = layerComponent->LayerMask()->GetValue();
        }
    }
    BASE_NS::vector<SCENE_NS::NodeHit> nodeHits =
        rayCast->CastRay(position, {layerMask, rootNode->GetInternalNode()}).GetResult();
    std::vector<CameraETS::RaycastResult> result(nodeHits.size());
    std::transform(nodeHits.begin(), nodeHits.end(), result.begin(), [](const auto &nodeHit) {
        return CameraETS::RaycastResult{FromNative(nodeHit.node), nodeHit.distanceToCenter, nodeHit.position};
    });
    return InvokeReturn<std::vector<CameraETS::RaycastResult>>(result);
}
}  // namespace OHOS::Render3D