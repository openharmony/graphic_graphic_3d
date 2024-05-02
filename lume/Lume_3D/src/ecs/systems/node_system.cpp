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

#include "node_system.h"

#include <PropertyTools/property_api_impl.inl>
#include <cinttypes>
#include <cstdlib>
#include <utility>

#include <3d/ecs/components/local_matrix_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <base/containers/unordered_map.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/log.h>
#include <core/namespace.h>

#include "ecs/components/previous_world_matrix_component.h"
#include "util/string_util.h"

CORE_BEGIN_NAMESPACE()
bool operator<(const Entity& lhs, const Entity& rhs)
{
    return lhs.id < rhs.id;
}
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;

namespace {
constexpr auto NODE_INDEX = 0U;
constexpr auto LOCAL_INDEX = 1U;
constexpr auto PREV_WORLD_INDEX = 2U;
constexpr auto WORLD_INDEX = 3U;

template<class T>
T* RecursivelyLookupNodeByPath(T& node, size_t index, const vector<string_view>& path)
{
    // Access to children will automatically refresh cache.
    // Loop through all childs, see if there is a node with given name.
    // If found, then recurse in to that node branch and see if lookup is able to complete in that branch.
    for (T* child : node.GetChildren()) {
        if (child && child->GetName() == path[index]) {
            // Found node with requested name, lookup completed?
            if (index + 1 >= path.size()) {
                return child;
            }

            // Continue lookup recursively.
            T* result = RecursivelyLookupNodeByPath(*child, index + 1, path);
            if (result) {
                // We have a hit.
                return result;
            }
        }
    }

    // No hit.
    return nullptr;
}

template<class T>
T* RecursivelyLookupNodeByName(T& node, string_view name)
{
    if (name.compare(node.GetName()) == 0) {
        return &node;
    }

    // Access to children will automatically refresh cache.
    // Loop through all childs, see if there is a node with given name.
    for (T* child : node.GetChildren()) {
        if (child) {
            // Continue lookup recursively.
            T* result = RecursivelyLookupNodeByName(*child, name);
            if (result) {
                // We have a hit.
                return result;
            }
        }
    }

    // No hit.
    return nullptr;
}

template<class T>
bool RecursivelyLookupNodesByComponent(
    T& node, const IComponentManager& componentManager, vector<T*>& results, bool singleNodeLookup)
{
    bool result = false;
    if (componentManager.HasComponent(node.GetEntity())) {
        result = true;
        results.push_back(&node);
        if (singleNodeLookup) {
            return result;
        }
    }

    // Access to children will automatically refresh cache.
    // Loop through all childs, see if there is a node with given name.
    // If found, then recurse in to that node branch and see if lookup is able to complete in that branch.
    for (T* child : node.GetChildren()) {
        if (child) {
            // Continue lookup recursively.
            if (RecursivelyLookupNodesByComponent(*child, componentManager, results, singleNodeLookup)) {
                result = true;
                if (singleNodeLookup) {
                    return result;
                }
            }
        }
    }

    return result;
}

template<typename ListType, typename ValueType>
inline auto Find(ListType&& list, ValueType&& value)
{
    return std::find(list.begin(), list.end(), BASE_NS::forward<ValueType>(value));
}

template<typename ListType, typename Predicate>
inline auto FindIf(ListType&& list, Predicate&& pred)
{
    return std::find_if(list.begin(), list.end(), BASE_NS::forward<Predicate>(pred));
}
} // namespace

// Interface that allows nodes to access other nodes and request cache updates.
class NodeSystem::NodeAccess {
public:
    virtual ~NodeAccess() = default;

    virtual string GetName(Entity entity) const = 0;
    virtual void SetName(Entity entity, string_view name) = 0;

    virtual Math::Vec3 GetPosition(Entity entity) const = 0;
    virtual Math::Quat GetRotation(Entity entity) const = 0;
    virtual Math::Vec3 GetScale(Entity entity) const = 0;
    virtual void SetScale(Entity entity, const Math::Vec3& scale) = 0;
    virtual void SetPosition(Entity entity, const Math::Vec3& position) = 0;
    virtual void SetRotation(Entity entity, const Math::Quat& rotation) = 0;

    virtual bool GetEnabled(Entity entity) const = 0;
    virtual void SetEnabled(Entity entity, bool isEnabled) = 0;
    virtual bool GetEffectivelyEnabled(Entity entity) const = 0;
    virtual ISceneNode* GetParent(Entity entity) const = 0;
    virtual void SetParent(Entity entity, ISceneNode const& node) = 0;

    virtual void Notify(const ISceneNode& parent, NodeSystem::SceneNodeListener::EventType type,
        const ISceneNode& child, size_t index) = 0;

    virtual NodeSystem::SceneNode* GetNode(Entity const& entity) const = 0;
    virtual void Refresh() = 0;
};

// Implementation of the node interface.
class NodeSystem::SceneNode : public ISceneNode {
public:
    struct NodeState {
        Entity parent { 0 };
        uint32_t localMatrixGeneration { 0 };
        bool enabled { false };
        bool parentChanged { false };
    };

    SceneNode(const SceneNode& other) = delete;
    SceneNode& operator=(const SceneNode& node) = delete;

    SceneNode(Entity entity, NodeAccess& nodeAccess) : entity_(entity), nodeAccess_(nodeAccess) {}

    ~SceneNode() override = default;

    string GetName() const override
    {
        return nodeAccess_.GetName(entity_);
    }

    void SetName(const string_view name) override
    {
        nodeAccess_.SetName(entity_, name);
    }

    void SetEnabled(bool isEnabled) override
    {
        nodeAccess_.SetEnabled(entity_, isEnabled);
    }

    bool GetEnabled() const override
    {
        return nodeAccess_.GetEnabled(entity_);
    }

    bool GetEffectivelyEnabled() const override
    {
        return nodeAccess_.GetEffectivelyEnabled(entity_);
    }

