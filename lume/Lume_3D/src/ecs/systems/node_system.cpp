/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include <core/perf/cpu_perf_scope.h>
#include <core/property/property_types.h>
#include <core/property_tools/property_api_impl.inl>

#include "util/log.h"
#include "util/string_util.h"

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;

namespace {
constexpr auto NODE_INDEX = 0U;
constexpr auto LOCAL_INDEX = 1U;
constexpr auto WORLD_INDEX = 2U;

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

template<class T>
T* LookupNodeByPath(T& node, array_view<const string_view> path)
{
    T* current = &node;
    auto curPath = path.cbegin();
    auto lastPath = path.cend();
    while (current) {
        const auto& children = current->GetChildren();
        auto pos = FindIf(children, [curPath](const ISceneNode* node) { return node && node->GetName() == *curPath; });
        if (pos == children.cend()) {
            // current subpath doesn't match any child node so node wasn't found.
            return nullptr;
        }
        current = *pos;
        ++curPath;
        if (curPath == lastPath) {
            // this was the last subpath so the node was found.
            return current;
        }
    }
    return nullptr;
}

template<class T>
T* LookupNodeByName(T& node, string_view name)
{
    BASE_NS::vector<T*> stack(1U, &node);
    while (!stack.empty()) {
        auto* current = stack.front();
        stack.erase(stack.cbegin());
        if (!current) {
            continue;
        }
        if (current->GetName() == name) {
            return current;
        }
        if (const auto& children = current->GetChildren(); !children.empty()) {
            stack.insert(stack.cend(), children.cbegin(), children.cend());
        }
    }

    return nullptr;
}

template<class T>
bool LookupNodesByComponent(
    T& node, const IComponentManager& componentManager, vector<T*>& results, bool singleNodeLookup)
{
    bool result = false;
    BASE_NS::vector<T*> stack(1U, &node);
    while (!stack.empty()) {
        auto* current = stack.front();
        stack.erase(stack.cbegin());
        if (!current) {
            continue;
        }
        if (componentManager.HasComponent(current->GetEntity())) {
            result = true;
            results.push_back(current);
            if (singleNodeLookup) {
                return result;
            }
        }
        if (auto children = current->GetChildren(); !children.empty()) {
            stack.insert(stack.cend(), children.cbegin(), children.cend());
        }
    }

    return result;
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
    virtual void SetEnabled(SceneNode& node, bool isEnabled) = 0;
    virtual bool GetEffectivelyEnabled(Entity entity) const = 0;
    virtual ISceneNode* GetParent(Entity entity) const = 0;
    virtual void SetParent(SceneNode& node, const ISceneNode& parent) = 0;

    virtual uint32_t GetSceneId(Entity entity) const = 0;
    virtual void SetSceneId(Entity entity, uint32_t sceneId) = 0;

    virtual void Notify(const ISceneNode& parent, NodeSystem::SceneNodeListener::EventType type,
        const ISceneNode& child, size_t index) = 0;

    virtual NodeSystem::SceneNode* GetNode(Entity const& entity) const = 0;
    virtual void Refresh() = 0;
};

// Implementation of the node interface.
class NodeSystem::SceneNode final : public ISceneNode {
public:
    struct NodeState {
        Entity parent { 0U };
        SceneNode* parentNode { nullptr };
        uint32_t localMatrixGeneration { 0U };
        uint16_t depth { 0U };
        bool enabled { false };
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
        nodeAccess_.SetEnabled(*this, isEnabled);
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

        nodeAccess_.SetParent(*this, node);
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
        const auto children = GetChildren();
        if (const auto pos = Find(children, &node); pos == children.end()) {
            if (const auto* oldParent = node.GetParent()) {
                const auto oldSiblings = oldParent->GetChildren();
                if (const auto oldPos = Find(oldSiblings, &node); oldPos != oldSiblings.end()) {
                    const auto oldIndex = static_cast<size_t>(oldPos - oldSiblings.begin());
                    nodeAccess_.Notify(*oldParent, INodeSystem::SceneNodeListener::EventType::REMOVED, node, oldIndex);
                }
            }
            nodeAccess_.SetParent(static_cast<SceneNode&>(node), *this);
            nodeAccess_.Notify(*this, INodeSystem::SceneNodeListener::EventType::ADDED, node, children_.size() - 1U);
            return true;
        }
        return false;
    }

