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

#include <3d/ecs/components/camera_component.h>
#include <base/math/matrix.h>
#include <base/math/matrix_util.h>

#include "../component/camera_effect_component.h"

SCENE_BEGIN_NAMESPACE()

using META_NS::GetValue;
using META_NS::SetValue;

bool CameraNode::Build(const META_NS::IMetadata::Ptr& d)
{
    return Super::Build(d);
}

CORE_NS::Entity CameraNode::CreateEntity(const IInternalScene::Ptr& scene)
{
    const auto& ecs = scene->GetEcsContext();
    auto ccm = CORE_NS::GetManager<CORE3D_NS::ICameraComponentManager>(*ecs.GetNativeEcs());
    if (!ccm) {
        return {};
    }

    auto ent = ecs.GetNativeEcs()->GetEntityManager().Create();
    ecs.AddDefaultComponents(ent);

    CORE3D_NS::CameraComponent cc;
    cc.sceneFlags |= CORE3D_NS::CameraComponent::SceneFlagBits::ACTIVE_RENDER_BIT;
    ccm->Set(ent, cc);

    return ent;
}

bool CameraNode::SetEcsObject(const IEcsObject::Ptr& o)
{
    if (Super::SetEcsObject(o)) {
        auto att = GetSelf<META_NS::IAttach>()->GetAttachments<ICamera>();
        if (!att.empty()) {
            camera_ = att.front();
            return true;
        }
        if (auto effects =
                interface_pointer_cast<IEcsObjectAccess>(GetEffectComponent(META_NS::MetadataQuery::EXISTING))) {
            effects->SetEcsObject(o);
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
void CameraNode::SendInputEvent(PointerEvent& event)
{
    CORE_ASSERT(camera_);
    camera_->SendInputEvent(event);
}

float CameraNode::GetAspectRatio() const
{
    const auto renderResolution = META_ACCESS_PROPERTY_VALUE(RenderTargetSize);
    const auto currentAspect = META_ACCESS_PROPERTY_VALUE(AspectRatio);
    auto aspect = 1.f;
    if (currentAspect > 0.f) {
        aspect = currentAspect;
    } else if (renderResolution.y > 0U) {
        aspect = static_cast<float>(renderResolution.x) / static_cast<float>(renderResolution.y);
    }
    return aspect;
}

BASE_NS::Math::Mat4X4 CameraNode::GetOrthoProjectionMatrix() const
{
    const auto zNear = META_ACCESS_PROPERTY_VALUE(NearPlane);
    const auto zFar = META_ACCESS_PROPERTY_VALUE(FarPlane);
    const auto xMagnification = META_ACCESS_PROPERTY_VALUE(XMagnification);
    const auto yMagnification = META_ACCESS_PROPERTY_VALUE(YMagnification);
    auto orthoProj = BASE_NS::Math::OrthoRhZo(
        xMagnification * -0.5f, xMagnification * 0.5f, yMagnification * -0.5f, yMagnification * 0.5f, zNear, zFar);
    orthoProj[1][1] *= -1.f; // left-hand NDC while Vulkan right-handed -> flip y
    return orthoProj;
}

BASE_NS::Math::Mat4X4 CameraNode::GetPerspectiveProjectionMatrix() const
{
    const auto zNear = META_ACCESS_PROPERTY_VALUE(NearPlane);
    const auto zFar = META_ACCESS_PROPERTY_VALUE(FarPlane);
    const auto yFov = META_ACCESS_PROPERTY_VALUE(FoV);
    auto persProj = BASE_NS::Math::PerspectiveRhZo(yFov, GetAspectRatio(), zNear, zFar);
    persProj[1][1] *= -1.f; // left-hand NDC while Vulkan right-handed -> flip y
    return persProj;
}

BASE_NS::Math::Mat4X4 CameraNode::GetFrustumProjectionMatrix() const
{
    float zNear = META_ACCESS_PROPERTY_VALUE(NearPlane);
    float zFar = META_ACCESS_PROPERTY_VALUE(FarPlane);
    float yFov = META_ACCESS_PROPERTY_VALUE(FoV);
    float cameraXOffset = META_ACCESS_PROPERTY_VALUE(XOffset);
    float cameraYOffset = META_ACCESS_PROPERTY_VALUE(YOffset);
    const auto aspect = GetAspectRatio();
    // with offset 0.5, the camera should offset half the screen
    const float scale = tan(yFov * 0.5f) * zNear;
    const float xOffset = cameraXOffset * scale * aspect * 2.0f;
    const float yOffset = cameraYOffset * scale * 2.0f;
    float left = -aspect * scale;
    float right = aspect * scale;
    float bottom = -scale;
    float top = scale;
    auto persProj =
        BASE_NS::Math::PerspectiveRhZo(left + xOffset, right + xOffset, bottom + yOffset, top + yOffset, zNear, zFar);
    persProj[1][1] *= -1.f; // left-hand NDC while Vulkan right-handed -> flip y
    return persProj;
}

BASE_NS::Math::Mat4X4 CameraNode::GetProjectionMatrix() const
{
    const auto type = META_ACCESS_PROPERTY_VALUE(Projection);
    switch (type) {
        case SCENE_NS::CameraProjection::ORTHOGRAPHIC:
            return GetOrthoProjectionMatrix();
        case SCENE_NS::CameraProjection::PERSPECTIVE:
            return GetPerspectiveProjectionMatrix();
        case SCENE_NS::CameraProjection::FRUSTUM:
            return GetFrustumProjectionMatrix();
        case SCENE_NS::CameraProjection::CUSTOM:
            return META_ACCESS_PROPERTY_VALUE(CustomProjectionMatrix);
        default:
            return BASE_NS::Math::IDENTITY_4X4;
    }
}

BASE_NS::Math::Mat4X4 CameraNode::GetViewMatrix() const
{
    BASE_NS::Math::Mat4X4 m = BASE_NS::Math::IDENTITY_4X4;
    auto node = GetSelf<INode>();
    while (node) {
        m = node->GetTransformMatrix() * m;
        node = node->GetParent().GetResult();
    }
    return BASE_NS::Math::Inverse(m);
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

ICameraEffect::Ptr CameraNode::GetEffectComponent(META_NS::MetadataQuery flag) const
{
    if (auto attach = GetSelf<IAttach>()) {
        if (auto ce = attach->GetAttachments<ICameraEffect>(); !ce.empty()) {
            // Assume we have only at most one CameraEffectComponent
            return ce.front();
        }
        if (flag == META_NS::MetadataQuery::CONSTRUCT_ON_REQUEST) {
            auto effects = META_NS::GetObjectRegistry().Create<ICameraEffect>(ClassId::CameraEffectComponent);
            if (attach->Attach(effects)) {
                if (auto ecso = interface_cast<IEcsObjectAccess>(effects)) {
                    ecso->SetEcsObject(object_);
                }
                return effects;
            }
        }
    }
    return {};
}

META_NS::ArrayProperty<IEffect::Ptr> CameraNode::GetEffectsProperty() const
{
    META_NS::ArrayProperty<IEffect::Ptr> p(effects_.lock());
    if (!p) {
        // We haven't initialized the local Effects property, create a CameraEffectComponent and take the
        // property from there
        if (auto effects = GetEffectComponent(META_NS::MetadataQuery::CONSTRUCT_ON_REQUEST)) {
            p = effects->Effects();
            effects_ = p.GetProperty();
        }
    }
    return p;
}

SCENE_END_NAMESPACE()