    ISceneNode* GetParent() const override
    {
        if (!EntityUtil::IsValid(entity_)) {
            return nullptr;
        }

        nodeAccess_.Refresh();

        return nodeAccess_.GetParent(entity_);
    }

    void SetParent(ISceneNode const& node) override
    {
        nodeAccess_.Refresh();

        // Ensure we are not ancestors of the new parent.
        CORE_ASSERT(IsAncestorOf(node) == false);

        nodeAccess_.SetParent(entity_, node);
    }

    bool IsAncestorOf(ISceneNode const& node) override
    {
        nodeAccess_.Refresh();

        ISceneNode const* curNode = &node;
        while (curNode != nullptr) {
            if (curNode == this) {
                return true;
            }
            const auto currEntity = curNode->GetEntity();

            curNode = EntityUtil::IsValid(currEntity) ? nodeAccess_.GetParent(currEntity) : nullptr;
        }

        return false;
    }

    array_view<ISceneNode* const> GetChildren() const override
    {
        nodeAccess_.Refresh();
        return { reinterpret_cast<ISceneNode* const*>(children_.data()), children_.size() };
    }

    array_view<ISceneNode*> GetChildren() override
    {
        nodeAccess_.Refresh();
        return array_view(reinterpret_cast<ISceneNode**>(children_.data()), children_.size());
    }

    Entity GetEntity() const override
    {
        return entity_;
    }

    const ISceneNode* GetChild(string_view const& name) const override
    {
        // Access to children will automatically refresh cache.
        auto children = GetChildren();
        if (auto pos =
                FindIf(children, [name](const ISceneNode* child) { return child && (child->GetName() == name); });
            pos != children.end()) {
            return *pos;
        }

        return nullptr;
    }

    ISceneNode* GetChild(string_view const& name) override
    {
        // Access to children will automatically refresh cache.
        auto children = GetChildren();
        if (auto pos =
                FindIf(children, [name](const ISceneNode* child) { return child && (child->GetName() == name); });
            pos != children.end()) {
            return *pos;
        }

        return nullptr;
    }

    bool AddChild(ISceneNode& node) override
    {
        auto children = GetChildren();
        if (auto pos = Find(children, &node); pos == children.end()) {
            nodeAccess_.SetParent(node.GetEntity(), *this);
            nodeAccess_.Notify(*this, INodeSystem::SceneNodeListener::EventType::ADDED, node, children_.size() - 1U);
            return true;
        }
        return false;
    }

    bool InsertChild(size_t index, ISceneNode& node) override
    {
        const auto children = GetChildren();
        if (auto pos = Find(children, &node); pos == children.cend()) {
            nodeAccess_.SetParent(node.GetEntity(), *this);
            if (index < children_.size()) {
                std::rotate(children_.begin() + static_cast<ptrdiff_t>(index), children_.end() - 1, children_.end());
            } else {
                index = children_.size() - 1U;
            }
            nodeAccess_.Notify(*this, INodeSystem::SceneNodeListener::EventType::ADDED, node, index);
            return true;
        }
        return false;
    }

    bool RemoveChild(ISceneNode& node) override
    {
        if (EntityUtil::IsValid(entity_)) {
            const auto children = GetChildren();
            if (auto pos = Find(children, &node); pos != children.cend()) {
                const auto index = pos - children.begin();
                nodeAccess_.SetParent(node.GetEntity(), *nodeAccess_.GetNode({}));
                nodeAccess_.Notify(
                    *this, INodeSystem::SceneNodeListener::EventType::REMOVED, node, static_cast<size_t>(index));
                return true;
            }
        }
        return false;
    }

    bool RemoveChild(size_t index) override
    {
        if (EntityUtil::IsValid(entity_) && !children_.empty()) {
            nodeAccess_.Refresh();
            if (index < children_.size()) {
                if (auto* node = children_[index]) {
                    nodeAccess_.SetParent(node->GetEntity(), *nodeAccess_.GetNode({}));
                    nodeAccess_.Notify(*this, INodeSystem::SceneNodeListener::EventType::REMOVED, *node, index);
                    return true;
                } else {
                    CORE_LOG_W("Node %" PRIx64 " with null child at %zu", entity_.id, index);
                    children_.erase(children_.cbegin() + static_cast<ptrdiff_t>(index));
                }
            }
        }
        return false;
    }

    bool RemoveChildren() override
    {
        if (EntityUtil::IsValid(entity_) && !children_.empty()) {
            auto root = nodeAccess_.GetNode({});
            while (!children_.empty()) {
                if (auto* node = children_.back()) {
                    const auto index = children_.size() - 1U;
                    nodeAccess_.SetParent(node->entity_, *root);
                    nodeAccess_.Notify(*this, INodeSystem::SceneNodeListener::EventType::REMOVED, *node, index);
                } else {
                    CORE_LOG_W("Node %" PRIx64 " with null child at %zu", entity_.id, children_.size() - 1U);
                    children_.pop_back();
                }
            }
            return true;
        }
        return false;
    }

    const ISceneNode* LookupNodeByPath(string_view const& path) const override
    {
        // Split path to array of node names.
        vector<string_view> parts = StringUtil::Split(path, "/");
        if (parts.size() > 0) {
            // Perform lookup using array of names and 'current' index (start from zero).
            return RecursivelyLookupNodeByPath<const ISceneNode>(*this, 0, parts);
        }

        return nullptr;
    }

    ISceneNode* LookupNodeByPath(string_view const& path) override
    {
        // Split path to array of node names.
        vector<string_view> parts = StringUtil::Split(path, "/");
        if (parts.size() > 0) {
            // Perform lookup using array of names and 'current' index (start from zero).
            return RecursivelyLookupNodeByPath<ISceneNode>(*this, 0, parts);
        }

        return nullptr;
    }

    const ISceneNode* LookupNodeByName(string_view const& name) const override
    {
        return RecursivelyLookupNodeByName<const ISceneNode>(*this, name);
    }

    ISceneNode* LookupNodeByName(string_view const& name) override
    {
        return RecursivelyLookupNodeByName<ISceneNode>(*this, name);
    }

