/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_3D_ECS_SYSTEMS_INODE_SYSTEM_H
#define API_3D_ECS_SYSTEMS_INODE_SYSTEM_H

#include <3d/namespace.h>
#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <base/math/quaternion.h>
#include <base/math/vector.h>
#include <core/ecs/entity.h>
#include <core/ecs/intf_system.h>

CORE_BEGIN_NAMESPACE()
class IComponentManager;
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
/** @ingroup group_ecs_systems_inode */
/**
 * Scene Node.
 * Scene node has components automatically for:
 * NodeComponent, NameComponent, LocalMatrixComponent, TransformComponent, WorldMatrixComponent
 */
class ISceneNode {
public:
    virtual ~ISceneNode() = default;

    /** Retrieve name of this node.
     */
    virtual BASE_NS::string GetName() const = 0;

    /** Set name of this node.
     *  @param name String to be set as a name for this node.
     */
    virtual void SetName(BASE_NS::string_view name) = 0;

    /** Set node enabled or disabled, Disabled node is excluded from rendering and logic that is run by ECS systems.
     *  @param isEnabled Boolean value that defines whether this node is enabled or disabled.
     */
    virtual void SetEnabled(bool isEnabled) = 0;

    /** Check whether node is enabled or disabled.
     */
    virtual bool GetEnabled() const = 0;

    /** Check whether node and all its parent nodes are enabled or disabled.
     */
    virtual bool GetEffectivelyEnabled() const = 0;

    /** Retrieve parent of this node.
     */
    virtual ISceneNode* GetParent() const = 0;

    /** Set parent of this node.
     *  @param node Parent to be set for this node.
     */
    virtual void SetParent(ISceneNode const& node) = 0;

    /** Check if given node is this node or descendant of this node.
     *  @param node a Node which against parenthood is checked
     */
    virtual bool IsAncestorOf(ISceneNode const& node) = 0;

    /** Retrieve children of this node.
     */
    virtual BASE_NS::array_view<ISceneNode* const> GetChildren() const = 0;

    /** Retrieve children of this node.
     */
    virtual BASE_NS::array_view<ISceneNode*> GetChildren() = 0;

    /** Retrieve Entity that is associated to this node.
     */
    virtual CORE_NS::Entity GetEntity() const = 0;

    /** Get child node of given parent node by name.
     *  @param name Name which is used to find children.
     */
    virtual const ISceneNode* GetChild(BASE_NS::string_view const& name) const = 0;

    /** Get child node of given parent node by name.
     *  @param name Name which is used to find children.
     */
    virtual ISceneNode* GetChild(BASE_NS::string_view const& name) = 0;

    /** Get child node of given parent node by path.
     *  @param path String of a path which is then used like (ie. path/to/child).
     */
    virtual const ISceneNode* LookupNodeByPath(BASE_NS::string_view const& path) const = 0;

    /** Get child node of given parent node by path.
     *  @param path String of a path which is then used like (ie. path/to/child).
     */
    virtual ISceneNode* LookupNodeByPath(BASE_NS::string_view const& path) = 0;

    /** Recursively look up node by name (returns the first node with given name assigned).
     *  @param name Name of the node.
     */
    virtual const ISceneNode* LookupNodeByName(BASE_NS::string_view const& name) const = 0;

    /** Recursively look up node by name (returns the first node with given name assigned).
     *  @param name Name of the node.
     */
    virtual ISceneNode* LookupNodeByName(BASE_NS::string_view const& name) = 0;

    /** Recursively look up node by component type (returns the first node that has given component assigned).
     *  @param componentManager Component manager that defines the type of the component.
     */
    virtual const ISceneNode* LookupNodeByComponent(const CORE_NS::IComponentManager& componentManager) const = 0;

    /** Recursively look up node by component type (returns the first node that has given component assigned).
     *  @param componentManager Component manager that defines the type of the component.
     */
    virtual ISceneNode* LookupNodeByComponent(const CORE_NS::IComponentManager& componentManager) = 0;

    /** Recursively look up multiple nodes in hierarchy by component type (returns all nodes with given component
     * assigned).
     *  @param componentManager Component manager that defines the type of the component.
     */
    virtual BASE_NS::vector<const ISceneNode*> LookupNodesByComponent(
        const CORE_NS::IComponentManager& componentManager) const = 0;

    /** Recursively look up multiple nodes in hierarchy by component type (returns all nodes with given component
     * assigned).
     *  @param componentManager Component manager that defines the type of the component.
     */
    virtual BASE_NS::vector<ISceneNode*> LookupNodesByComponent(const CORE_NS::IComponentManager& componentManager) = 0;

    // Convenience methods for accessing node transform data.
    /** Get nodes position data
     */
    virtual BASE_NS::Math::Vec3 GetPosition() const = 0;

    /** Get nodes rotation data
     */
    virtual BASE_NS::Math::Quat GetRotation() const = 0;

    /** Get nodes scale data
     */
    virtual BASE_NS::Math::Vec3 GetScale() const = 0;

    /** Set nodes scale data
     */
    virtual void SetScale(const BASE_NS::Math::Vec3& scale) = 0;

    /** Set nodes position data
     */
    virtual void SetPosition(const BASE_NS::Math::Vec3& position) = 0;

    /** Set nodes rotation data
     */
    virtual void SetRotation(const BASE_NS::Math::Quat& rotation) = 0;
};

/** @ingroup group_ecs_systems_inode */
class INodeSystem : public CORE_NS::ISystem {
public:
    static constexpr BASE_NS::Uid UID { "b564d740-3d39-41e7-9add-0bdbdbdf23a8" };

    /** Retrieve root node of the scene graph.
     */
    virtual ISceneNode& GetRootNode() const = 0;

    /** Get node from entity.
     *  @param entity Entity object where we get node from
     */
    virtual ISceneNode* GetNode(CORE_NS::Entity entity) const = 0;

    /** Create new scene node.
     * Creates components for LocalMatrixComponent, NodeComponent, NameComponent, TransformComponent,
     * WorldMatrixComponent.
     */
    virtual ISceneNode* CreateNode() = 0;

    /* Create a clone of a node. Clones all components that the node has.
     * @param node Node to be cloned.
     * @param recursive Set to true to also clone all children of the node recursively.
     */
    virtual ISceneNode* CloneNode(const ISceneNode& node, bool recursive) = 0;

    /** Destroy the scene node and its children.
     * @param node Node to destroy.
     */
    virtual void DestroyNode(ISceneNode& node) = 0;

protected:
    INodeSystem() = default;
    INodeSystem(const INodeSystem&) = delete;
    INodeSystem(INodeSystem&&) = delete;
    INodeSystem& operator=(const INodeSystem&) = delete;
    INodeSystem& operator=(INodeSystem&&) = delete;
};

/** @ingroup group_ecs_systems_inode */
/** Return name of this system
 */
inline constexpr BASE_NS::string_view GetName(const INodeSystem*)
{
    return "NodeSystem";
}
CORE3D_END_NAMESPACE()

#endif // API_3D_ECS_SYSTEMS_INODE_SYSTEM_H