    bool InsertChild(size_t index, ISceneNode& node) override
    {
        const auto children = GetChildren();
        if (auto pos = Find(children, &node); pos == children.cend()) {
            if (const auto* oldParent = node.GetParent()) {
                const auto oldSiblings = oldParent->GetChildren();
                if (const auto oldPos = Find(oldSiblings, &node); oldPos != oldSiblings.end()) {
                    const auto oldIndex = static_cast<size_t>(oldPos - oldSiblings.begin());
                    nodeAccess_.Notify(*oldParent, INodeSystem::SceneNodeListener::EventType::REMOVED, node, oldIndex);
                }
            }
            nodeAccess_.SetParent(static_cast<SceneNode&>(node), *this);
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
                nodeAccess_.SetParent(static_cast<SceneNode&>(node), *nodeAccess_.GetNode({}));
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
                    nodeAccess_.SetParent(*node, *nodeAccess_.GetNode({}));
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
                    nodeAccess_.SetParent(*node, *root);
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
            return CORE3D_NS::LookupNodeByPath<const ISceneNode>(*this, parts);
        }

        return nullptr;
    }

    ISceneNode* LookupNodeByPath(string_view const& path) override
    {
        // Split path to array of node names.
        vector<string_view> parts = StringUtil::Split(path, "/");
        if (parts.size() > 0) {
            // Perform lookup using array of names and 'current' index (start from zero).
            return CORE3D_NS::LookupNodeByPath<ISceneNode>(*this, parts);
        }

        return nullptr;
    }

    const ISceneNode* LookupNodeByName(string_view const& name) const override
    {
        return CORE3D_NS::LookupNodeByName<const ISceneNode>(*this, name);
    }

    ISceneNode* LookupNodeByName(string_view const& name) override
    {
        return CORE3D_NS::LookupNodeByName<ISceneNode>(*this, name);
    }

    const ISceneNode* LookupNodeByComponent(const IComponentManager& componentManager) const override
    {
        vector<const ISceneNode*> results;
        if (CORE3D_NS::LookupNodesByComponent<const ISceneNode>(*this, componentManager, results, true)) {
            return results[0];
        }

        return nullptr;
    }

    ISceneNode* LookupNodeByComponent(const IComponentManager& componentManager) override
    {
        vector<ISceneNode*> results;
        if (CORE3D_NS::LookupNodesByComponent<ISceneNode>(*this, componentManager, results, true)) {
            return results[0];
        }

        return nullptr;
    }

    vector<const ISceneNode*> LookupNodesByComponent(const IComponentManager& componentManager) const override
    {
        vector<const ISceneNode*> results;
        CORE3D_NS::LookupNodesByComponent<const ISceneNode>(*this, componentManager, results, false);
        return results;
    }

    vector<ISceneNode*> LookupNodesByComponent(const IComponentManager& componentManager) override
    {
        vector<ISceneNode*> results;
        CORE3D_NS::LookupNodesByComponent<ISceneNode>(*this, componentManager, results, false);
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

    uint32_t GetSceneId() const override
    {
        return nodeAccess_.GetSceneId(entity_);
    }

    void SetSceneId(uint32_t sceneId) override
    {
        nodeAccess_.SetSceneId(entity_, sceneId);
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
        if (auto pos = std::lower_bound(first, last, Entity()); (pos != last) && (*pos == Entity())) {
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
        auto pos = std::lower_bound(first, last, entity);
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
                result->lastState_.parentNode = parent;
                result->lastState_.depth = parent->lastState_.depth + 1U;
            }
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

    static void SetLevelTree(SceneNode* node, INodeComponentManager& nodeComponentManager, const uint32_t sceneId)
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
            ScopedHandle<NodeComponent>(childHandle)->sceneId = sceneId;
            stack.append(current->children_.cbegin(), current->children_.cend());
        }
    }