    const ISceneNode* LookupNodeByComponent(const IComponentManager& componentManager) const override
    {
        vector<const ISceneNode*> results;
        if (RecursivelyLookupNodesByComponent<const ISceneNode>(*this, componentManager, results, true)) {
            return results[0];
        }

        return nullptr;
    }

    ISceneNode* LookupNodeByComponent(const IComponentManager& componentManager) override
    {
        vector<ISceneNode*> results;
        if (RecursivelyLookupNodesByComponent<ISceneNode>(*this, componentManager, results, true)) {
            return results[0];
        }

        return nullptr;
    }

    vector<const ISceneNode*> LookupNodesByComponent(const IComponentManager& componentManager) const override
    {
        vector<const ISceneNode*> results;
        RecursivelyLookupNodesByComponent<const ISceneNode>(*this, componentManager, results, false);
        return results;
    }

    vector<ISceneNode*> LookupNodesByComponent(const IComponentManager& componentManager) override
    {
        vector<ISceneNode*> results;
        RecursivelyLookupNodesByComponent<ISceneNode>(*this, componentManager, results, false);
        return results;
    }

    Math::Vec3 GetPosition() const override
    {
        return nodeAccess_.GetPosition(entity_);
    }

    Math::Quat GetRotation() const override
    {
        return nodeAccess_.GetRotation(entity_);
    }

    Math::Vec3 GetScale() const override
    {
        return nodeAccess_.GetScale(entity_);
    }

    void SetScale(const Math::Vec3& scale) override
    {
        return nodeAccess_.SetScale(entity_, scale);
    }

    void SetPosition(const Math::Vec3& position) override
    {
        nodeAccess_.SetPosition(entity_, position);
    }

    void SetRotation(const Math::Quat& rotation) override
    {
        nodeAccess_.SetRotation(entity_, rotation);
    }

    // Internally for NodeSystem to skip unneccessary NodeCache::Refresh calls
    SceneNode* PopChildNoRefresh()
    {
        if (children_.empty()) {
            return nullptr;
        } else {
            auto child = children_.back();
            children_.pop_back();
            return child;
        }
    }

private:
    const Entity entity_;

    NodeAccess& nodeAccess_;

    vector<SceneNode*> children_;
    NodeState lastState_ {};

    friend NodeSystem;
    friend NodeSystem::NodeCache;
};

// Cache for nodes.
class NodeSystem::NodeCache final : public NodeAccess {
public:
    NodeCache(IEntityManager& entityManager, INameComponentManager& nameComponentManager,
        INodeComponentManager& nodeComponentManager, ITransformComponentManager& transformComponentManager)
        : nameComponentManager_(nameComponentManager), nodeComponentManager_(nodeComponentManager),
          transformComponentManager_(transformComponentManager), entityManager_(entityManager)
    {
        // Add root node.
        AddNode(Entity());
    }

    ~NodeCache() override = default;

    void Reset()
    {
        const auto first = nodeEntities_.cbegin();
        const auto last = nodeEntities_.cend();
        if (auto pos = std::lower_bound(
                first, last, Entity(), [](const Entity& lhs, const Entity& rhs) { return lhs.id < rhs.id; });
            (pos != last) && (*pos == Entity())) {
            const auto index = static_cast<size_t>(pos - first);
            auto rootNode = move(nodes_[index]);
            rootNode->children_.clear();
            nodes_.clear();
            nodes_.push_back(move(rootNode));
            nodeEntities_.clear();
            nodeEntities_.push_back(Entity());
            auto entry = lookUp_.extract(Entity());
            lookUp_.clear();
            lookUp_.insert(move(entry));
        }
    }

    SceneNode* AddNode(Entity const& entity)
    {
        SceneNode* result;

        auto node = make_unique<SceneNode>(entity, *this);
        result = node.get();
        const auto first = nodeEntities_.cbegin();
        const auto last = nodeEntities_.cend();
        auto pos =
            std::lower_bound(first, last, entity, [](const Entity& lhs, const Entity& rhs) { return lhs.id < rhs.id; });
        const auto index = pos - first;
        if ((pos == last) || (*pos != entity)) {
            nodeEntities_.insert(pos, entity);
            lookUp_[entity] = node.get();
            nodes_.insert(nodes_.cbegin() + index, move(node));
        } else {
            lookUp_[entity] = node.get();
            nodes_[static_cast<size_t>(index)] = move(node);
        }

        if (auto handle = nodeComponentManager_.Read(entity)) {
            if (auto* parent = GetNode(handle->parent)) {
                // Set parent / child relationship.
                parent->children_.push_back(result);
                result->lastState_.parent = handle->parent;
            }
            result->lastState_.parentChanged = true;
        }

        // check if some node thinks it should be the child of the new node and it there.
        for (const auto& nodePtr : nodes_) {
            if (nodePtr->lastState_.parent == entity) {
                result->children_.push_back(nodePtr.get());
            }
        }

        return result;
    }

    string GetName(const Entity entity) const override
    {
        if (const auto nameId = nameComponentManager_.GetComponentId(entity);
            nameId != IComponentManager::INVALID_COMPONENT_ID) {
            return nameComponentManager_.Get(entity).name;
        } else {
            return "";
        }
    }

    void SetName(const Entity entity, const string_view name) override
    {
        if (ScopedHandle<NameComponent> data = nameComponentManager_.Write(entity); data) {
            data->name = name;
        }
    }

    Math::Vec3 GetPosition(const Entity entity) const override
    {
        if (const auto nameId = transformComponentManager_.GetComponentId(entity);
            nameId != IComponentManager::INVALID_COMPONENT_ID) {
            return transformComponentManager_.Get(entity).position;
        } else {
            return Math::Vec3();
        }
    }

    Math::Quat GetRotation(const Entity entity) const override
    {
        if (const auto nameId = transformComponentManager_.GetComponentId(entity);
            nameId != IComponentManager::INVALID_COMPONENT_ID) {
            return transformComponentManager_.Get(entity).rotation;
        } else {
            return Math::Quat();
        }
    }

