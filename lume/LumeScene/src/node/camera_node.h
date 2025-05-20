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

#ifndef SCENE_SRC_NODE_CAMERA_NODE_H
#define SCENE_SRC_NODE_CAMERA_NODE_H

#include <scene/ext/intf_create_entity.h>
#include <scene/interface/intf_camera.h>
#include <scene/interface/intf_raycast.h>

#include "node.h"

SCENE_BEGIN_NAMESPACE()

class CameraNode : public META_NS::IntroduceInterfaces<Node, ICamera, ICreateEntity, ICameraRayCast> {
    META_OBJECT(CameraNode, SCENE_NS::ClassId::CameraNode, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_FORWARDED_PROPERTY_DATA(ICamera, float, FoV)
    META_STATIC_FORWARDED_PROPERTY_DATA(ICamera, float, AspectRatio)
    META_STATIC_FORWARDED_PROPERTY_DATA(ICamera, float, NearPlane)
    META_STATIC_FORWARDED_PROPERTY_DATA(ICamera, float, FarPlane)
    META_STATIC_FORWARDED_PROPERTY_DATA(ICamera, float, XMagnification)
    META_STATIC_FORWARDED_PROPERTY_DATA(ICamera, float, YMagnification)
    META_STATIC_FORWARDED_PROPERTY_DATA(ICamera, float, XOffset)
    META_STATIC_FORWARDED_PROPERTY_DATA(ICamera, float, YOffset)
    META_STATIC_FORWARDED_PROPERTY_DATA(ICamera, CameraProjection, Projection)
    META_STATIC_FORWARDED_PROPERTY_DATA(ICamera, CameraCulling, Culling)
    META_STATIC_FORWARDED_PROPERTY_DATA(ICamera, CameraPipeline, RenderingPipeline)
    META_STATIC_FORWARDED_PROPERTY_DATA(ICamera, uint32_t, SceneFlags)
    META_STATIC_FORWARDED_PROPERTY_DATA(ICamera, uint32_t, PipelineFlags)
    META_STATIC_FORWARDED_PROPERTY_DATA(ICamera, BASE_NS::Math::Vec4, Viewport)
    META_STATIC_FORWARDED_PROPERTY_DATA(ICamera, BASE_NS::Math::Vec4, Scissor)
    META_STATIC_FORWARDED_PROPERTY_DATA(ICamera, BASE_NS::Math::UVec2, RenderTargetSize)
    META_STATIC_FORWARDED_PROPERTY_DATA(ICamera, BASE_NS::Math::Vec4, ClearColor)
    META_STATIC_FORWARDED_PROPERTY_DATA(ICamera, float, ClearDepth)
    META_STATIC_FORWARDED_PROPERTY_DATA(ICamera, IPostProcess::Ptr, PostProcess)
    META_STATIC_FORWARDED_PROPERTY_DATA(ICamera, uint64_t, LayerMask)
    META_STATIC_FORWARDED_PROPERTY_DATA(ICamera, ColorFormat, ColorTargetCustomization)
    META_STATIC_FORWARDED_PROPERTY_DATA(ICamera, BASE_NS::Math::Mat4X4, CustomProjectionMatrix)
    META_END_STATIC_DATA()

    SCENE_USE_COMPONENT_PROPERTY(float, FoV, "CameraComponent")
    SCENE_USE_COMPONENT_PROPERTY(float, AspectRatio, "CameraComponent")
    SCENE_USE_COMPONENT_PROPERTY(float, NearPlane, "CameraComponent")
    SCENE_USE_COMPONENT_PROPERTY(float, FarPlane, "CameraComponent")
    SCENE_USE_COMPONENT_PROPERTY(float, XMagnification, "CameraComponent")
    SCENE_USE_COMPONENT_PROPERTY(float, YMagnification, "CameraComponent")
    SCENE_USE_COMPONENT_PROPERTY(float, XOffset, "CameraComponent")
    SCENE_USE_COMPONENT_PROPERTY(float, YOffset, "CameraComponent")
    SCENE_USE_COMPONENT_PROPERTY(CameraProjection, Projection, "CameraComponent")
    SCENE_USE_COMPONENT_PROPERTY(CameraCulling, Culling, "CameraComponent")
    SCENE_USE_COMPONENT_PROPERTY(CameraPipeline, RenderingPipeline, "CameraComponent")
    SCENE_USE_COMPONENT_PROPERTY(uint32_t, SceneFlags, "CameraComponent")
    SCENE_USE_COMPONENT_PROPERTY(uint32_t, PipelineFlags, "CameraComponent")
    SCENE_USE_COMPONENT_PROPERTY(BASE_NS::Math::Vec4, Viewport, "CameraComponent")
    SCENE_USE_COMPONENT_PROPERTY(BASE_NS::Math::Vec4, Scissor, "CameraComponent")
    SCENE_USE_COMPONENT_PROPERTY(BASE_NS::Math::UVec2, RenderTargetSize, "CameraComponent")
    SCENE_USE_COMPONENT_PROPERTY(BASE_NS::Math::Vec4, ClearColor, "CameraComponent")
    SCENE_USE_COMPONENT_PROPERTY(float, ClearDepth, "CameraComponent")
    SCENE_USE_COMPONENT_PROPERTY(IPostProcess::Ptr, PostProcess, "CameraComponent")
    SCENE_USE_COMPONENT_PROPERTY(uint64_t, LayerMask, "CameraComponent")
    SCENE_USE_COMPONENT_PROPERTY(ColorFormat, ColorTargetCustomization, "CameraComponent")
    SCENE_USE_COMPONENT_PROPERTY(BASE_NS::Math::Mat4X4, CustomProjectionMatrix, "CameraComponent")

    Future<bool> SetActive(bool active = true) override;
    bool IsActive() const override;
    Future<bool> SetRenderTarget(const IRenderTarget::Ptr&) override;

    bool SetEcsObject(const IEcsObject::Ptr&) override;

    void SendInputEvent(PointerEvent& event) override;

public:
    Future<NodeHits> CastRay(const BASE_NS::Math::Vec2& pos, const RayCastOptions& options) const override;
    Future<BASE_NS::Math::Vec3> ScreenPositionToWorld(const BASE_NS::Math::Vec3& pos) const override;
    Future<BASE_NS::Math::Vec3> WorldPositionToScreen(const BASE_NS::Math::Vec3& pos) const override;

public:
    bool Build(const META_NS::IMetadata::Ptr&) override;

    CORE_NS::Entity CreateEntity(const IInternalScene::Ptr& scene) override;

private:
    ICamera::Ptr camera_;
};

SCENE_END_NAMESPACE()

#endif
