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

#ifndef SCENE_API_NODE_H
#define SCENE_API_NODE_H

#include <scene/api/post_process.h>
#include <scene/ext/intf_component.h>
#include <scene/interface/intf_camera.h>
#include <scene/interface/intf_environment.h>
#include <scene/interface/intf_layer.h>
#include <scene/interface/intf_light.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_node_import.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_text.h>

#include <meta/api/util.h>

SCENE_BEGIN_NAMESPACE()

namespace Internal {
template<const META_NS::AsyncCallType& CallType, typename SyncReturnType, typename FutureType>
constexpr auto UnwrapFuture(FutureType&& f)
{
    if constexpr (CallType.async) {
        return BASE_NS::forward<FutureType>(f);
    } else {
        return SyncReturnType(f.GetResult());
    }
}
template<typename Type>
bool CheckName(const Type& target, BASE_NS::string_view name) noexcept
{
    if constexpr (BASE_NS::is_same_v<Type, META_NS::IObject::Ptr>) {
        return target && target->GetName() == name;
    } else {
        auto o = interface_cast<META_NS::IObject>(target);
        return o && o->GetName() == name;
    }
}
template<typename Result, typename Input>
BASE_NS::shared_ptr<Result> FindFromContainer(
    const BASE_NS::vector<BASE_NS::shared_ptr<Input>>& container, BASE_NS::string_view name) noexcept
{
    for (const auto& c : container) {
        if (CheckName<BASE_NS::shared_ptr<Input>>(c, name)) {
            return interface_pointer_cast<Result>(c);
        }
    }
    return nullptr;
}

} // namespace Internal

class Layer : public META_NS::Named {
public:
    META_INTERFACE_OBJECT(Layer, META_NS::Named, ILayer)
    /// @see ILayer::LayerMask
    META_INTERFACE_OBJECT_PROPERTY(uint64_t, LayerMask)
};