    Math::Vec3 GetScale(const Entity entity) const override
    {
        if (const auto nameId = transformComponentManager_.GetComponentId(entity);
            nameId != IComponentManager::INVALID_COMPONENT_ID) {
            return transformComponentManager_.Get(entity).scale;
        } else {
            return Math::Vec3 { 1.0f, 1.0f, 1.0f };
        }
    }

    void SetScale(const Entity entity, const Math::Vec3& scale) override
    {
        if (ScopedHandle<TransformComponent> data = transformComponentManager_.Write(entity); data) {
            data->scale = scale;
        }
    }

    void SetPosition(const Entity entity, const Math::Vec3& position) override
    {
        if (ScopedHandle<TransformComponent> data = transformComponentManager_.Write(entity); data) {
            data->position = position;
        }
    }

    void SetRotation(const Entity entity, const Math::Quat& rotation) override
    {
        if (ScopedHandle<TransformComponent> data = transformComponentManager_.Write(entity); data) {
            data->rotation = rotation;
        }
    }

    bool GetEnabled(const Entity entity) const override
    {
        bool enabled = true;
        if (ScopedHandle<const NodeComponent> data = nodeComponentManager_.Read(entity); data) {
            enabled = data->enabled;
        }
        return enabled;
    }

    static void DisableTree(SceneNode* node, INodeComponentManager& nodeComponentManager)
    {
        BASE_NS::vector<SceneNode*> stack;
        stack.push_back(node);

        while (!stack.empty()) {
            auto current = stack.back();
            stack.pop_back();
            if (!current) {
                continue;
            }
            auto* handle = nodeComponentManager.GetData(current->entity_);
            if (!handle) {
                continue;
            }
            // if a node is effectively enabled, update and add children. otherwise we can assume the node had been
            // disabled earlier and its children are already disabled.
            if (ScopedHandle<const NodeComponent>(handle)->effectivelyEnabled) {
                ScopedHandle<NodeComponent>(handle)->effectivelyEnabled = false;
                stack.append(current->children_.cbegin(), current->children_.cend());
            }
        }
    }

    static void EnableTree(SceneNode* node, INodeComponentManager& nodeComponentManager)
    {
        BASE_NS::vector<SceneNode*> stack;
        stack.push_back(node);

        while (!stack.empty()) {
            auto current = stack.back();
            stack.pop_back();
            if (!current) {
                continue;
            }
            auto* childHandle = nodeComponentManager.GetData(current->entity_);
            if (!childHandle) {
                continue;
            }
            // if a node is enabled, update and add children. otherwise the node and its children remain effectivaly
            // disabled.
            if (ScopedHandle<const NodeComponent>(childHandle)->enabled) {
                ScopedHandle<NodeComponent>(childHandle)->effectivelyEnabled = true;
                stack.append(current->children_.cbegin(), current->children_.cend());
            }
        }
    }

    void SetEnabled(const Entity entity, const bool isEnabled) override
    {
        auto* handle = nodeComponentManager_.GetData(entity);
        if (!handle) {
            return;
        }
        const auto nodeComponent = *ScopedHandle<const NodeComponent>(handle);
        if (nodeComponent.enabled == isEnabled) {
            return;
        }

        ScopedHandle<NodeComponent>(handle)->enabled = isEnabled;

        const auto nodeGeneration = nodeComponentManager_.GetGenerationCounter();
        if ((nodeComponentGenerationId_ + 1U) != nodeGeneration) {
            // if generation count has changed, we can only update this node.
            return;
        }
        nodeComponentGenerationId_ = nodeGeneration;

        if (isEnabled == nodeComponent.effectivelyEnabled) {
            // if effectivelyEnabled matches the new state, there's no need to update the tree.
            return;
        }

        auto node = GetNode(entity);
        if (!node) {
            // if the node can't be found, we can only update this node.
            return;
        }

        if (!isEnabled) {
            DisableTree(node, nodeComponentManager_);
        } else if (auto parent = GetNode(nodeComponent.parent); !parent || !parent->GetEffectivelyEnabled()) {
            // if the node's parent is disabled, there's no need to update the tree.
            return;
        } else {
            EnableTree(node, nodeComponentManager_);
        }

        nodeComponentGenerationId_ = nodeComponentManager_.GetGenerationCounter();
    }

    bool GetEffectivelyEnabled(const Entity entity) const override
    {
        bool effectivelyEnabled = true;
        if (ScopedHandle<const NodeComponent> data = nodeComponentManager_.Read(entity); data) {
            effectivelyEnabled = data->effectivelyEnabled;
        }
        return effectivelyEnabled;
    }

    ISceneNode* GetParent(const Entity entity) const override
    {
        Entity parent;
        if (ScopedHandle<const NodeComponent> data = nodeComponentManager_.Read(entity); data) {
            parent = data->parent;
        }

        return GetNode(parent);
    }

    void SetParent(const Entity entity, ISceneNode const& node) override
    {
        Entity oldParent;
        const auto newParent = node.GetEntity();
        const auto effectivelyEnabled = node.GetEffectivelyEnabled();
        if (ScopedHandle<NodeComponent> data = nodeComponentManager_.Write(entity); data) {
            oldParent = data->parent;
            data->parent = newParent;
            data->effectivelyEnabled = effectivelyEnabled;
        }
        UpdateParent(entity, oldParent, newParent);
    }

    void Notify(const ISceneNode& parent, NodeSystem::SceneNodeListener::EventType type, const ISceneNode& child,
        size_t index) override
    {
        for (auto* listener : listeners_) {
            if (listener) {
                listener->OnChildChanged(parent, type, child, index);
            }
        }
    }

    SceneNode* GetNode(Entity const& entity) const override
    {
        auto pos = lookUp_.find(entity);
        if (pos != lookUp_.end()) {
            return pos->second;
        }
        return nullptr;
    }

