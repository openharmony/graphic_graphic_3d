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
#ifndef SCENEPLUGIN_INTF_NODE_H
#define SCENEPLUGIN_INTF_NODE_H

#include <scene_plugin/namespace.h>

#include <base/containers/vector.h>

#include <meta/api/animation/animation.h>
#include <meta/api/make_callback.h>
#include <meta/base/types.h>
#include <meta/interface/intf_container.h>
#include <meta/interface/property/property.h>

SCENE_BEGIN_NAMESPACE()

REGISTER_INTERFACE(IPendingRequest, "59856b13-8674-48f0-9b34-9b1093472958")
template<typename T>
class IPendingRequest : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IPendingRequest, InterfaceId::IPendingRequest)
public:
    META_EVENT(META_NS::IOnChanged, OnReady)
    virtual const BASE_NS::vector<T>& GetResults() const = 0;
};

using IPickingResult = IPendingRequest<BASE_NS::Math::Vec3>;

class IMesh;
class IScene;
class IMultiMeshProxy;

enum class NodeState { DETACHED = 0, ATTACHED = 1 };

// Implemented by SCENE_NS::ClassId::Camera, SCENE_NS::ClassId::Light, SCENE_NS::ClassId::Model
REGISTER_INTERFACE(INode, "e9770092-67fe-42ad-8703-a9be44250841")
class INode : public META_NS::INamed {
    META_INTERFACE(META_NS::INamed, INode, InterfaceId::INode)
public:
    /**
     * @brief Path of this object on 3D scene (mirrored to META_NS::IContainer, also),
     * @return property pointer
     */
    META_READONLY_PROPERTY(BASE_NS::string, Path)

    /**
     * @brief If the node was created from a prefab, the uri is stored here, empty otherwise
     * @return property pointer
     */

    /**
     * @brief Transform of the node Position
     * @return property pointer
     */
    META_PROPERTY(BASE_NS::Math::Vec3, Position)

    /**
     * @brief Transform of the node Scale
     * @return property pointer
     */
    META_PROPERTY(BASE_NS::Math::Vec3, Scale)

    /**
     * @brief Transform of the node Rotation
     * @return property pointer
     */
    META_PROPERTY(BASE_NS::Math::Quat, Rotation)

    /**
     * @brief Enable node in 3D scene.
     * @return property pointer
     */
    META_PROPERTY(bool, Visible)

    /**
     * @brief Bitmask that controls if the node are rendered. ICamera::LayerMask defines the
     *        bits that need to be active, while nodes may have different combinations of bits
     * @return property pointer
     */
    META_PROPERTY(uint64_t, LayerMask)

    enum NodeStatus {
        /** Instance exists, but the properties are not set even locally*/
        NODE_STATUS_UNINITIALIZED = 0,
        /** The local properties are set*/
        NODE_STATUS_CONNECTING = 1,
        /** The node is bound to 3D scene*/
        NODE_STATUS_CONNECTED = 2,
        /** The respective 3D element was not found
         *  Node properties are preserved but not proxied to 3D scene */
        NODE_STATUS_DISCONNECTED = 3,
        /** The node and all its children are bound to 3D scene*/
        NODE_STATUS_FULLY_CONNECTED = 4
    };
    /**
     * @brief Node status.
     * @return property pointer
     */
    META_READONLY_PROPERTY(uint8_t, Status)

    /**
     * @brief Nodes local matrix on 3D scene.
     * @return Pointer to the property.
     */
    META_PROPERTY(BASE_NS::Math::Mat4X4, LocalMatrix)

    /**
     * @brief The mesh attached to node, if any.
     * @return Pointer to the property.
     */
    META_PROPERTY(BASE_NS::shared_ptr<IMesh>, Mesh)

    /**
     * @brief Event that is invoked when the backing resources for the node have been loaded
     * @return Reference to the event.
     */
    META_EVENT(META_NS::IOnChanged, OnLoaded)

    /**
     * @brief Event that is invoked when all the children of this node have been bound to 3d scene
     * @return Reference to the event.
     */
    META_EVENT(META_NS::IOnChanged, OnBound)

    /**
     * @brief Deprecated, use Mesh() property. Set the given mesh to this node.
     * @param the mesh to set.
     */
    virtual void SetMesh(BASE_NS::shared_ptr<IMesh> mesh) = 0;

    /**
     * @brief Deprecated, use Mesh() property. Returns a mesh from the scene.
     * @returns always an instance, even the engine node does not no have mesh attached.
     */
    virtual BASE_NS::shared_ptr<IMesh> GetMesh() const = 0;

    virtual BASE_NS::shared_ptr<IMultiMeshProxy> CreateMeshProxy(size_t count, BASE_NS::shared_ptr<IMesh> mesh) = 0;
    virtual void SetMultiMeshProxy(BASE_NS::shared_ptr<IMultiMeshProxy> multimesh) = 0;
    virtual BASE_NS::shared_ptr<IMultiMeshProxy> GetMultiMeshProxy() const = 0;