/// Wrapper for generic Scene nodes which implement SCENE_NS::INode.
class Node : public Layer {
public:
    META_INTERFACE_OBJECT(Node, Layer, INode)
    /// @see ITransform::Position
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::Math::Vec3, Position)
    /// @see ITransform::Scale
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::Math::Vec3, Scale)
    /// @see ITransform::Rotation
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::Math::Quat, Rotation)
    /// @see INode::Enabled
    META_INTERFACE_OBJECT_PROPERTY(bool, Enabled)
    /// @see INode::GetName
    auto GetName() const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetName());
    }
    /// @see INode::GetPath
    META_API_ASYNC auto GetPath() const
    {
        return META_INTERFACE_OBJECT_ASYNC_CALL_PTR(BASE_NS::string, GetPath());
    }
    /// @see INode::GetParent
    META_API_ASYNC auto GetParent() const
    {
        return META_INTERFACE_OBJECT_ASYNC_CALL_PTR(Node, GetParent());
    }
    /// @see INode::GetChildren
    META_API_ASYNC auto GetChildren() const
    {
        return META_INTERFACE_OBJECT_ASYNC_CALL_PTR(META_NS::Internal::ArrayCast<Node>, GetChildren());
    }
    /// @see INode::AddChild
    META_API_ASYNC auto AddChild(const INode::Ptr& child, size_t index = -1)
    {
        return META_INTERFACE_OBJECT_ASYNC_CALL_PTR(bool, AddChild(child, index));
    }
    /// @see INode::RemoveChild
    META_API_ASYNC auto RemoveChild(const INode::Ptr& child)
    {
        return META_INTERFACE_OBJECT_ASYNC_CALL_PTR(bool, RemoveChild(child));
    }
    /// @see INode::GetScene
    auto GetScene() const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetScene());
    }
    /// @see INode::GetUniqueChildName
    META_API_ASYNC auto GetUniqueChildName(BASE_NS::string_view name) const
    {
        return META_INTERFACE_OBJECT_ASYNC_CALL_PTR(BASE_NS::string, GetUniqueChildName(name));
    }
    /**
     * @brief Finds all occurrences of a child node with given name from this node's hierarchy.
     * @note  If the full path is known, it is more efficient to call Scene::GetNode
     * @param name Name of the node to find.
     * @param maxCOunt Maximum number of nodes to return.
     * @param traversalType Can be used to control how the scene is traversed, or to limit finding only immediate
     *        children by specifying TraversalType::NO_HIERARCHY.
     * @return The first node matching the parameters or an invalid node in case of failure.
     */
    META_API_ASYNC auto FindNodes(BASE_NS::string_view name, size_t maxCount,
        META_NS::TraversalType traversalType = META_NS::TraversalType::FULL_HIERARCHY) const
    {
        auto f = CallPtr<INode>([=](auto& n) {
            auto scene = n.GetScene();
            ::SCENE_NS::IScene::FindNamedNodeParams params;
            params.name = name;
            params.maxCount = maxCount;
            params.root = GetPtr<INode>();
            params.traversalType = traversalType;
            return scene->FindNamedNodes(params);
        });
        if constexpr (CallType.async) {
            return f;
        } else {
            return META_NS::Internal::ArrayCast<Node>(f.GetResult());
        }
    }
    /**
     * @brief Finds the first occurrence of a child node with given name from this node's hierarchy.
     * @note  If the full path is known, it is more efficient to call Scene::GetNode
     * @param name Name of the node to find.
     * @param traversalType Can be used to control how the scene is traversed, or to limit finding only immediate
     *        children by specifying TraversalType::NO_HIERARCHY.
     * @return The first node matching the parameters or an invalid node in case of failure.
     */
    META_API_ASYNC auto FindNode(
        BASE_NS::string_view name, META_NS::TraversalType traversalType = META_NS::TraversalType::FULL_HIERARCHY) const
    {
        auto f = CallPtr<INode>([=](auto& n) {
            auto scene = n.GetScene();
            ::SCENE_NS::IScene::FindNamedNodeParams params;
            params.name = name;
            params.maxCount = 1;
            params.root = GetPtr<INode>();
            params.traversalType = traversalType;
            return scene ? scene->FindNamedNode(params) : decltype(scene->FindNamedNode(params)) {};
        });
        return Internal::UnwrapFuture<CallType, Node>(BASE_NS::move(f));
    }
    /**
     * @brief Imports a Scene into this scene under this node.
     * @param scene The scene to import.
     * @param name The name which should be given to the root node of the imported scene, placed as a child of this
     *        node.
     */
    META_API_ASYNC auto ImportScene(const IScene::Ptr& scene, BASE_NS::string_view name)
    {
        auto f = CallPtr<INodeImport>([=](auto& ni) { return ni.ImportChildScene(scene, name); });
        return Internal::UnwrapFuture<CallType, Node>(BASE_NS::move(f));
    }
    /**
     * @brief Imports a Scene from a resource under this node.
     * @param uri Uri of the scene resource to import (.gltf or .scene)
     * @param name The name which should be given to the root node of the imported scene, placed as a child of this
     *        node.
     */
    META_API_ASYNC auto ImportScene(BASE_NS::string_view uri, BASE_NS::string_view name)
    {
        auto f = CallPtr<INodeImport>([=](auto& ni) { return ni.ImportChildScene(uri, name); });
        return Internal::UnwrapFuture<CallType, Node>(BASE_NS::move(f));
    }
};