    void UpdateParent(Entity entity, Entity oldParent, Entity newParent)
    {
        const uint32_t nodeGenerationId = nodeComponentManager_.GetGenerationCounter();
        if (nodeGenerationId <= (nodeComponentGenerationId_ + 1U)) {
            nodeComponentGenerationId_ = nodeGenerationId;

            if (SceneNode* node = GetNode(entity)) {
                if (SceneNode* parent = GetNode(oldParent); parent) {
                    parent->children_.erase(std::remove(parent->children_.begin(), parent->children_.end(), node),
                        parent->children_.cend());
                    node->lastState_.parent = {};
                }
                if (SceneNode* parent = GetNode(newParent); parent) {
                    // Set parent / child relationship.
                    parent->children_.push_back(node);
                    node->lastState_.parent = newParent;
                }
                node->lastState_.parentChanged = true;
            } else {
                CORE_LOG_W("Updating parent of invalid node %" PRIx64 " (old %" PRIx64 " new %" PRIx64 ")", entity.id,
                    oldParent.id, newParent.id);
            }
        }
    }

    void Refresh() override
    {
        // If no modifications, then no need to refresh.
        const uint32_t nodeGenerationId = nodeComponentManager_.GetGenerationCounter();
        const uint32_t entityGenerationId = entityManager_.GetGenerationCounter();
        if (nodeGenerationId == nodeComponentGenerationId_ && entityGenerationId == entityGenerationId_) {
            return;
        }

        vector<Entity> entitiesWithNode;
        // allocate space for all entities + extra for collecting removed and added entities. worst case is that all
        // the old ones have been removed and everything is new.
        const auto nodeComponents = nodeComponentManager_.GetComponentCount();
        entitiesWithNode.reserve((nodeComponents + nodeEntities_.size()) * 2U);
        entitiesWithNode.resize(nodeComponents);
        for (IComponentManager::ComponentId i = 0; i < nodeComponents; ++i) {
            auto entity = nodeComponentManager_.GetEntity(i);
            entitiesWithNode[i] = entity;
        }

        auto first = entitiesWithNode.begin();
        auto last = entitiesWithNode.end();

        // remove inactive entities
        last = entitiesWithNode.erase(
            std::remove_if(first, last, [&em = entityManager_](Entity entity) { return !em.IsAlive(entity); }), last);

        std::sort(first, last, [](Entity lhs, Entity rhs) { return lhs.id < rhs.id; });

        // there's at least the root (invalid/default constructed entity)
        entitiesWithNode.insert(first, Entity());
        last = entitiesWithNode.end();

        // Look up entities that no longer exist.
        auto inserter = std::back_inserter(entitiesWithNode);
        inserter = std::set_difference(nodeEntities_.cbegin(), nodeEntities_.cend(), first, last, inserter,
            [](Entity lhs, Entity rhs) { return lhs.id < rhs.id; });

        auto lastRemoved = entitiesWithNode.end();

        // Look up entities that have been added.
        inserter = std::set_difference(first, last, nodeEntities_.cbegin(), nodeEntities_.cend(), inserter,
            [](Entity lhs, Entity rhs) { return lhs.id < rhs.id; });
        auto lastAdded = entitiesWithNode.end();

        // Remove !alive entities and those without node component
        std::for_each(last, lastRemoved, [this](Entity oldEntity) {
            const auto first = nodeEntities_.cbegin();
            const auto last = nodeEntities_.cend();
            if (auto pos =
                    std::lower_bound(first, last, oldEntity, [](Entity lhs, Entity rhs) { return lhs.id < rhs.id; });
                (pos != last) && (*pos == oldEntity)) {
                const auto index = pos - first;
                // detach the node from its parent before cleanup
                {
                    auto* node = nodes_[static_cast<size_t>(index)].get();
                    Entity parentEntity = static_cast<SceneNode*>(node)->lastState_.parent;
                    if (auto parentIt = lookUp_.find(parentEntity); parentIt != lookUp_.end()) {
                        auto& children = parentIt->second->children_;
                        children.erase(std::remove(children.begin(), children.end(), node), children.cend());
                    }
                    // can we rely on lastState_.parent, or also check from node component?
                    if (ScopedHandle<const NodeComponent> data = nodeComponentManager_.Read(oldEntity); data) {
                        if (parentEntity != data->parent) {
                            if (auto parentIt = lookUp_.find(data->parent); parentIt != lookUp_.end()) {
                                auto& children = parentIt->second->children_;
                                children.erase(std::remove(children.begin(), children.end(), node), children.cend());
                            }
                        }
                    }
                }
                nodeEntities_.erase(pos);
                nodes_.erase(nodes_.cbegin() + index);
                lookUp_.erase(oldEntity);
            }
        });

        // Add entities that appeared since last refresh.
        std::for_each(lastRemoved, lastAdded, [this](Entity newEntity) { AddNode(newEntity); });

        std::for_each(first, last, [this](Entity entity) {
            if (SceneNode* sceneNode = GetNode(entity)) {
                if (const auto nodeComponent = nodeComponentManager_.Read(entity)) {
                    if (sceneNode->lastState_.parent != nodeComponent->parent) {
                        if (SceneNode* oldParent = GetNode(sceneNode->lastState_.parent)) {
                            oldParent->children_.erase(
                                std::remove(oldParent->children_.begin(), oldParent->children_.end(), sceneNode),
                                oldParent->children_.cend());
                        }
                        sceneNode->lastState_.parent = {};
                        sceneNode->lastState_.parentChanged = true;
                        if (SceneNode* newParent = GetNode(nodeComponent->parent)) {
                            // Set parent / child relationship.
                            if (std::none_of(newParent->children_.cbegin(), newParent->children_.cend(),
                                    [sceneNode](const SceneNode* childNode) { return childNode == sceneNode; })) {
                                newParent->children_.push_back(sceneNode);
                            }
                            sceneNode->lastState_.parent = nodeComponent->parent;
                        }
                    }
                }
            }
        });

        nodeComponentGenerationId_ = nodeGenerationId;
        entityGenerationId_ = entityGenerationId;
    }

