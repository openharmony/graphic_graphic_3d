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

#ifndef SCENEPLUGINAPI_ENVIRONMENT_H
#define SCENEPLUGINAPI_ENVIRONMENT_H

#include <scene_plugin/api/environment_uid.h>
#include <scene_plugin/interface/intf_environment.h>
#include <scene_plugin/interface/intf_node.h>

#include <meta/api/internal/object_api.h>
SCENE_BEGIN_NAMESPACE()

class Environment final : public META_NS::Internal::ObjectInterfaceAPI<Environment, ClassId::Environment> {
    META_API(Environment)
    API_OBJECT_CONVERTIBLE(IEnvironment)
    API_CACHE_INTERFACE(IEnvironment, Environment)
    API_OBJECT_CONVERTIBLE(INode)
    API_CACHE_INTERFACE(INode, Node)

public:
    API_INTERFACE_PROPERTY_CACHED(Node, Name, BASE_NS::string)
    API_INTERFACE_PROPERTY_CACHED(Node, Position, BASE_NS::Math::Vec3)
    API_INTERFACE_PROPERTY_CACHED(Node, Scale, BASE_NS::Math::Vec3)
    API_INTERFACE_PROPERTY_CACHED(Node, Rotation, BASE_NS::Math::Quat)
    API_INTERFACE_PROPERTY_CACHED(Node, Visible, bool)
    API_INTERFACE_PROPERTY_CACHED(Node, LayerMask, uint64_t)
    API_INTERFACE_PROPERTY_CACHED(Node, LocalMatrix, BASE_NS::Math::Mat4X4)

    API_INTERFACE_PROPERTY_CACHED(Environment, Background, IEnvironment::BackgroundType)
    API_INTERFACE_PROPERTY_CACHED(Environment, IndirectDiffuseFactor, BASE_NS::Math::Vec4)
    API_INTERFACE_PROPERTY_CACHED(Environment, IndirectSpecularFactor, BASE_NS::Math::Vec4)
    API_INTERFACE_PROPERTY_CACHED(Environment, EnvMapFactor, BASE_NS::Math::Vec4)
    API_INTERFACE_PROPERTY_CACHED(Environment, RadianceImage, IBitmap::Ptr)
    API_INTERFACE_PROPERTY_CACHED(Environment, RadianceCubemapMipCount, uint32_t)
    API_INTERFACE_PROPERTY_CACHED(Environment, EnvironmentImage, IBitmap::Ptr)
    API_INTERFACE_PROPERTY_CACHED(Environment, EnvironmentMapLodLevel, float)
    API_INTERFACE_PROPERTY_CACHED(Environment, EnvironmentRotation, BASE_NS::Math::Quat)
    API_INTERFACE_PROPERTY_CACHED(Environment, AdditionalFactor, BASE_NS::Math::Vec4)
    API_INTERFACE_PROPERTY_CACHED(Environment, ShaderHandle, uint64_t)

    /**
     * @brief Construct Environment instance from INode strong pointer.
     * @param node the object pointed by interface is kept alive
     */
    explicit Environment(const INode::Ptr& node)
    {
        Initialize(interface_pointer_cast<META_NS::IObject>(node));
    }

    /**
     * @brief Construct Environment instance from IEnvironment strong pointer.
     * @param node the object pointed by interface is kept alive
     */
    explicit Environment(const IEnvironment::Ptr& node)
    {
        Initialize(interface_pointer_cast<META_NS::IObject>(node));
    }
};
SCENE_END_NAMESPACE()

#endif // SCENEPLUGINAPI_ENVIRONMENT_H