    void SetEnabled(SceneNode& node, const bool isEnabled) override
    {
        auto* handle = nodeComponentManager_.GetData(node.GetEntity());
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

        if (!isEnabled) {
            DisableTree(&node, nodeComponentManager_);
        } else if (auto parent = GetNode(nodeComponent.parent); !parent || !parent->GetEffectivelyEnabled()) {
            // if the node's parent is disabled, there's no need to update the tree.
            return;
        } else {
            EnableTree(&node, nodeComponentManager_);
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
        if (auto node = GetNode(entity)) {
            if (node->lastState_.parentNode) {
                return node->lastState_.parentNode;
            }
            return GetNode(node->lastState_.parent);
        }

        Entity parent;
        if (ScopedHandle<const NodeComponent> data = nodeComponentManager_.Read(entity); data) {
            parent = data->parent;
        }

        return GetNode(parent);
    }

    void SetParent(SceneNode& node, const ISceneNode& parent) override
    {
        if (&node == &parent) {
            return;
        }
        Entity oldParent;
        const auto newParent = parent.GetEntity();
        uint32_t oldLevel = 0U;
        const uint32_t newLevel = parent.GetSceneId();
        const auto effectivelyEnabled = parent.GetEffectivelyEnabled();
        if (ScopedHandle<NodeComponent> data = nodeComponentManager_.Write(node.GetEntity())) {
            oldParent = data->parent;
            data->parent = newParent;
            data->effectivelyEnabled = effectivelyEnabled;
            oldLevel = data->sceneId;
            data->sceneId = newLevel;
        }
        UpdateParent(node, oldParent, newParent);
        if (oldLevel != newLevel) {
            for (auto* child : node.GetChildren()) {
                if (!child) {
                    continue;
                }
                SetLevelTree(static_cast<SceneNode*>(child), nodeComponentManager_, newLevel);
            }
        }
    }

    uint32_t GetSceneId(const Entity entity) const override
    {
        if (ScopedHandle<const NodeComponent> data = nodeComponentManager_.Read(entity); data) {
            return data->sceneId;
        }
        return 0U;
    }

    void SetSceneId(const Entity entity, const uint32_t sceneId) override
    {
        auto node = GetNode(entity);
        if (!node) {
            return;
        }
        const auto nodeGeneration = nodeComponentManager_.GetGenerationCounter();
        SetLevelTree(node, nodeComponentManager_, sceneId);
        if (nodeComponentGenerationId_ == nodeGeneration) {
            nodeComponentGenerationId_ = nodeComponentManager_.GetGenerationCounter();
        }
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

    void UpdateParent(SceneNode& node, Entity oldParent, Entity newParent)
    {
        const uint32_t nodeGenerationId = nodeComponentManager_.GetGenerationCounter();
        if (nodeGenerationId <= (nodeComponentGenerationId_ + 1U)) {
            nodeComponentGenerationId_ = nodeGenerationId;

            if (SceneNode* parent = GetNode(oldParent)) {
                parent->children_.erase(
                    std::remove(parent->children_.begin(), parent->children_.end(), &node), parent->children_.cend());
                node.lastState_.parent = {};
                node.lastState_.parentNode = nullptr;
                node.lastState_.depth = 0U;
            }
            if (SceneNode* parent = GetNode(newParent); parent != &node) {
                // Set parent / child relationship.
                parent->children_.push_back(&node);
                node.lastState_.parent = newParent;
                node.lastState_.parentNode = parent;
                node.lastState_.depth = parent->lastState_.depth + 1U;
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

#if (CORE3D_DEV_ENABLED == 1)
        CORE_CPU_PERF_SCOPE("CORE3D", "NodeSystem", "NodeCache::Refresh", CORE3D_PROFILER_DEFAULT_COLOR);
#endif
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

        std::sort(first, last);

        // there's at least the root (invalid/default constructed entity)
        entitiesWithNode.insert(first, Entity());
        last = entitiesWithNode.end();

        // Look up entities that no longer exist.
        auto inserter = std::back_inserter(entitiesWithNode);
        inserter = std::set_difference(nodeEntities_.cbegin(), nodeEntities_.cend(), first, last, inserter);

        auto lastRemoved = entitiesWithNode.end();

        // Look up entities that have been added.
        inserter = std::set_difference(first, last, nodeEntities_.cbegin(), nodeEntities_.cend(), inserter);
        auto lastAdded = entitiesWithNode.end();

        // Remove !alive entities and those without node component
        std::for_each(last, lastRemoved, [this](Entity oldEntity) {
            const auto first = nodeEntities_.cbegin();
            const auto last = nodeEntities_.cend();
            if (auto pos = std::lower_bound(first, last, oldEntity); (pos != last) && (*pos == oldEntity)) {
                const auto index = pos - first;
                // detach the node from its children and parent before cleanup
                auto* node = static_cast<SceneNode*>(nodes_[static_cast<size_t>(index)].get());
                for (auto* child : node->children_) {
                    child->lastState_.parent = {};
                    child->lastState_.parentNode = nullptr;
                    child->lastState_.depth = 0U;
                }
                auto* parent = node->lastState_.parentNode;
                if (!parent) {
                    parent = GetNode(node->lastState_.parent);
                }
                if (parent) {
                    auto& children = parent->children_;
                    children.erase(std::remove(children.begin(), children.end(), node), children.cend());
                }
                // can we rely on lastState_.parent, or also check from node component?
                if (ScopedHandle<const NodeComponent> data = nodeComponentManager_.Read(oldEntity)) {
                    Entity parentEntity = node->lastState_.parent;
                    if (parentEntity != data->parent) {
                        parent = GetNode(parentEntity);
                        if (parent) {
                            auto& children = parent->children_;
                            children.erase(std::remove(children.begin(), children.end(), node), children.cend());
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
                        sceneNode->lastState_.parentNode = {};
                        sceneNode->lastState_.depth = 0U;
                        if (SceneNode* newParent = GetNode(nodeComponent->parent)) {
                            // Set parent / child relationship.
                            if (std::none_of(newParent->children_.cbegin(), newParent->children_.cend(),
                                    [sceneNode](const SceneNode* childNode) { return childNode == sceneNode; })) {
                                newParent->children_.push_back(sceneNode);
                            }
                            sceneNode->lastState_.parent = nodeComponent->parent;
                            sceneNode->lastState_.parentNode = newParent;
                            sceneNode->lastState_.depth = newParent->lastState_.depth + 1U;
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
        // Find a leaf
        if (auto* child = node->PopChildNoRefresh()) {
            node = child;
            continue;
        }
        // The current node will be destroyed and we continue from its parent.
        auto* destroy = std::exchange(node, cache_->GetParentNoRefresh(*node));
        if (destroy != &rootNode) {
            destroy->lastState_.parent = {};
            destroy->lastState_.parentNode = nullptr;
            destroy->lastState_.depth = 0U;
            // If this isn't the starting point we just destroy the entity and continue from the parent.
            entityManager.Destroy(destroy->GetEntity());
            continue;
        }
        // The entities in this hierarchy have been destroyed.
        done = true;
        if (node) {
            //  Detach the starting point from its parent.
            node->children_.erase(
                std::remove(node->children_.begin(), node->children_.end(), destroy), node->children_.cend());
            destroy->lastState_.parent = {};
            destroy->lastState_.parentNode = {};
            destroy->lastState_.depth = 0U;
        }
        // Check this is not the root node which cannot be destroyed.
        const auto entity = destroy->GetEntity();
        if (EntityUtil::IsValid(entity)) {
            entityManager.Destroy(entity);
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

void NodeSystem::OnComponentEvent(IEcs::ComponentListener::EventType type, const IComponentManager& componentManager,
    array_view<const Entity> entities)
{
    if (componentManager.GetUid() == ITransformComponentManager::UID) {
        switch (type) {
            case EventType::CREATED:
                CORE_PROFILER_PLOT("NewTransform", static_cast<int64_t>(entities.size()));
                modifiedEntities_.append(entities.cbegin(), entities.cend());
                break;
            case EventType::MODIFIED:
                CORE_PROFILER_PLOT("UpdateTransform", static_cast<int64_t>(entities.size()));
                modifiedEntities_.append(entities.cbegin(), entities.cend());
                break;
            case EventType::DESTROYED:
                CORE_PROFILER_PLOT("DeleteTransform", static_cast<int64_t>(entities.size()));
                break;
            case EventType::MOVED:
                // Not using directly with ComponentIds.
                break;
        }
    } else if (componentManager.GetUid() == ILocalMatrixComponentManager::UID) {
        switch (type) {
            case EventType::CREATED:
                CORE_PROFILER_PLOT("NewLocalMatrix", static_cast<int64_t>(entities.size()));
                modifiedEntities_.append(entities.cbegin(), entities.cend());
                break;
            case EventType::MODIFIED:
                CORE_PROFILER_PLOT("UpdateLocalMatrix", static_cast<int64_t>(entities.size()));
                modifiedEntities_.append(entities.cbegin(), entities.cend());
                break;
            case EventType::DESTROYED:
                CORE_PROFILER_PLOT("DeleteLocalMatrix", static_cast<int64_t>(entities.size()));
                break;
            case EventType::MOVED:
                // Not using directly with ComponentIds.
                break;
        }
    } else if (componentManager.GetUid() == INodeComponentManager::UID) {
        switch (type) {
            case EventType::CREATED:
                CORE_PROFILER_PLOT("NewNode", static_cast<int64_t>(entities.size()));
                modifiedEntities_.append(entities.cbegin(), entities.cend());
                break;
            case EventType::MODIFIED:
                CORE_PROFILER_PLOT("UpdateNode", static_cast<int64_t>(entities.size()));
                modifiedEntities_.append(entities.cbegin(), entities.cend());
                break;
            case EventType::DESTROYED:
                CORE_PROFILER_PLOT("DeleteNode", static_cast<int64_t>(entities.size()));
                break;
            case EventType::MOVED:
                // Not using directly with ComponentIds.
                break;
        }
    }
}

void NodeSystem::Initialize()
{
    ComponentQuery::Operation operations[] = {
        { localMatrixManager_, ComponentQuery::Operation::Method::OPTIONAL },
        { worldMatrixManager_, ComponentQuery::Operation::Method::OPTIONAL },
    };
    CORE_ASSERT(&operations[LOCAL_INDEX - 1U].target == &localMatrixManager_);
    CORE_ASSERT(&operations[WORLD_INDEX - 1U].target == &worldMatrixManager_);
    nodeQuery_.SetupQuery(nodeManager_, operations, true);
    nodeQuery_.SetEcsListenersEnabled(true);
    ecs_.AddListener(nodeManager_, *this);
    ecs_.AddListener(transformManager_, *this);
    ecs_.AddListener(localMatrixManager_, *this);
}

bool NodeSystem::Update(bool, uint64_t, uint64_t)
{
    if (!active_) {
        return false;
    }

    nodeQuery_.Execute();

    // Cache world matrices from previous frame.
    UpdatePreviousWorldMatrices();

    if (localMatrixGeneration_ == localMatrixManager_.GetGenerationCounter() &&
        nodeGeneration_ == nodeManager_.GetGenerationCounter()) {
        return false;
    }

    // Make sure node cache is valid.
    cache_->Refresh();

    // Find all parent nodes that have their transform updated.
    const vector<ISceneNode*> changedNodes = CollectChangedNodes();

    // Update world transformations for changed tree branches. Remember parent as nodes are sorted according to depth
    // and parent so we don't have to that often fetch the information.
    const auto* root = &GetRootNode();
    bool parentEnabled = true;
    Math::Mat4X4 parentMatrix(Math::IDENTITY_4X4);
    const ISceneNode* parent = nullptr;
    for (const auto node : changedNodes) {
        const ISceneNode* nodeParent = node->GetParent();
        if (nodeParent && nodeParent != parent) {
            parent = nodeParent;
            // Get parent world matrix.
            if (parent == root) {
                // As the root node isn't a real entity with components it won't be available in nodeQuery_. Instead we
                // just reset enable and matrix information.
                parentEnabled = true;
                parentMatrix = Math::IDENTITY_4X4;
            } else if (auto row = nodeQuery_.FindResultRow(parent->GetEntity()); row) {
                if (row->IsValidComponentId(WORLD_INDEX)) {
                    parentMatrix = worldMatrixManager_.Get(row->components[WORLD_INDEX]).matrix;
                }
                if (auto nodeHandle = nodeManager_.Read(row->components[NODE_INDEX])) {
                    parentEnabled = nodeHandle->effectivelyEnabled;
                } else {
#if (CORE3D_VALIDATION_ENABLED == 1)
                    CORE_LOG_W("%" PRIx64 " missing Node", row->entity.id);
#endif
                }
            } else {
#if (CORE3D_VALIDATION_ENABLED == 1)
                CORE_LOG_W("Parent %" PRIx64 " not found from component query", parent->GetEntity().id);
#endif
                parentEnabled = true;
                parentMatrix = Math::IDENTITY_4X4;
            }
        } else if (!nodeParent) {
            parentEnabled = true;
            parentMatrix = Math::IDENTITY_4X4;
            parent = nullptr;
        }

        UpdateTransformations(*node, parentMatrix, parentEnabled);
    }

    // Store generation counters.
    localMatrixGeneration_ = localMatrixManager_.GetGenerationCounter();
    nodeGeneration_ = nodeManager_.GetGenerationCounter();

    return true;
}

void NodeSystem::Uninitialize()
{
    cache_->Reset();
    ecs_.RemoveListener(nodeManager_, *this);
    ecs_.RemoveListener(transformManager_, *this);
    ecs_.RemoveListener(localMatrixManager_, *this);
    nodeQuery_.SetEcsListenersEnabled(false);
}

vector<ISceneNode*> NodeSystem::CollectChangedNodes()
{
    vector<ISceneNode*> result;
#if (CORE3D_DEV_ENABLED == 1)
    CORE_CPU_PERF_SCOPE("CORE3D", "NodeSystem", "CollectChangedNodes", CORE3D_PROFILER_DEFAULT_COLOR);
#endif

    if (modifiedEntities_.empty()) {
        return result;
    }

    // sort enities and remove duplicates
    std::sort(modifiedEntities_.begin().ptr(), modifiedEntities_.end().ptr(), std::less {});
    modifiedEntities_.erase(
        decltype(modifiedEntities_)::const_iterator(std::unique(modifiedEntities_.begin(), modifiedEntities_.end())),
        modifiedEntities_.cend());

    // fetch SceneNode for each entity
    result.reserve(modifiedEntities_.size());
    for (const auto& entity : modifiedEntities_) {
        if (auto modifiedNode = GetNode(entity)) {
            result.push_back(modifiedNode);
        }
    }
    modifiedEntities_.clear();

    // sort SceneNodes according to depth, parent and entity id.
    std::sort(result.begin().ptr(), result.end().ptr(), [](const ISceneNode* lhs, const ISceneNode* rhs) {
        if (static_cast<const SceneNode*>(lhs)->lastState_.depth <
            static_cast<const SceneNode*>(rhs)->lastState_.depth) {
            return true;
        }
        if (static_cast<const SceneNode*>(lhs)->lastState_.depth >
            static_cast<const SceneNode*>(rhs)->lastState_.depth) {
            return false;
        }
        if (static_cast<const SceneNode*>(lhs)->lastState_.parent <
            static_cast<const SceneNode*>(rhs)->lastState_.parent) {
            return true;
        }
        if (static_cast<const SceneNode*>(lhs)->lastState_.parent >
            static_cast<const SceneNode*>(rhs)->lastState_.parent) {
            return false;
        }
        if (static_cast<const SceneNode*>(lhs)->entity_ < static_cast<const SceneNode*>(rhs)->entity_) {
            return true;
        }
        if (static_cast<const SceneNode*>(lhs)->entity_ > static_cast<const SceneNode*>(rhs)->entity_) {
            return false;
        }
        return false;
    });
    return result;
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
#if (CORE3D_DEV_ENABLED == 1)
    CORE_CPU_PERF_SCOPE("CORE3D", "NodeSystem", "UpdateTransformations", CORE3D_PROFILER_DEFAULT_COLOR);
#endif
    stack_.clear();
    stack_.reserve(nodeManager_.GetComponentCount());
    stack_.push_back(State { static_cast<SceneNode*>(&node), matrix, enabled });
    while (!stack_.empty()) {
        auto state = stack_.back();
        stack_.pop_back();

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
#if (CORE3D_VALIDATION_ENABLED == 1)
                CORE_LOG_W("%" PRIx64 " missing LocalWorldMatrix", row->entity.id);
#endif
            }

            if (row->IsValidComponentId(WORLD_INDEX)) {
                if (auto worldMatrixHandle = worldMatrixManager_.Write(row->components[WORLD_INDEX])) {
                    worldMatrixHandle->matrix = pm;
                } else {
#if (CORE3D_VALIDATION_ENABLED == 1)
                    CORE_LOG_W("%" PRIx64 " missing WorldMatrix", row->entity.id);
#endif
                }
            } else {
                worldMatrixManager_.Set(row->entity, { pm });
            }

            // Save the values that were used to calculate current world matrix.
            state.node->lastState_.localMatrixGeneration =
                localMatrixManager_.GetComponentGeneration(row->components[LOCAL_INDEX]);
            if (state.node->lastState_.parent != nodeInfo.parent) {
                state.node->lastState_.parent = nodeInfo.parent;
                state.node->lastState_.parentNode = static_cast<SceneNode*>(GetNode(nodeInfo.parent));
            }
        }
        if (nodeInfo.isEffectivelyEnabled || nodeInfo.effectivelyEnabledChanged) {
            for (auto* child : state.node->GetChildren()) {
                if (child) {
                    stack_.push_back(State { static_cast<SceneNode*>(child), pm, nodeInfo.isEffectivelyEnabled });
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

void NodeSystem::UpdatePreviousWorldMatrices()
{
#if (CORE3D_DEV_ENABLED == 1)
    CORE_CPU_PERF_SCOPE("CORE3D", "NodeSystem", "UpdatePreviousWorldMatrices", CORE3D_PROFILER_DEFAULT_COLOR);
#endif
    if (worldMatrixGeneration_ != worldMatrixManager_.GetGenerationCounter()) {
        const auto components = worldMatrixManager_.GetComponentCount();
        for (IComponentManager::ComponentId id = 0U; id < components; ++id) {
            auto handle = worldMatrixManager_.GetData(id);
            if (auto comp = ScopedHandle<const WorldMatrixComponent>(*handle)) {
                if (comp->prevMatrix == comp->matrix) {
                    continue;
                }
            }
            if (auto comp = ScopedHandle<WorldMatrixComponent>(*handle)) {
                comp->prevMatrix = comp->matrix;
            }
        }
        worldMatrixGeneration_ = worldMatrixManager_.GetGenerationCounter();
    }
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
