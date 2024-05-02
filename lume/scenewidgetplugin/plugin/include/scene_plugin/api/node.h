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

#ifndef SCENEPLUGINAPI_NODE_H
#define SCENEPLUGINAPI_NODE_H

#include <scene_plugin/api/node_uid.h>
#include <scene_plugin/interface/intf_environment.h>
#include <scene_plugin/interface/intf_mesh.h>
#include <scene_plugin/interface/intf_scene.h>

#include <meta/api/internal/object_api.h>
SCENE_BEGIN_NAMESPACE()

/**
 * @brief Node class wraps INode interface. It keeps the referenced object alive using strong ref.
 *        The construction of the object is asynchronous, the properties of the engine may not be available
 *        right after the object instantiation, but OnLoaded() event can be used to observe the state changes.
 */
class Node final : public META_NS::Internal::ObjectInterfaceAPI<Node, ClassId::Node> {
    META_API(Node)
    META_API_OBJECT_CONVERTIBLE(INode)
    META_API_CACHE_INTERFACE(INode, Node)
public:
    META_API_INTERFACE_PROPERTY_CACHED(Node, Name, BASE_NS::string)
    META_API_INTERFACE_PROPERTY_CACHED(Node, Position, BASE_NS::Math::Vec3)
    META_API_INTERFACE_PROPERTY_CACHED(Node, Scale, BASE_NS::Math::Vec3)
    META_API_INTERFACE_PROPERTY_CACHED(Node, Rotation, BASE_NS::Math::Quat)
    META_API_INTERFACE_PROPERTY_CACHED(Node, Visible, bool)
    META_API_INTERFACE_PROPERTY_CACHED(Node, LayerMask, uint64_t)
    META_API_INTERFACE_PROPERTY_CACHED(Node, LocalMatrix, BASE_NS::Math::Mat4X4)

    /**
     * @brief Construct Node instance from INode strong pointer.
     * @param node the object pointed by interface is kept alive
     */
    explicit Node(const INode::Ptr& node)
    {
        Initialize(interface_pointer_cast<META_NS::IObject>(node));
    }

    /**
     * @brief Get a mesh attached to node instance
     * @return A mesh attached to node.
     */
    IMesh::Ptr GetMesh()
    {
        if (auto impl = META_API_CACHED_INTERFACE(Node)) {
            return impl->GetMesh();
        }
        return IMesh::Ptr {};
    }

    /**
     * @brief Set a mesh to node instance
     * @param A mesh to attach to node.
     */
    void SetMesh(IMesh::Ptr mesh)
    {
        if (auto impl = META_API_CACHED_INTERFACE(Node)) {
            impl->SetMesh(mesh);
        }
    }

    BASE_NS::Math::Mat4X4 GetGlobalTransform() const
    {
        if (auto impl = META_API_CACHED_INTERFACE(Node)) {
            return impl->GetGlobalTransform();
        }
        return BASE_NS::Math::IDENTITY_4X4;
    }

    void SetGlobalTransform(const BASE_NS::Math::Mat4X4& mat)
    {
        if (auto impl = META_API_CACHED_INTERFACE(Node)) {
            return impl->SetGlobalTransform(mat);
        }
    }

    /**
     * @brief Gets OnLoaded event reference from INode-interface
     * @return INode::OnLoaded
     * @return
     */
    auto OnLoaded()
    {
        return META_API_CACHED_INTERFACE(Node)->OnLoaded();
    }

    /**
     * @brief Runs a callback once the node is loaded on engine. If node is already initialized, callback will not run.
     * @param callback Code to run, if strong reference is passed, it will keep the instance alive
     *                 causing engine to report memory leak on application exit.
     * @return reference to this instance of Mesh.
     */
    template<class Callback>
    auto OnLoaded(Callback&& callback)
    {
        OnLoaded()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>(callback));
        return *this;
    }

    /**
     * @brief Returns scene this node is attached into.
     * @return Scene object this node is attached to.
     */
    IScene::Ptr GetScene() const
    {
        if (auto impl = META_API_CACHED_INTERFACE(Node)) {
            return impl->GetScene();
        }

        return {};
    }
};

SCENE_END_NAMESPACE()

#endif // SCENEPLUGINAPI_NODE_H