    void InternalNodeUpdate()
    {
        const uint32_t nodeGenerationId = nodeComponentManager_.GetGenerationCounter();
        const uint32_t entityGenerationId = entityManager_.GetGenerationCounter();
        if (nodeGenerationId <= (nodeComponentGenerationId_ + 1U) && entityGenerationId <= (entityGenerationId_ + 1U)) {
            nodeComponentGenerationId_ = nodeGenerationId;
            entityGenerationId_ = entityGenerationId;
        }
    }

    // Internally for NodeSystem to skip unneccessary NodeCache::Refresh calls
    SceneNode* GetParentNoRefresh(ISceneNode const& node) const
    {
        Entity parent;
        if (ScopedHandle<const NodeComponent> data = nodeComponentManager_.Read(node.GetEntity()); data) {
            parent = data->parent;
        }

        if (EntityUtil::IsValid(parent)) {
            return GetNode(parent);
        }
        return nullptr;
    }

    void AddListener(SceneNodeListener& listener)
    {
        if (Find(listeners_, &listener) != listeners_.end()) {
            // already added.
            return;
        }
        listeners_.push_back(&listener);
    }

    void RemoveListener(SceneNodeListener& listener)
    {
        if (auto it = Find(listeners_, &listener); it != listeners_.end()) {
            *it = nullptr;
            return;
        }
    }

private:
    vector<Entity> nodeEntities_;
    vector<unique_ptr<SceneNode>> nodes_;
    unordered_map<Entity, SceneNode*> lookUp_;

    uint32_t nodeComponentGenerationId_ = { 0 };
    uint32_t entityGenerationId_ = { 0 };

    INameComponentManager& nameComponentManager_;
    INodeComponentManager& nodeComponentManager_;
    ITransformComponentManager& transformComponentManager_;
    IEntityManager& entityManager_;

    BASE_NS::vector<SceneNodeListener*> listeners_;
};

// State when traversing node tree.
struct NodeSystem::State {
    SceneNode* node;
    Math::Mat4X4 parentMatrix;
    bool parentEnabled;
};

void NodeSystem::SetActive(bool state)
{
    active_ = state;
}

bool NodeSystem::IsActive() const
{
    return active_;
}

NodeSystem::NodeSystem(IEcs& ecs)
    : ecs_(ecs), nameManager_(*(GetManager<INameComponentManager>(ecs))),
      nodeManager_(*(GetManager<INodeComponentManager>(ecs))),
      transformManager_(*(GetManager<ITransformComponentManager>(ecs))),
      localMatrixManager_(*(GetManager<ILocalMatrixComponentManager>(ecs))),
      worldMatrixManager_(*(GetManager<IWorldMatrixComponentManager>(ecs))),
      prevWorldMatrixManager_(*(GetManager<IPreviousWorldMatrixComponentManager>(ecs))),
      cache_(make_unique<NodeCache>(ecs.GetEntityManager(), nameManager_, nodeManager_, transformManager_))
{}

string_view NodeSystem::GetName() const
{
    return CORE3D_NS::GetName(this);
}

Uid NodeSystem::GetUid() const
{
    return UID;
}

IPropertyHandle* NodeSystem::GetProperties()
{
    return nullptr;
}

const IPropertyHandle* NodeSystem::GetProperties() const
{
    return nullptr;
}

void NodeSystem::SetProperties(const IPropertyHandle&) {}

const IEcs& NodeSystem::GetECS() const
{
    return ecs_;
}

ISceneNode& NodeSystem::GetRootNode() const
{
    if (auto rootNode = cache_->GetNode(Entity()); rootNode) {
        return *rootNode;
    }
    std::abort();
}

ISceneNode* NodeSystem::GetNode(Entity entity) const
{
    if (EntityUtil::IsValid(entity)) {
        // Make sure node cache is valid.
        cache_->Refresh();

        return cache_->GetNode(entity);
    }
    return nullptr;
}

ISceneNode* NodeSystem::CreateNode()
{
    const Entity entity = ecs_.GetEntityManager().Create();

    nodeManager_.Create(entity);
    nameManager_.Create(entity);
    transformManager_.Create(entity);
    localMatrixManager_.Create(entity);
    worldMatrixManager_.Create(entity);
    cache_->InternalNodeUpdate();
    return cache_->AddNode(entity);
}

ISceneNode* NodeSystem::CloneNode(const ISceneNode& node, bool recursive)
{
    // gather all the entities in the hierarchy
    vector<Entity> nodes;
    nodes.reserve(nodeManager_.GetComponentCount());
    if (recursive) {
        GatherNodeEntities(node, nodes);
    } else {
        nodes.push_back(node.GetEntity());
    }

    // clone the hierachy while gathering a map from old to new entities
    unordered_map<Entity, Entity> oldToNew;
    oldToNew.reserve(nodes.size());
    for (const Entity& originalEntity : nodes) {
        oldToNew.insert({ originalEntity, ecs_.CloneEntity(originalEntity) });
    }
    auto update = [](const unordered_map<Entity, Entity>& oldToNew, const Property& property, IPropertyHandle* handle,
                      Entity current, size_t entityIdx) {
        if (EntityUtil::IsValid(current)) {
            if (const auto pos = oldToNew.find(current); pos != oldToNew.end()) {
                reinterpret_cast<Entity*>(reinterpret_cast<uintptr_t>(handle->WLock()) + property.offset)[entityIdx] =
                    pos->second;
                handle->WUnlock();
            }
        }
    };
    // go through the new entities and update their components to point to the clones instead of originals.
    auto managers = ecs_.GetComponentManagers();
    for (auto [oldEntity, newEntity] : oldToNew) {
        for (auto cm : managers) {
            if (auto handle = cm->GetData(newEntity); handle) {
                for (const auto& property : handle->Owner()->MetaData()) {
                    if ((property.type == PropertyType::ENTITY_T) || (property.type == PropertyType::ENTITY_ARRAY_T)) {
                        const Entity* entities = reinterpret_cast<const Entity*>(
                            reinterpret_cast<uintptr_t>(handle->RLock()) + property.offset);
                        size_t entityIdx = 0;
                        for (auto current : array_view(entities, property.count)) {
                            update(oldToNew, property, handle, current, entityIdx);
                            ++entityIdx;
                        }
                        handle->RUnlock();
                    }
                }
            }
        }
    }

    return GetNode(oldToNew[nodes.front()]);
}