/// Wrapper for Scene camera nodes which implement SCENE_NS::ICamera.
class Camera : public Node {
public:
    META_INTERFACE_OBJECT(Camera, Node, ICamera)
    /// @see ICamera::FoV
    META_INTERFACE_OBJECT_PROPERTY(float, FoV)
    /// @see ICamera::AspectRatio
    META_INTERFACE_OBJECT_PROPERTY(float, AspectRatio)
    /// @see ICamera::NearPlane
    META_INTERFACE_OBJECT_PROPERTY(float, NearPlane)
    /// @see ICamera::FarPlane
    META_INTERFACE_OBJECT_PROPERTY(float, FarPlane)
    /// @see ICamera::XMagnification
    META_INTERFACE_OBJECT_PROPERTY(float, XMagnification)
    /// @see ICamera::YMagnification
    META_INTERFACE_OBJECT_PROPERTY(float, YMagnification)
    /// @see ICamera::XOffset
    META_INTERFACE_OBJECT_PROPERTY(float, XOffset)
    /// @see ICamera::YOffset
    META_INTERFACE_OBJECT_PROPERTY(float, YOffset)
    /// @see ICamera::Projection
    META_INTERFACE_OBJECT_PROPERTY(CameraProjection, Projection)
    /// @see ICamera::Culling
    META_INTERFACE_OBJECT_PROPERTY(CameraCulling, Culling)
    /// @see ICamera::RenderingPipeline
    META_INTERFACE_OBJECT_PROPERTY(CameraPipeline, RenderingPipeline)
    /// @see ICamera::MSAASampleCount
    META_INTERFACE_OBJECT_PROPERTY(CameraSampleCount, MSAASampleCount)
    /// @see ICamera::SceneFlags
    META_INTERFACE_OBJECT_PROPERTY(uint32_t, SceneFlags)
    /// @see ICamera::PipelineFlags
    META_INTERFACE_OBJECT_PROPERTY(uint32_t, PipelineFlags)
    /// @see ICamera::Viewport
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::Math::Vec4, Viewport)
    /// @see ICamera::Scissor
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::Math::Vec4, Scissor)
    /// @see ICamera::RenderTargetSize
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::Math::UVec2, RenderTargetSize)
    /// @see ICamera::ClearColor
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::Math::Vec4, ClearColor)
    /// Set ClearColor from BASE_NS::Color.
    auto& SetClearColor(const BASE_NS::Color& color)
    {
        SetClearColor(BASE_NS::Math::Vec4(color.r, color.g, color.b, color.a));
        return *this;
    }
    /// @see ICamera::ClearDepth
    META_INTERFACE_OBJECT_PROPERTY(float, ClearDepth)
    /// @see ICamera::LayerMask
    META_INTERFACE_OBJECT_PROPERTY(uint64_t, CameraLayerMask)
    /// @see ICamera::ColorTargetCustomization
    META_INTERFACE_OBJECT_ARRAY_PROPERTY(ColorFormat, ColorTargetCustomization, ColorTargetCustomization)
    /// @see ICamera::CustomProjectionMatrix
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::Math::Mat4X4, CustomProjectionMatrix)
    /// @see ICamera::PostProcess
    META_INTERFACE_OBJECT_PROPERTY(SCENE_NS::PostProcess, PostProcess)
    /// @see ICamera::DownsamplePercentage
    META_INTERFACE_OBJECT_PROPERTY(float, DownsamplePercentage)
    /// @see ICamera::SetActive
    META_API_ASYNC auto SetActive(bool active)
    {
        return META_INTERFACE_OBJECT_ASYNC_CALL_PTR(bool, SetActive(active));
    }
    /// @see ICamera::IsActive
    bool IsActive() const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(IsActive());
    }
    /// @see ICamera::SetRenderTarget
    META_API_ASYNC auto SetRenderTarget(const IRenderTarget::Ptr& target)
    {
        return META_INTERFACE_OBJECT_ASYNC_CALL_PTR(bool, SetRenderTarget(target));
    }
    /// @see ICamera::SendInputEvent
    void SendInputEvent(SCENE_NS::PointerEvent& event)
    {
        META_INTERFACE_OBJECT_CALL_PTR(SendInputEvent(event));
    }
    /// Helper method for sending pointer down event for a single pointer
    void SendPointerDown(uint32_t pointerId, const BASE_NS::Math::Vec2& position, META_NS::TimeSpan time = {})
    {
        SendPointerEvent(pointerId, SCENE_NS::PointerEvent::PointerState::POINTER_DOWN, position, time);
    }
    /// Helper method for sending pointer up event for a single pointer
    void SendPointerUp(uint32_t pointerId, const BASE_NS::Math::Vec2& position, META_NS::TimeSpan time = {})
    {
        SendPointerEvent(pointerId, SCENE_NS::PointerEvent::PointerState::POINTER_UP, position, time);
    }
    /// @see ICameraEffect::Effects
    META_NS::ArrayProperty<IEffect::Ptr> Effects()
    {
        return CallPtr<ICameraEffect>([](auto& p) { return p.Effects(); });
    }
    /// Return effects associated with the camera object
    BASE_NS::vector<IEffect::Ptr> GetEffects() const
    {
        return CallPtr<ICameraEffect>([](auto& p) { return p.Effects()->GetValue(); });
    }
    /// Set the list of effects to apply on the camera output.
    void SetEffects(BASE_NS::array_view<const IEffect::Ptr> effects)
    {
        CallPtr<ICameraEffect>([&](auto& p) { p.Effects()->SetValue(effects); });
    }

