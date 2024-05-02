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

#ifndef SCENEPLUGINAPI_CAMERA_H
#define SCENEPLUGINAPI_CAMERA_H

#include <scene_plugin/api/camera_uid.h>
#include <scene_plugin/api/postprocess.h>
#include <scene_plugin/api/postprocess_uid.h>
#include <scene_plugin/interface/intf_camera.h>

#include <meta/api/internal/object_api.h>
SCENE_BEGIN_NAMESPACE()

/**
 * @brief Camera class wraps ICamera interface. It keeps the referenced object alive using strong ref.
 *        The construction of the object is asynchronous, the properties of the engine may not be available
 *        right after the object instantiation, but OnLoaded() event can be used to observe the state changes.
 */
class Camera final : public META_NS::Internal::ObjectInterfaceAPI<Camera, ClassId::Camera> {
    META_API(Camera)
    META_API_OBJECT_CONVERTIBLE(ICamera)
    META_API_OBJECT_CONVERTIBLE(INode)
    META_API_CACHE_INTERFACE(ICamera, Camera)
    META_API_CACHE_INTERFACE(INode, Node)

public:
    // From Node
    META_API_INTERFACE_PROPERTY_CACHED(Node, Name, BASE_NS::string)
    META_API_INTERFACE_PROPERTY_CACHED(Node, Position, BASE_NS::Math::Vec3)
    META_API_INTERFACE_PROPERTY_CACHED(Node, Scale, BASE_NS::Math::Vec3)
    META_API_INTERFACE_PROPERTY_CACHED(Node, Rotation, BASE_NS::Math::Quat)
    META_API_INTERFACE_PROPERTY_CACHED(Node, Visible, bool)
    META_API_INTERFACE_PROPERTY_CACHED(Node, LayerMask, uint64_t)
    META_API_INTERFACE_PROPERTY_CACHED(Node, LocalMatrix, BASE_NS::Math::Mat4X4)
    META_API_INTERFACE_PROPERTY_CACHED(Camera, FoV, float)
    META_API_INTERFACE_PROPERTY_CACHED(Camera, AspectRatio, float)
    META_API_INTERFACE_PROPERTY_CACHED(Camera, NearPlane, float)
    META_API_INTERFACE_PROPERTY_CACHED(Camera, FarPlane, float)
    META_API_INTERFACE_PROPERTY_CACHED(Camera, XOffset, float)
    META_API_INTERFACE_PROPERTY_CACHED(Camera, YOffset, float)
    META_API_INTERFACE_PROPERTY_CACHED(Camera, Projection, uint8_t)
    META_API_INTERFACE_PROPERTY_CACHED(Camera, Culling, uint8_t)
    META_API_INTERFACE_PROPERTY_CACHED(Camera, RenderingPipeline, uint8_t)
    META_API_INTERFACE_PROPERTY_CACHED(Camera, SceneFlags, uint32_t)
    META_API_INTERFACE_PROPERTY_CACHED(Camera, PipelineFlags, uint32_t)
    META_API_INTERFACE_PROPERTY_CACHED(Camera, Viewport, BASE_NS::Math::Vec4)
    META_API_INTERFACE_PROPERTY_CACHED(Camera, Scissor, BASE_NS::Math::Vec4)
    META_API_INTERFACE_PROPERTY_CACHED(Camera, RenderTargetSize, BASE_NS::Math::UVec2)
    META_API_INTERFACE_PROPERTY_CACHED(Camera, ClearColor, SCENE_NS::Color)
    META_API_INTERFACE_PROPERTY_CACHED(Camera, ClearDepth, float)
    MAP_PROPERTY_TO_USER(Camera, PostProcess)

    /**
     * @brief Set camera to use orthographic projection with default property values
     */
    void SetOrthographicProjection()
    {
        if (Projection()->GetValue() != SCENE_NS::ICamera::SCENE_CAM_PROJECTION_ORTHOGRAPHIC) {
            auto renderSize = RenderTargetSize()->GetValue();
            XOffset()->SetValue(renderSize.x / 2);
            YOffset()->SetValue(renderSize.y / 2);
            Projection()->SetValue(SCENE_NS::ICamera::SCENE_CAM_PROJECTION_ORTHOGRAPHIC);
        }
    }

    /**
     * @brief Set camera to use perspective projection. This is on by default.
     */
    void SetPerspectiveProjection()
    {
        META_API_CACHED_INTERFACE(Camera)->Projection()->SetValue(SCENE_NS::ICamera::SCENE_CAM_PROJECTION_PERSPECTIVE);
    }

    /**
     * @brief Construct Camera instance from ICamera strong pointer.
     * @param node the object pointed by interface is kept alive
     */
    explicit Camera(const INode::Ptr& node)
    {
        Initialize(interface_pointer_cast<META_NS::IObject>(node));
    }

    /**
     * @brief Construct Camera instance from ICamera strong pointer.
     * @param node the object pointed by interface is kept alive
     */
    explicit Camera(const ICamera::Ptr& node)
    {
        Initialize(interface_pointer_cast<META_NS::IObject>(node));
    }

    /**
     * @brief Gets OnLoaded event reference from INode-interface
     * @return INode::OnLoaded
     */
    auto OnLoaded()
    {
        return META_API_CACHED_INTERFACE(Node)->OnLoaded();
    }

    /**
     * @brief Runs a callback once the camera is loaded. If camera is already initialized, callback will not run.
     * @param callback Code to run, if strong reference is passed, it will keep the instance alive
     *                 causing engine to report memory leak on application exit.
     * @return reference to this instance of Camera.
     */
    template<class Callback>
    auto OnLoaded(Callback&& callback)
    {
        OnLoaded()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>(callback));
        return *this;
    }

    void AddMultiviewCamera(Camera camera)
    {
        META_API_CACHED_INTERFACE(Camera)->AddMultiviewCamera(camera);
    }

    void RemoveMultiviewCamera(Camera camera)
    {
        META_API_CACHED_INTERFACE(Camera)->RemoveMultiviewCamera(camera);
    }
};

SCENE_END_NAMESPACE()

#endif // SCENEPLUGINAPI_CAMERA_H