void NodeSystem::DestroyNode(ISceneNode& rootNode)
{
    // Make sure node cache is valid.
    cache_->Refresh();

    IEntityManager& entityManager = ecs_.GetEntityManager();

    auto* node = static_cast<SceneNode*>(&rootNode);
    for (bool done = false; !done && node;) {
        if (auto child = node->PopChildNoRefresh(); !child) {
            auto* destroy = std::exchange(node, cache_->GetParentNoRefresh(*node));
            if (destroy == &rootNode) {
                done = true;
                if (node) {
                    node->children_.erase(
                        std::remove(node->children_.begin(), node->children_.end(), destroy), node->children_.cend());
                    destroy->lastState_.parent = {};
                    destroy->lastState_.parentChanged = true;
                }
                // check this is not the root node which cannot be destroyed.
                const auto entity = destroy->GetEntity();
                if (EntityUtil::IsValid(entity)) {
                    entityManager.Destroy(entity);
                }
            } else {
                entityManager.Destroy(destroy->GetEntity());
            }
        } else {
            node = child;
        }
    }
}

void NodeSystem::AddListener(SceneNodeListener& listener)
{
    cache_->AddListener(listener);
}

void NodeSystem::RemoveListener(SceneNodeListener& listener)
{
    cache_->RemoveListener(listener);
}

void NodeSystem::Initialize()
{
    ComponentQuery::Operation operations[] = {
        { localMatrixManager_, ComponentQuery::Operation::Method::OPTIONAL },
        { prevWorldMatrixManager_, ComponentQuery::Operation::Method::OPTIONAL },
        { worldMatrixManager_, ComponentQuery::Operation::Method::OPTIONAL },
    };
    CORE_ASSERT(&operations[LOCAL_INDEX - 1U].target == &localMatrixManager_);
    CORE_ASSERT(&operations[WORLD_INDEX - 1U].target == &worldMatrixManager_);
    CORE_ASSERT(&operations[PREV_WORLD_INDEX - 1U].target == &prevWorldMatrixManager_);
    nodeQuery_.SetupQuery(nodeManager_, operations, true);
    nodeQuery_.SetEcsListenersEnabled(true);
}

bool NodeSystem::Update(bool, uint64_t, uint64_t)
{
    if (!active_) {
        return false;
    }

    nodeQuery_.Execute();

    // Cache world matrices from previous frame.
    const bool missingPrevWorldMatrix = UpdatePreviousWorldMatrices();

    if (localMatrixGeneration_ == localMatrixManager_.GetGenerationCounter() &&
        nodeGeneration_ == nodeManager_.GetGenerationCounter()) {
        return false;
    }

    // Make sure node cache is valid.
    cache_->Refresh();

    vector<ISceneNode*> changedNodes;
    changedNodes.reserve(256u); // reserve a chunk to fit a large scene.

    // Find all parent nodes that have their transform updated.
    CollectChangedNodes(GetRootNode(), changedNodes);

    // Update world transformations for changed tree branches.
    for (const auto node : changedNodes) {
        bool parentEnabled = true;
        Math::Mat4X4 parentMatrix(1.0f);

        if (const ISceneNode* parent = node->GetParent(); parent) {
            // Get parent world matrix.
            if (auto row = nodeQuery_.FindResultRow(parent->GetEntity()); row) {
                if (row->IsValidComponentId(WORLD_INDEX)) {
                    parentMatrix = worldMatrixManager_.Get(row->components[WORLD_INDEX]).matrix;
                }
                if (auto nodeHandle = nodeManager_.Read(row->components[NODE_INDEX])) {
                    parentEnabled = nodeHandle->effectivelyEnabled;
                } else {
                    CORE_LOG_W("%" PRIx64 " missing Node", row->entity.id);
                }
            }
        }

        UpdateTransformations(*node, parentMatrix, parentEnabled);
    }

    // Store generation counters.
    localMatrixGeneration_ = localMatrixManager_.GetGenerationCounter();
    nodeGeneration_ = nodeManager_.GetGenerationCounter();

    if (missingPrevWorldMatrix) {
        for (const auto& row : nodeQuery_.GetResults()) {
            // Create missing PreviousWorldMatrixComponents and initialize with current world.
            if (!row.IsValidComponentId(PREV_WORLD_INDEX) && row.IsValidComponentId(WORLD_INDEX)) {
                prevWorldMatrixManager_.Create(row.entity);
                if (auto dst = prevWorldMatrixManager_.Write(row.entity)) {
                    if (auto src = worldMatrixManager_.Read(row.components[WORLD_INDEX])) {
                        dst->matrix = src->matrix;
                    } else {
                        CORE_LOG_W("%" PRIx64 " missing WorldMatrix", row.entity.id);
                    }
                } else {
                    CORE_LOG_W("%" PRIx64 " missing PreviousWorldMatrix", row.entity.id);
                }
            }
        }
    }

    return true;
}

void NodeSystem::Uninitialize()
{
    cache_->Reset();
    nodeQuery_.SetEcsListenersEnabled(false);
}

