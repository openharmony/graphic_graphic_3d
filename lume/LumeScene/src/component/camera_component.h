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

#ifndef SCENE_SRC_COMPONENT_CAMERA_COMPONENT_H
#define SCENE_SRC_COMPONENT_CAMERA_COMPONENT_H

#include <scene/ext/component.h>
#include <scene/ext/intf_internal_camera.h>
#include <scene/interface/intf_camera.h>
#include <scene/interface/intf_raycast.h>
#include <shared_mutex>

#include <meta/api/container/find_cache.h>
#include <meta/api/event_handler.h>
#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(CameraComponent, "db677290-7776-419d-8883-f1c6387740b1", META_NS::ObjectCategoryBits::NO_CATEGORY)

class CameraComponent : public META_NS::IntroduceInterfaces<Component, ICamera, IInternalCamera, ICameraRayCast> {
    META_OBJECT(CameraComponent, ClassId::CameraComponent, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_PROPERTY_DATA(ICamera, float, FoV, "CameraComponent.yFov")
    SCENE_STATIC_PROPERTY_DATA(ICamera, float, AspectRatio, "CameraComponent.aspect")
    SCENE_STATIC_PROPERTY_DATA(ICamera, float, NearPlane, "CameraComponent.zNear")
    SCENE_STATIC_PROPERTY_DATA(ICamera, float, FarPlane, "CameraComponent.zFar")
    SCENE_STATIC_PROPERTY_DATA(ICamera, float, XMagnification, "CameraComponent.xMag")
    SCENE_STATIC_PROPERTY_DATA(ICamera, float, YMagnification, "CameraComponent.yMag")
    SCENE_STATIC_PROPERTY_DATA(ICamera, float, XOffset, "CameraComponent.xOffset")
    SCENE_STATIC_PROPERTY_DATA(ICamera, float, YOffset, "CameraComponent.yOffset")
    SCENE_STATIC_PROPERTY_DATA(ICamera, CameraProjection, Projection, "CameraComponent.projection")
    SCENE_STATIC_PROPERTY_DATA(ICamera, CameraCulling, Culling, "CameraComponent.culling")
    SCENE_STATIC_PROPERTY_DATA(ICamera, CameraPipeline, RenderingPipeline, "CameraComponent.renderingPipeline")
    SCENE_STATIC_PROPERTY_DATA(ICamera, uint32_t, SceneFlags, "CameraComponent.sceneFlags")
    SCENE_STATIC_PROPERTY_DATA(ICamera, uint32_t, PipelineFlags, "CameraComponent.pipelineFlags")
    SCENE_STATIC_PROPERTY_DATA(ICamera, BASE_NS::Math::Vec4, Viewport, "CameraComponent.viewport")
    SCENE_STATIC_PROPERTY_DATA(ICamera, BASE_NS::Math::Vec4, Scissor, "CameraComponent.scissor")
    SCENE_STATIC_PROPERTY_DATA(ICamera, BASE_NS::Math::UVec2, RenderTargetSize, "CameraComponent.renderResolution")
    SCENE_STATIC_PROPERTY_DATA(ICamera, BASE_NS::Math::Vec4, ClearColor, "CameraComponent.clearColorValue")
    SCENE_STATIC_PROPERTY_DATA(ICamera, float, ClearDepth, "CameraComponent.clearDepthValue")
    SCENE_STATIC_PROPERTY_DATA(ICamera, uint64_t, CameraLayerMask, "CameraComponent.layerMask")
    SCENE_STATIC_PROPERTY_DATA(
        ICamera, BASE_NS::Math::Mat4X4, CustomProjectionMatrix, "CameraComponent.customProjectionMatrix")
    SCENE_STATIC_PROPERTY_DATA(ICamera, CameraSampleCount, MSAASampleCount, "CameraComponent.msaaSampleCount")

    SCENE_STATIC_DYNINIT_PROPERTY_DATA(ICamera, IPostProcess::Ptr, PostProcess, "CameraComponent.postProcess")
    SCENE_STATIC_DYNINIT_ARRAY_PROPERTY_DATA(
        ICamera, ColorFormat, ColorTargetCustomization, "CameraComponent.colorTargetCustomization")
    SCENE_STATIC_PROPERTY_DATA(ICamera, float, DownsamplePercentage, "CameraComponent.screenPercentage")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(float, FoV)
    META_IMPLEMENT_PROPERTY(float, AspectRatio)
    META_IMPLEMENT_PROPERTY(float, NearPlane)
    META_IMPLEMENT_PROPERTY(float, FarPlane)
    META_IMPLEMENT_PROPERTY(float, XMagnification)
    META_IMPLEMENT_PROPERTY(float, YMagnification)
    META_IMPLEMENT_PROPERTY(float, XOffset)
    META_IMPLEMENT_PROPERTY(float, YOffset)
    META_IMPLEMENT_PROPERTY(CameraProjection, Projection)
    META_IMPLEMENT_PROPERTY(CameraCulling, Culling)
    META_IMPLEMENT_PROPERTY(CameraPipeline, RenderingPipeline)
    META_IMPLEMENT_PROPERTY(uint32_t, SceneFlags)
    META_IMPLEMENT_PROPERTY(uint32_t, PipelineFlags)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec4, Viewport)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec4, Scissor)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::UVec2, RenderTargetSize)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec4, ClearColor)
    META_IMPLEMENT_PROPERTY(float, ClearDepth)
    META_IMPLEMENT_PROPERTY(uint64_t, CameraLayerMask)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Mat4X4, CustomProjectionMatrix)
    META_IMPLEMENT_PROPERTY(CameraSampleCount, MSAASampleCount)

    META_IMPLEMENT_PROPERTY(IPostProcess::Ptr, PostProcess)
    META_IMPLEMENT_PROPERTY(ColorFormat, ColorTargetCustomization)
    META_IMPLEMENT_PROPERTY(float, DownsamplePercentage)

    bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override;

    Future<bool> SetActive(bool active = true) override;
    Future<bool> SetRenderTarget(const IRenderTarget::Ptr&) override;

    bool IsActive() const override;

    void NotifyRenderTargetChanged() override;

    void SendInputEvent(PointerEvent& event) override;

public:
    Future<NodeHits> CastRay(const BASE_NS::Math::Vec2& pos, const RayCastOptions& options) const override;
    Future<BASE_NS::Math::Vec3> ScreenPositionToWorld(const BASE_NS::Math::Vec3& pos) const override;
    Future<BASE_NS::Math::Vec3> WorldPositionToScreen(const BASE_NS::Math::Vec3& pos) const override;

public:
    bool Build(const META_NS::IMetadata::Ptr&) override;
    BASE_NS::string GetName() const override;

    bool Attaching(const IAttach::Ptr& target, const IObject::Ptr& dataContext) override;
    bool Detaching(const IAttach::Ptr& target) override;

private:
    void UpdateEffects(META_NS::ArrayProperty<IEffect::Ptr>& prop);

    META_NS::EventHandler bitmapHandler_;
    IRenderTarget::Ptr renderTarget_;
    META_NS::FindCache<IInputReceiver> inputReceivers_;
};

SCENE_END_NAMESPACE()

#endif
