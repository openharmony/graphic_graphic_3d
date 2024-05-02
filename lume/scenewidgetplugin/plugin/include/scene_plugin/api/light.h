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

#ifndef SCENEPLUGINAPI_LIGHT_H
#define SCENEPLUGINAPI_LIGHT_H

#include <scene_plugin/api/light_uid.h>
#include <scene_plugin/interface/intf_light.h>

#include <meta/api/internal/object_api.h>
SCENE_BEGIN_NAMESPACE()

/**
 * @brief Light class wraps ILight interface. It keeps the referenced object alive using strong ref.
 *        The construction of the object is asynchronous, the properties of the engine may not be available
 *        right after the object instantiation, but OnLoaded() event can be used to observe the state changes.
 */
class Light final : public META_NS::Internal::ObjectInterfaceAPI<Light, ClassId::Light> {
    META_API(Light)
    META_API_OBJECT_CONVERTIBLE(ILight)
    META_API_CACHE_INTERFACE(ILight, Light)
    META_API_OBJECT_CONVERTIBLE(INode)
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

public:
    META_API_INTERFACE_PROPERTY_CACHED(Light, Color, BASE_NS::Color)
    META_API_INTERFACE_PROPERTY_CACHED(Light, Intensity, float)
    META_API_INTERFACE_PROPERTY_CACHED(Light, NearPlane, float)
    META_API_INTERFACE_PROPERTY_CACHED(Light, ShadowEnabled, bool)
    META_API_INTERFACE_PROPERTY_CACHED(Light, ShadowStrength, float)
    META_API_INTERFACE_PROPERTY_CACHED(Light, ShadowDepthBias, float)
    META_API_INTERFACE_PROPERTY_CACHED(Light, ShadowNormalBias, float)
    META_API_INTERFACE_PROPERTY_CACHED(Light, SpotInnerAngle, float)
    META_API_INTERFACE_PROPERTY_CACHED(Light, SpotOuterAngle, float)
    META_API_INTERFACE_PROPERTY_CACHED(Light, Type, uint8_t)
    META_API_INTERFACE_PROPERTY_CACHED(Light, AdditionalFactor, BASE_NS::Math::Vec4)
    META_API_INTERFACE_PROPERTY_CACHED(Light, LightLayerMask, uint64_t)
    META_API_INTERFACE_PROPERTY_CACHED(Light, ShadowLayerMask, uint64_t)

    /**
     * @brief Construct Light instance from INode strong pointer.
     * @param node the object pointed by interface is kept alive
     */
    explicit Light(const INode::Ptr& node)
    {
        Initialize(interface_pointer_cast<META_NS::IObject>(node));
    }

    /**
     * @brief Construct Light instance from ILight strong pointer.
     * @param node the object pointed by interface is kept alive
     */
    explicit Light(const ILight::Ptr& node)
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
     * @brief Runs a callback once the light is loaded. If light is already initialized, callback will not run.
     * @param callback Code to run, if strong reference is passed, it will keep the instance alive
     *                 causing engine to report memory leak on application exit.
     * @return reference to this instance of Light.
     */
    template<class Callback>
    auto OnLoaded(Callback&& callback)
    {
        OnLoaded()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>(callback));
        return *this;
    }
};

SCENE_END_NAMESPACE()

#endif // SCENEPLUGINAPI_LIGHT_H
