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

#ifndef SCENE_INTERFACE_ISCENE_H
#define SCENE_INTERFACE_ISCENE_H

#include <scene/base/types.h>
#include <scene/ext/intf_component.h>
#include <scene/interface/intf_camera.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_render_configuration.h>
#include <scene/interface/resource/resource_group_bundle.h>

#include <render/intf_render_context.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/intf_iterable.h>

SCENE_BEGIN_NAMESPACE()

class IInternalScene;

class IScene : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IScene, "b61d5521-bfa9-4465-9e4d-65d4358ede26")
public:
    META_READONLY_PROPERTY(IRenderConfiguration::Ptr, RenderConfiguration)

public:
    /**
     * @brief Get root node for the scene
     * @return Root node
     */
    virtual Future<INode::Ptr> GetRootNode() const = 0;

    /**
     * @brief Add new node to the scene
     * @param path Path of the node
     * @param id   Type of the node, defaults to generic node
     * @return Newly created node
     */
    virtual Future<INode::Ptr> CreateNode(BASE_NS::string_view path, META_NS::ObjectId id = {}) = 0;
    virtual Future<INode::Ptr> CreateNode(
        const INode::ConstPtr& parent, BASE_NS::string_view name, META_NS::ObjectId id = {}) = 0;

    /**
     * @brief Find node in scene. This will find node with given type in scene and constructs node object for it
     * @notice Nodes are cached, the existing node object is returned if there is such
     * @param path Path of the node
     * @param id   Type of the node, if not given, the system tries to deduce the node type and falls back to generic
     * node
     * @return Found node or null if no such node exists or doesn't match the type
     */
    virtual Future<INode::Ptr> FindNode(BASE_NS::string_view path, META_NS::ObjectId id = {}) const = 0;

    /**
     * @brief The FindNamedNodeParams class defines the input parameters for FindNamedNode and FindNamedNodes.
     * @note FindNamedNode ignores maxCount.
     */
    struct FindNamedNodeParams {
        /// Name of the node to find.
        BASE_NS::string_view name;
        /// Maximum number of nodes to return. If 0, return all matching nodes.
        size_t maxCount {};
        /// Root node to start the search from. If {}, start from the scene's root node.
        INode::Ptr root;
        /// Type of the node, if not given, the system tries to deduce the node type and falls back to generic.
        META_NS::ObjectId id {};
        /// Defines how the the hierarchy should be iterated.
        META_NS::TraversalType traversalType { META_NS::TraversalType::FULL_HIERARCHY };
    };

    /**
     * @brief Find node in scene with a given name.
     * @param criteria Search criteria.
     * @return List of nodes in the scene that match the search criteria.
     */
    virtual Future<BASE_NS::vector<INode::Ptr>> FindNamedNodes(const FindNamedNodeParams& criteria) const = 0;

    /**
     * @brief Find first instance of a node with given name from the scene.
     * @return The first node in the scene that matches the search criteria.
     */
    Future<INode::Ptr> FindNamedNode(const FindNamedNodeParams& criteria) const
    {
        auto c = criteria;
        c.maxCount = 1;
        return FindNamedNodes(c).Then([](const BASE_NS::vector<INode::Ptr>& nodes) -> INode::Ptr {
            return nodes.empty() ? INode::Ptr {} : nodes.front();
        });
    }

    /**
     * @brief Remove the node (and possibly its children) from caches if it is the only instance and stop listening
     * notifications related to it. This does not affect ECS side.
     * @notice You have to move the last user instance of node to this function for it to have any effect.
     * @param node Node to release
     * @param recursive Should we check all child nodes recursive and try to release them
     * @return Number of node objects released
     */
    virtual Future<uint32_t> ReleaseNode(INode::Ptr&& node, bool recursive) = 0;

    /**
     * @brief Remove the node (and its children) from the scene (the underlying ecs)
     * @notice Do not use the node after this!
     */
    virtual Future<bool> RemoveNode(INode::Ptr&& node) = 0;

    /// Options for RemoveObject
    struct RemoveObjectOptions {
        /// If true, remove the object also from resource index. In this case the resource cannot be instantiated again
        /// after removal of the target object.
        bool removeFromResourceIndex { true };
    };
    /**
     * @brief Remove the object and its associated resources from the scene (the underlying ecs)
     * @note By default the associated resources will also be removed from the resource manager and cannot be
     *       re-instantiated afterwards. If the associated resources need to be instantiated later,
     *       set options.removeFromResourceIndex = false.
     * @note Do not use the object after this!
     */
    virtual Future<bool> RemoveObject(META_NS::IObject::Ptr&& object, const RemoveObjectOptions& options) = 0;

    /**
     * @brief Remove object from scene using default removal options.
     * @see RemoveObject
     */
    Future<bool> RemoveObject(META_NS::IObject::Ptr&& object)
    {
        return RemoveObject(BASE_NS::move(object), {});
    }

    /**
     * @brief Remove the animation from Scene.
     * @param animation The animation to remove.
     */
    inline Future<bool> RemoveAnimation(const META_NS::IAnimation::Ptr& animation)
    {
        return RemoveObject(interface_pointer_cast<META_NS::IObject>(animation));
    }

    template<class T>
    Future<typename T::Ptr> CreateNode(BASE_NS::string_view path, META_NS::ObjectId id = {})
    {
        return CreateNode(path, id).Then([](INode::Ptr d) { return interface_pointer_cast<T>(d); }, nullptr);
    }

    template<class T>
    Future<typename T::Ptr> FindNode(BASE_NS::string_view path, META_NS::ObjectId id = {}) const
    {
        return FindNode(path, id).Then([](INode::Ptr d) { return interface_pointer_cast<T>(d); }, nullptr);
    }

    virtual Future<META_NS::IObject::Ptr> CreateObject(META_NS::ObjectId id) = 0;

    template<class T>
    Future<typename T::Ptr> CreateObject(META_NS::ObjectId id)
    {
        return CreateObject(id).Then([](META_NS::IObject::Ptr d) { return interface_pointer_cast<T>(d); }, nullptr);
    }

    virtual Future<BASE_NS::vector<ICamera::Ptr>> GetCameras() const = 0;
    virtual Future<BASE_NS::vector<META_NS::IAnimation::Ptr>> GetAnimations() const = 0;

    virtual BASE_NS::shared_ptr<IInternalScene> GetInternalScene() const = 0;

    virtual void StartAutoUpdate(META_NS::TimeSpan interval) = 0;

    virtual Future<bool> SetRenderMode(RenderMode) = 0;
    virtual Future<RenderMode> GetRenderMode() const = 0;

    /**
     * @brief Returns a component from given node.
     * @param node Node to query the component from.
     * @param componentName Name of the component manager to query.
     * @return The component or nullptr if not found from the node.
     */
    virtual IComponent::Ptr GetComponent(const INode::Ptr& node, BASE_NS::string_view componentName) const = 0;
    /**
     * @brief Create a new component with given component manager name and attaches it to given node.
     * @param node The node for which the component should be created.
     * @param componentName Name of the component manager for the component.
     * @return If the name is valid, returns a component attached to node with given name.
     *         If the name is invalid, returns nullptr.
     */
    virtual Future<IComponent::Ptr> CreateComponent(const INode::Ptr& node, BASE_NS::string_view componentName) = 0;

    /**
     * @brief Get resource groups owned by this scene; the first one is the primary group
     */
    virtual ResourceGroupBundle GetResourceGroups() const = 0;
};

META_REGISTER_CLASS(Scene, "ef6321d7-071c-414a-bb3d-55ea6f94688e", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IScene)
META_TYPE(BASE_NS::shared_ptr<RENDER_NS::IRenderContext>)

#endif