    /**
     * @brief Set node parent on the container, reflected to 3D scene.
     * @param node Node to parent.
     * @param parent New parent of the node.
     * @return true on success.
     */
    static bool SetParent(const INode::Ptr& node, const META_NS::IObject::Ptr& parent)
    {
        if (auto container = interface_pointer_cast<META_NS::IContainer>(parent); node && container) {
            return container->Add(node);
        }
        return false;
    }

    // ToDo: Should perhaps be a property
    enum BuildBehavior {
        // Do not build children automatically
        NODE_BUILD_CHILDREN_NO_BUILD = 0,
        // Build children gradually asynchronously
        NODE_BUILD_CHILDREN_GRADUAL = 1,
        // Build all node descendants on one go
        NODE_BUILD_CHILDREN_ALL = 2,
        // Build only direct descendants.
        // (does not recurse in to children of children, to access them you need to call buildchildren on the children first.)
        NODE_BUILD_ONLY_DIRECT_CHILDREN = 3
    };
    /**
     * @brief Instantiate children nodes, i.e. mirror all immediate children from 3D system.
     * @param options controls if the node builds also descendants of immediate children.
     * @return True if scene is ready to process possible children.
     */
    virtual bool BuildChildren(BuildBehavior options) = 0;

    /** @brief Get the scene attached to this node.
     */
    virtual BASE_NS::shared_ptr<IScene> GetScene() const = 0;

    /**
     * @brief Add handler for OnLoaded event
     * @param callback
     * @return reference to this instance.
     */
    template<class Callback>
    auto& OnLoaded(Callback&& callback)
    {
        OnLoaded()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>(callback));
        return *this;
    }

    /**
     * @brief Run a callback once object is loaded. if object has been already loaded, runs the callback synchronously.
     *        Keeps the instance alive using strong ref if callback attaches one and removes the reference on callback.
     * @param callback
     * @return true if object is already loaded.
     */
    template<class Callback>
    bool OnLoadedSingleShot(Callback&& callback)
    {
        if (auto status = META_NS::GetValue(Status());
            (status == NODE_STATUS_FULLY_CONNECTED || status == NODE_STATUS_CONNECTED)) {
            callback();
            return true;
        }
        auto token = OnLoaded()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>(callback));
        OnLoaded()->AddHandler(
            META_NS::MakeCallback<META_NS::IOnChanged>([token, this]() { OnLoaded()->RemoveHandler(token); }));
        return false;
    }

    /**
     * @brief Like above, the function makes a strong assumption that either the callback
     *        contains a strong pointer to the object itself, or lifetime is otherwise guaranteed.
     *        ToDo: should make that more explicit
     * @param self pointer to this instance
     * @param callback that runs either immediately if the object is ready or later if the construction of node or its
     * children is not finished
     * @return true if object is already fully loaded.
     */
    template<class Callback>
    bool OnReady(Callback&& callback)
    {
        if (META_NS::GetValue(Status()) == NODE_STATUS_FULLY_CONNECTED) {
            callback();
            return true;
        }

        auto token = OnBound()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>(callback));
        OnBound()->AddHandler(
            META_NS::MakeCallback<META_NS::IOnChanged>([token, this]() { OnBound()->RemoveHandler(token); }));
        return false;
    }

    /**
     * @brief Calculates the global matrix based on node hierarchy and local matrices.
     *        The local matrix reflects values from transform component, so there can be
     *        some delay reading back values set using local transformations.
     * @return Global matrix of Node transformations.
     */
    virtual BASE_NS::Math::Mat4X4 GetGlobalTransform() const = 0;

    /**
     * @brief Calculates the parents global matrix based on node hierarchy and local matrices
     *        and subtracts that from the given matrix to store it as local matrix.
     * @param matrix Global matrix of Node transformations.
     */
    virtual void SetGlobalTransform(const BASE_NS::Math::Mat4X4& matrix) = 0;

    virtual void AttachToHierarchy() = 0;

    virtual void DetachFromHierarchy() = 0;

    virtual NodeState GetAttachedState() const = 0;

    /**
     * Calculates a world space AABB for this node and optionally all of it's children recursively. Uses the world
     * matrix component for the calculations i.e. the values are only valid after the ECS has updated all the systems
     * that manipulate the world matrix.
     */
    virtual IPickingResult::Ptr GetWorldMatrixComponentAABB(bool isRecursive) const = 0;

    /**
     * Calculates a world space AABB for this node and optionally all of it's children recursively. Uses only the
     * transform component for the calculations. This way the call will also give valid results even when the ECS has
     * not been updated. Does not take skinning or animations into account.
     */
    virtual IPickingResult::Ptr GetTransformComponentAABB(bool isRecursive) const = 0;
};

struct NodeDistance {
    INode::Ptr node;
    float distance;
};

using IRayCastResult = IPendingRequest<NodeDistance>;

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::INode::WeakPtr);
META_TYPE(SCENE_NS::INode::Ptr);

#endif // SCENEPLUGIN_INTF_NODE_H