void NodeSystem::CollectChangedNodes(ISceneNode& node, vector<ISceneNode*>& result)
{
    auto& sceneNode = (const SceneNode&)(node);
    auto& lastState = sceneNode.lastState_;

    if (auto row = nodeQuery_.FindResultRow(node.GetEntity()); row) {
        // If local matrix component has changed, the sub-tree needs to be recalculated.
        if (row->IsValidComponentId(LOCAL_INDEX)) {
            const uint32_t currentGeneration = localMatrixManager_.GetComponentGeneration(row->components[LOCAL_INDEX]);
            if (lastState.localMatrixGeneration != currentGeneration) {
                result.push_back(&node);
                return;
            }
        }

        // If node enabled state or parent component has changed, the sub-tree needs to be recalculated.
        if (const auto& nodeComponent = nodeManager_.Read(row->components[NODE_INDEX])) {
            const bool parentChanged = (lastState.parent != nodeComponent->parent) || lastState.parentChanged;
            const bool enabledChanged = lastState.enabled != nodeComponent->enabled;
            if (parentChanged || enabledChanged) {
                result.push_back(&node);
                return;
            }
        } else {
            CORE_LOG_W("%" PRIx64 " missing Node", row->entity.id);
        }
    }

    for (auto* const child : node.GetChildren()) {
        if (child) {
            CollectChangedNodes(*child, result);
        }
    }
}

struct NodeSystem::NodeInfo {
    Entity parent;
    bool isEffectivelyEnabled;
    bool effectivelyEnabledChanged;
};

NodeSystem::NodeInfo NodeSystem::ProcessNode(
    SceneNode* node, const bool parentEnabled, const ComponentQuery::ResultRow* row)
{
    NodeInfo info;
    info.isEffectivelyEnabled = parentEnabled;
    info.effectivelyEnabledChanged = false;

    IPropertyHandle* handle = nodeManager_.GetData(row->components[NODE_INDEX]);
    auto nc = ScopedHandle<const NodeComponent>(handle);
    info.parent = nc->parent;
    const bool nodeEnabled = nc->enabled;
    // Update effectively enabled status if it has changed (e.g. due to parent enabled state changes).
    info.isEffectivelyEnabled = info.isEffectivelyEnabled && nodeEnabled;
    if (nc->effectivelyEnabled != info.isEffectivelyEnabled) {
        ScopedHandle<NodeComponent>(handle)->effectivelyEnabled = info.isEffectivelyEnabled;
        cache_->InternalNodeUpdate();
        info.effectivelyEnabledChanged = true;
    }
    node->lastState_.enabled = nodeEnabled;

    return info;
}

void NodeSystem::UpdateTransformations(ISceneNode& node, Math::Mat4X4 const& matrix, bool enabled)
{
    BASE_NS::vector<State> stack;
    stack.reserve(nodeManager_.GetComponentCount());
    stack.push_back(State { static_cast<SceneNode*>(&node), matrix, enabled });
    while (!stack.empty()) {
        auto state = stack.back();
        stack.pop_back();

        auto row = nodeQuery_.FindResultRow(state.node->GetEntity());
        if (!row) {
            continue;
        }
        const auto nodeInfo = ProcessNode(state.node, state.parentEnabled, row);

        Math::Mat4X4& pm = state.parentMatrix;

        if (nodeInfo.isEffectivelyEnabled && row->IsValidComponentId(LOCAL_INDEX)) {
            if (auto local = localMatrixManager_.Read(row->components[LOCAL_INDEX])) {
                pm = pm * local->matrix;
            } else {
                CORE_LOG_W("%" PRIx64 " missing LocalWorldMatrix", row->entity.id);
            }

            if (row->IsValidComponentId(WORLD_INDEX)) {
                if (auto worldMatrixHandle = worldMatrixManager_.Write(row->components[WORLD_INDEX])) {
                    worldMatrixHandle->matrix = pm;
                } else {
                    CORE_LOG_W("%" PRIx64 " missing WorldMatrix", row->entity.id);
                }
            } else {
                worldMatrixManager_.Set(row->entity, { pm });
            }

            // Save the values that were used to calculate current world matrix.
            state.node->lastState_.localMatrixGeneration =
                localMatrixManager_.GetComponentGeneration(row->components[LOCAL_INDEX]);
            state.node->lastState_.parent = nodeInfo.parent;
        }
        if (nodeInfo.isEffectivelyEnabled || nodeInfo.effectivelyEnabledChanged) {
            for (auto* child : state.node->GetChildren()) {
                if (child) {
                    stack.push_back(State { static_cast<SceneNode*>(child), pm, nodeInfo.isEffectivelyEnabled });
                }
            }
        }
    }
}

void NodeSystem::GatherNodeEntities(const ISceneNode& node, vector<Entity>& entities) const
{
    entities.push_back(node.GetEntity());
    for (const ISceneNode* child : node.GetChildren()) {
        if (child) {
            GatherNodeEntities(*child, entities);
        }
    }
}

bool NodeSystem::UpdatePreviousWorldMatrices()
{
    bool missingPrevWorldMatrix = false;
    if (worldMatrixGeneration_ != worldMatrixManager_.GetGenerationCounter()) {
        worldMatrixGeneration_ = worldMatrixManager_.GetGenerationCounter();

        for (const auto& row : nodeQuery_.GetResults()) {
            const bool hasPrev = row.IsValidComponentId(PREV_WORLD_INDEX);
            if (hasPrev && row.IsValidComponentId(WORLD_INDEX)) {
                if (auto dst = prevWorldMatrixManager_.Write(row.components[PREV_WORLD_INDEX])) {
                    if (auto src = worldMatrixManager_.Read(row.components[WORLD_INDEX])) {
                        dst->matrix = src->matrix;
                    } else {
                        CORE_LOG_W("%" PRIx64 " missing WorldMatrix", row.entity.id);
                    }
                } else {
                    CORE_LOG_W("%" PRIx64 " missing PreviousWorldMatrix", row.entity.id);
                }
            } else if (!hasPrev) {
                missingPrevWorldMatrix = true;
            }
        }
    }
    return missingPrevWorldMatrix;
}

ISystem* INodeSystemInstance(IEcs& ecs)
{
    return new NodeSystem(ecs);
}
void INodeSystemDestroy(ISystem* instance)
{
    delete static_cast<NodeSystem*>(instance);
}

CORE3D_END_NAMESPACE()