private:
    void SendPointerEvent(uint32_t pointerId, SCENE_NS::PointerEvent::PointerState state,
        const BASE_NS::Math::Vec2& position, META_NS::TimeSpan time = {})
    {
        SCENE_NS::PointerEvent event;
        event.time = time;
        event.pointers = { { pointerId, position, state } };
        SendInputEvent(event);
    }
};

/// Wrapper for Scene light nodes which implement SCENE_NS::ILight.
class Light : public Node {
public:
    META_INTERFACE_OBJECT(Light, Node, ILight)
    /// @see ILight::Color
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::Color, Color)
    /// @see ILight::Intensity
    META_INTERFACE_OBJECT_PROPERTY(float, Intensity)
    /// @see ILight::NearPlane
    META_INTERFACE_OBJECT_PROPERTY(float, NearPlane)
    /// @see ILight::ShadowEnabled
    META_INTERFACE_OBJECT_PROPERTY(bool, ShadowEnabled)
    /// @see ILight::ShadowStrength
    META_INTERFACE_OBJECT_PROPERTY(float, ShadowStrength)
    /// @see ILight::ShadowDepthBias
    META_INTERFACE_OBJECT_PROPERTY(float, ShadowDepthBias)
    /// @see ILight::ShadowNormalBias
    META_INTERFACE_OBJECT_PROPERTY(float, ShadowNormalBias)
    /// @see ILight::SpotInnerAngle
    META_INTERFACE_OBJECT_PROPERTY(float, SpotInnerAngle)
    /// @see ILight::SpotOuterAngle
    META_INTERFACE_OBJECT_PROPERTY(float, SpotOuterAngle)
    /// @see ILight::Type
    META_INTERFACE_OBJECT_PROPERTY(LightType, Type)
    /// @see ILight::AdditionalFactor
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::Math::Vec4, AdditionalFactor)
    /// @see ILight::LightLayerMask
    META_INTERFACE_OBJECT_PROPERTY(uint64_t, LightLayerMask)
    /// @see ILight::ShadowLayerMask
    META_INTERFACE_OBJECT_PROPERTY(uint64_t, ShadowLayerMask)
};

/// Wrapper for Scene geometry nodes which implement SCENE_NS::IMesh.
class Geometry : public Node {
public:
    META_INTERFACE_OBJECT(Geometry, Node, IMesh)
    /// @see IMesh::AABBMin
    META_INTERFACE_OBJECT_READONLY_PROPERTY(BASE_NS::Math::Vec3, AABBMin)
    /// @see IMesh::AABBMax
    META_INTERFACE_OBJECT_READONLY_PROPERTY(BASE_NS::Math::Vec3, AABBMax)
    /// @see IMorphAccess::Morpher
    auto GetMorpher() const
    {
        return Morpher(META_NS::GetValue(CallPtr<IMorphAccess>([](auto& p) { return p.Morpher(); })));
    }
    /// @see IMesh::Mesh
    META_API_ASYNC auto GetMesh() const
    {
        auto f = CallPtr<IMeshAccess>([](const auto& ma) { return ma.GetMesh(); });
        return Internal::UnwrapFuture<CallType, Mesh>(BASE_NS::move(f));
    }
};

/// Wrapper for Scene text nodes which implement SCENE_NS::IText.
class Text3D : public Node {
public:
    META_INTERFACE_OBJECT(Text3D, Node, IText)
    /// @see IText::Test
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::string, Text)
    /// @see IText::FontFamily
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::string, FontFamily)
    /// @see IText::FontStyle
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::string, FontStyle)
    /// @see IText::FontSize
    META_INTERFACE_OBJECT_PROPERTY(float, FontSize)
    /// @see IText::Font3DThickness
    META_INTERFACE_OBJECT_PROPERTY(float, Font3DThickness)
    /// @see IText::FontMethod
    META_INTERFACE_OBJECT_PROPERTY(SCENE_NS::FontMethod, FontMethod)
    /// @see IText::TextColor
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::Math::Vec4, TextColor)
    /// Set TextColor from BASE_NS::Color.
    auto& SetTextColor(const BASE_NS::Color& color)
    {
        SetTextColor(BASE_NS::Math::Vec4(color.r, color.g, color.b, color.a));
        return *this;
    }
};

SCENE_END_NAMESPACE()

#endif // SCENE_API_NODE_H
