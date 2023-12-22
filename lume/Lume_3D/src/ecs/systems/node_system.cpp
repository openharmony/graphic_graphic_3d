/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
    auto children = node.GetChildren();

    // Loop through all childs, see if there is a node with given name.
    // If found, then recurse in to that node branch and see if lookup is able to complete in that branch.
    for (size_t i = 0; i < children.size(); i++) {
        T* child = children[i];
        if (child->GetName() == path[index]) {
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
    auto children = node.GetChildren();

    // Loop through all childs, see if there is a node with given name.
    for (size_t i = 0; i < children.size(); i++) {
        T* child = children[i];

        // Continue lookup recursively.
        T* result = RecursivelyLookupNodeByName(*child, name);
        if (result) {
            // We have a hit.
            return result;
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
    auto children = node.GetChildren();

    // Loop through all childs, see if there is a node with given name.
    // If found, then recurse in to that node branch and see if lookup is able to complete in that branch.
    for (size_t i = 0; i < children.size(); i++) {
        T* child = children[i];

        // Continue lookup recursively.
        if (RecursivelyLookupNodesByComponent(*child, componentManager, results, singleNodeLookup)) {
            result = true;
            if (singleNodeLookup) {
                return result;
            }
        }
    }

    return result;
}
} // namespace

// Interface that allows nodes to access other nodes and request cache updates.
class NodeSystem::NodeAccess {
public:
    virtual ~NodeAccess() = default;

    virtual NodeSystem::SceneNode* GetNode(Entity const& entity) const = 0;
    virtual void Refresh() = 0;
};

// Implementation of the node interface.
class NodeSystem::SceneNode : public ISceneNode {
public:
    struct NodeState {
        uint32_t localMatrixGeneration { 0 };
        Entity parent {};
        bool enabled { false };
    };

    SceneNode(const SceneNode& other) = delete;
    SceneNode& operator=(const SceneNode& node) = delete;

    SceneNode(Entity entity, NodeAccess& nodeAccess, INameComponentManager& nameComponentManager,
        INodeComponentManager& nodeComponentManager, ITransformComponentManager& transformComponentManager)
        : entity_(entity), nodeAccess_(nodeAccess), nameManager_(nameComponentManager),
          nodeManager_(nodeComponentManager), transformManager_(transformComponentManager)
    {}

    ~SceneNode() override = default;

    string GetName() const override
    {
        if (const auto nameId = nameManager_.GetComponentId(entity_);
            nameId != IComponentManager::INVALID_COMPONENT_ID) {
            return nameManager_.Get(entity_).name;
        } else {
            return "";
        }
    }

    void SetName(const string_view name) override
    {
        if (ScopedHandle<NameComponent> data = nameManager_.Write(entity_); data) {
            data->name = name;
        }
    }

    void SetEnabled(bool isEnabled) override
    {
        if (ScopedHandle<NodeComponent> data = nodeManager_.Write(entity_); data) {
            data->enabled = isEnabled;
        }
    }

    bool GetEnabled() const override
    {
        bool enabled = true;
        if (ScopedHandle<const NodeComponent> data = nodeManager_.Read(entity_); data) {
            enabled = data->enabled;
        }
        return enabled;
    }

    bool GetEffectivelyEnabled() const override
    {
        bool effectivelyEnabled = true;
        if (ScopedHandle<const NodeComponent> data = nodeManager_.Read(entity_); data) {
            effectivelyEnabled = data->effectivelyEnabled;
        }
        return effectivelyEnabled;
    }

    ISceneNode* GetParent() const override
    {
        if (!EntityUtil::IsValid(entity_)) {
            return nullptr;
        }

        nodeAccess_.Refresh();

        Entity parent;
        if (ScopedHandle<const NodeComponent> data = nodeManager_.Read(entity_); data) {
            parent = data->parent;
        }

        return nodeAccess_.GetNode(parent);
    }

    void SetParent(ISceneNode const& node) override
    {
        // Ensure we are not ancestors of the new parent.
        CORE_ASSERT(IsAncestorOf(node) == false);
        if (ScopedHandle<NodeComponent> data = nodeManager_.Write(entity_); data) {
            data->parent = node.GetEntity();
        }
    }

    bool IsAncestorOf(ISceneNode const& node) override
    {
        ISceneNode const* curNode = &node;
        while (curNode != nullptr) {
            if (curNode == this) {
                return true;
            }

            curNode = curNode->GetParent();
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

        for (size_t i = 0; i < children.size(); i++) {
            const ISceneNode* child = children[i];
            if (child->GetName() == name) {
                return child;
            }
        }

        return nullptr;
    }

    ISceneNode* GetChild(string_view const& name) override
    {
        // Access to children will automatically refresh cache.
        auto children = GetChildren();

        for (size_t i = 0; i < children.size(); i++) {
            ISceneNode* child = children[i];
            if (child->GetName() == name) {
                return child;
            }
        }

        return nullptr;
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
        if (const auto nameId = transformManager_.GetComponentId(entity_);
            nameId != IComponentManager::INVALID_COMPONENT_ID) {
            return transformManager_.Get(entity_).position;
        } else {
            return Math::Vec3();
        }
    }

    Math::Quat GetRotation() const override
    {
        if (const auto nameId = transformManager_.GetComponentId(entity_);
            nameId != IComponentManager::INVALID_COMPONENT_ID) {
            return transformManager_.Get(entity_).rotation;
        } else {
            return Math::Quat();
        }
    }

    Math::Vec3 GetScale() const override
    {
        if (const auto nameId = transformManager_.GetComponentId(entity_);
            nameId != IComponentManager::INVALID_COMPONENT_ID) {
            return transformManager_.Get(entity_).scale;
        } else {
            return Math::Vec3 { 1.0f, 1.0f, 1.0f };
        }
    }

    void SetScale(const Math::Vec3& scale) override
    {
        if (ScopedHandle<TransformComponent> data = transformManager_.Write(entity_); data) {
            data->scale = scale;
        }
    }

    void SetPosition(const Math::Vec3& position) override
    {
        if (ScopedHandle<TransformComponent> data = transformManager_.Write(entity_); data) {
            data->position = position;
        }
    }

    void SetRotation(const Math::Quat& rotation) override
    {
        if (ScopedHandle<TransformComponent> data = transformManager_.Write(entity_); data) {
            data->rotation = rotation;
        }
    }

    // Internally for NodeSystem to skip unneccessary NodeCache::Refresh calls
    SceneNode* GetParentNoRefresh() const
    {
        Entity parent;
        if (ScopedHandle<const NodeComponent> data = nodeManager_.Read(entity_); data) {
            parent = data->parent;
        }

        if (EntityUtil::IsValid(parent)) {
            return nodeAccess_.GetNode(parent);
        } else {
            return nullptr;
        }
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
    NodeState lastState_ {};

    NodeAccess& nodeAccess_;
    INameComponentManager& nameManager_;
    INodeComponentManager& nodeManager_;
    ITransformComponentManager& transformManager_;

    vector<SceneNode*> children_;

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
        auto rootNode = nodes_.extract(Entity());
        nodes_.clear();
        nodes_.insert(move(rootNode));
    }

    SceneNode* AddNode(Entity const& entity)
    {
        SceneNode* result;

        auto node = make_unique<SceneNode>(
            entity, *this, nameComponentManager_, nodeComponentManager_, transformComponentManager_);
        result = node.get();

        nodes_[entity] = move(node);

        return result;
    }

    SceneNode* GetNode(Entity const& entity) const override
    {
        const auto it = nodes_.find(entity);
        if (it != nodes_.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    void Refresh() override
    {
        // If no modifications, then no need to refresh.
        const uint32_t nodeGenerationId = nodeComponentManager_.GetGenerationCounter();
        const uint32_t entityGenerationId = entityManager_.GetGenerationCounter();
        if (nodeGenerationId == nodeComponentGenerationId_ && entityGenerationId == entityGenerationId_) {
            return;
        }

        // Look up entities that no longer exist.
        for (NodeCache::Nodes::const_iterator it = nodes_.begin(), end = nodes_.end(); it != end;) {
            SceneNode* node = it->second.get();

            // Reset.
            node->children_.clear();
            // Do not destroy root.
            if (!EntityUtil::IsValid(node->GetEntity())) {
                ++it;
                continue;
            }

            if (!entityManager_.IsAlive(node->GetEntity()) || !nodeComponentManager_.HasComponent(node->GetEntity())) {
                // This node no longer exists.
                it = nodes_.erase(it);
            } else {
                ++it;
            }
        }

        // Add entities that appeared since last refresh.
        for (IComponentManager::ComponentId i = 0; i < nodeComponentManager_.GetComponentCount(); i++) {
            const Entity entity = nodeComponentManager_.GetEntity(i);
            if (!GetNode(entity) && entityManager_.IsAlive(entity)) {
                AddNode(entity);
            }
        }

        for (IComponentManager::ComponentId i = 0; i < nodeComponentManager_.GetComponentCount(); i++) {
            // Get associated entity.
            const Entity entity = nodeComponentManager_.GetEntity(i);
            if (entityManager_.IsAlive(entity)) {
                // Get node component.
                const Entity parentEntity = nodeComponentManager_.Read(i)->parent;

                // Get node and it's parent.
                SceneNode* parent = GetNode(parentEntity);
                if (parent) {
                    // Set parent / child relationship.
                    SceneNode* node = GetNode(entity);
                    parent->children_.push_back(node);
                }
            }
        }

        nodeComponentGenerationId_ = nodeGenerationId;
        entityGenerationId_ = entityGenerationId;
    }

private:
    using Nodes = unordered_map<Entity, unique_ptr<SceneNode>>;

    Nodes nodes_;
    uint32_t nodeComponentGenerationId_ = { 0 };
    uint32_t entityGenerationId_ = { 0 };

    INameComponentManager& nameComponentManager_;
    INodeComponentManager& nodeComponentManager_;
    ITransformComponentManager& transformComponentManager_;
    IEntityManager& entityManager_;
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
    // Make sure node cache is valid.
    cache_->Refresh();

    return cache_->GetNode(entity);
}

ISceneNode* NodeSystem::CreateNode()
{
    const Entity entity = ecs_.GetEntityManager().Create();

    nodeManager_.Create(entity);
    nameManager_.Create(entity);
    transformManager_.Create(entity);
    localMatrixManager_.Create(entity);
    worldMatrixManager_.Create(entity);

    return GetNode(entity);
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
            const auto* destroy = std::exchange(node, node->GetParentNoRefresh());
            if (destroy == &rootNode) {
                done = true;
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

    // Make sure node cache is valid.
    cache_->Refresh();

    nodeQuery_.Execute();

    // Cache world matrices from previous frame.
    const bool missingPrevWorldMatrix = UpdatePreviousWorldMatrices();

    if (localMatrixGeneration_ == localMatrixManager_.GetGenerationCounter() &&
        nodeGeneration_ == nodeManager_.GetGenerationCounter()) {
        return false;
    }

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

                parentEnabled = nodeManager_.Read(row->components[NODE_INDEX])->effectivelyEnabled;
            }
        }

        UpdateTransformationRecursive(*node, parentMatrix, parentEnabled);
    }

    // Store generation counters.
    localMatrixGeneration_ = localMatrixManager_.GetGenerationCounter();
    nodeGeneration_ = nodeManager_.GetGenerationCounter();

    if (missingPrevWorldMatrix) {
        for (const auto& row : nodeQuery_.GetResults()) {
            // Create missing PreviousWorldMatrixComponents and initialize with current world.
            if (!row.IsValidComponentId(PREV_WORLD_INDEX) && row.IsValidComponentId(WORLD_INDEX)) {
                prevWorldMatrixManager_.Create(row.entity);
                prevWorldMatrixManager_.Write(row.entity)->matrix =
                    worldMatrixManager_.Read(row.components[WORLD_INDEX])->matrix;
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
        {
            const auto& nodeComponent = nodeManager_.Read(row->components[NODE_INDEX]);
            const bool parentChanged = lastState.parent != nodeComponent->parent;
            const bool enabledChanged = lastState.enabled != nodeComponent->enabled;
            if (parentChanged || enabledChanged) {
                result.push_back(&node);
                return;
            }
        }
    }
    auto const& children = node.GetChildren();
    for (size_t i = 0; i < children.size(); ++i) {
        CollectChangedNodes(*(children[i]), result);
    }
}

void NodeSystem::UpdateTransformationRecursive(ISceneNode& node, Math::Mat4X4 const& matrix, bool enabled)
{
    SceneNode& sceneNode = (SceneNode&)(node);

    if (auto row = nodeQuery_.FindResultRow(sceneNode.GetEntity()); row) {
        bool isEffectivelyEnabled = enabled;
        Entity parent;
        {
            IPropertyHandle* handle = nodeManager_.GetData(row->components[NODE_INDEX]);
            auto nc = ScopedHandle<const NodeComponent>(handle);
            parent = nc->parent;
            const bool nodeEnabled = nc->enabled;
            // Update effectively enabled status if it has changed (e.g. due to parent enabled state changes).
            isEffectivelyEnabled = nodeEnabled && enabled;
            if (nc->effectivelyEnabled != isEffectivelyEnabled) {
                ScopedHandle<NodeComponent>(handle)->effectivelyEnabled = isEffectivelyEnabled;
            }
            sceneNode.lastState_.enabled = nodeEnabled;
        }
        Math::Mat4X4 pm = matrix;

        if (isEffectivelyEnabled && row->IsValidComponentId(LOCAL_INDEX)) {
            pm = pm * localMatrixManager_.Read(row->components[LOCAL_INDEX])->matrix;
            if (row->IsValidComponentId(WORLD_INDEX)) {
                auto worldMatrixHandle = worldMatrixManager_.Write(row->components[WORLD_INDEX]);
                worldMatrixHandle->matrix = pm;
            } else {
                worldMatrixManager_.Set(row->entity, { pm });
            }

            // Save the values that were used to calculate current world matrix.
            sceneNode.lastState_.localMatrixGeneration =
                localMatrixManager_.GetComponentGeneration(row->components[LOCAL_INDEX]);
            sceneNode.lastState_.parent = parent;
        }

        for (auto const& child : sceneNode.GetChildren()) {
            UpdateTransformationRecursive(*child, pm, isEffectivelyEnabled);
        }
    }
}

void NodeSystem::GatherNodeEntities(const ISceneNode& node, vector<Entity>& entities) const
{
    entities.push_back(node.GetEntity());
    for (const ISceneNode* child : node.GetChildren()) {
        GatherNodeEntities(*child, entities);
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
                prevWorldMatrixManager_.Write(row.components[PREV_WORLD_INDEX])->matrix =
                    worldMatrixManager_.Read(row.components[WORLD_INDEX])->matrix;
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
