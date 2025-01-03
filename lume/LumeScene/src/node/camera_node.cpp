/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "camera_node.h"

#include <3d/util/intf_scene_util.h>

SCENE_BEGIN_NAMESPACE()

bool CameraNode::Build(const META_NS::IMetadata::Ptr& d)
{
    return Super::Build(d);
}

CORE_NS::Entity CameraNode::CreateEntity(const IInternalScene::Ptr& scene)
{
    const auto& sceneUtil = scene->GetGraphicsContext().GetSceneUtil();
    return sceneUtil.CreateCamera(
        *scene->GetEcsContext().GetNativeEcs(), BASE_NS::Math::Vec3(0.0f, 0.0f, 2.5f), {}, 0.1f, 100.f, 60.f);
}

bool CameraNode::SetEcsObject(const IEcsObject::Ptr& o)
{
    if (Super::SetEcsObject(o)) {
        auto att = GetSelf<META_NS::IAttach>()->GetAttachments<ICamera>();
        if (!att.empty()) {
            camera_ = att.front();
            return true;
        }
    }
    return false;
}

Future<bool> CameraNode::SetActive(bool active)
{
    CORE_ASSERT(camera_);
    return camera_->SetActive(active);
}
bool CameraNode::IsActive() const
{
    CORE_ASSERT(camera_);
    return camera_->IsActive();
}
Future<bool> CameraNode::SetRenderTarget(const IRenderTarget::Ptr& target)
{
    CORE_ASSERT(camera_);
    return camera_->SetRenderTarget(target);
}

Future<NodeHits> CameraNode::CastRay(const BASE_NS::Math::Vec2& pos, const RayCastOptions& options) const
{
    CORE_ASSERT(camera_);
    if (auto i = interface_cast<ICameraRayCast>(camera_)) {
        return i->CastRay(pos, options);
    }
    return {};
}
Future<BASE_NS::Math::Vec3> CameraNode::ScreenPositionToWorld(const BASE_NS::Math::Vec3& pos) const
{
    CORE_ASSERT(camera_);
    if (auto i = interface_cast<ICameraRayCast>(camera_)) {
        return i->ScreenPositionToWorld(pos);
    }
    return {};
}
Future<BASE_NS::Math::Vec3> CameraNode::WorldPositionToScreen(const BASE_NS::Math::Vec3& pos) const
{
    CORE_ASSERT(camera_);
    if (auto i = interface_cast<ICameraRayCast>(camera_)) {
        return i->WorldPositionToScreen(pos);
    }
    return {};
}
SCENE_END_NAMESPACE()