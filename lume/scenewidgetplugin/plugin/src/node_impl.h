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
#ifndef NODE_IMPL_H_
#define NODE_IMPL_H_
#include <algorithm>
#include <scene_plugin/api/node_uid.h>
#include <scene_plugin/interface/intf_environment.h>

#include <meta/ext/object_container.h>
#include <meta/interface/intf_containable.h>

#include "intf_node_private.h"
#include "PropertyHandlerArrayHolder.h"

class NodeImpl : public META_NS::ObjectContainerFwd<NodeImpl, SCENE_NS::ClassId::Node, SCENE_NS::INode,
                     META_NS::IContainable, META_NS::IMutableContainable, INodeEcsInterfacePrivate,
                     SCENE_NS::IEcsObject, SCENE_NS::IProxyObject/*, UI_NS::Input::IInputFilter*/> {
    using Super = META_NS::ObjectContainerFwd<NodeImpl, SCENE_NS::ClassId::Node, SCENE_NS::INode, META_NS::IContainable,
        META_NS::IMutableContainable, INodeEcsInterfacePrivate, SCENE_NS::IEcsObject, SCENE_NS::IProxyObject/*,
        UI_NS::Input::IInputFilter*/>;

public:
    META_IMPLEMENT_INTERFACE_PROPERTY(META_NS::INamed, BASE_NS::string, Name, {})
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(SCENE_NS::INode, BASE_NS::string, Path, {}, META_NS::ObjectFlagBits::INTERNAL)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::INode, BASE_NS::Math::Vec3, Position,
        BASE_NS::Math::Vec3(0.f, 0.f, 0.f))
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::INode, BASE_NS::Math::Quat, Rotation,
        BASE_NS::Math::Quat(0.f, 0.f, 0.f, 1.f))
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::INode, BASE_NS::Math::Vec3, Scale, BASE_NS::Math::Vec3(1.f, 1.f, 1.f))
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::INode, bool, Visible, true)
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::INode, uint64_t, LayerMask, (1ULL << 0U))
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(SCENE_NS::INode, uint8_t, Status, SCENE_NS::INode::NODE_STATUS_UNINITIALIZED, META_NS::ObjectFlagBits::INTERNAL)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::INode, BASE_NS::Math::Mat4X4, LocalMatrix, BASE_NS::Math::IDENTITY_4X4,
        META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(SCENE_NS::INode, uint8_t, ConnectionStatus,
        SCENE_NS::IEcsObject::ECS_OBJ_STATUS_DISCONNECTED, META_NS::ObjectFlagBits::INTERNAL)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::INode, SCENE_NS::IMesh::Ptr, Mesh, {})

    META_IMPLEMENT_EVENT(META_NS::IOnChanged, OnLoaded)
    META_IMPLEMENT_EVENT(META_NS::IOnChanged, OnBound)

protected:
    using Fwd = META_NS::ObjectContainerFwd<NodeImpl, SCENE_NS::ClassId::Node, SCENE_NS::INode, META_NS::IContainable,
        META_NS::IMutableContainable, INodeEcsInterfacePrivate, SCENE_NS::IEcsObject, SCENE_NS::IProxyObject/*,
        UI_NS::Input::IInputFilter*/>;

/*public: // IInputFilter
    META_IMPLEMENT_INTERFACE_PROPERTY(
        UI_NS::Input::IInputFilter, UI_NS::Input::InputMode, InputMode, UI_NS::Input::InputMode::RECEIVE)*/

protected:
    static constexpr BASE_NS::string_view TRANSFORM_COMPONENT_NAME = "TransformComponent";
    static constexpr size_t TRANSFORM_COMPONENT_NAME_LEN = TRANSFORM_COMPONENT_NAME.size() + 1;
    static constexpr BASE_NS::string_view TRANSFORM_POSITION = "TransformComponent.position";
    static constexpr BASE_NS::string_view TRANSFORM_SCALE = "TransformComponent.scale";
    static constexpr BASE_NS::string_view TRANSFORM_ROTATION = "TransformComponent.rotation";
    static constexpr BASE_NS::string_view NODE_COMPONENT_NAME = "NodeComponent";
    static constexpr size_t NODE_COMPONENT_NAME_LEN = NODE_COMPONENT_NAME.size() + 1;
    static constexpr BASE_NS::string_view NODE_ENABLED = "NodeComponent.enabled";
    static constexpr BASE_NS::string_view LAYER_COMPONENT_NAME = "LayerComponent";
    static constexpr size_t LAYER_COMPONENT_NAME_LEN = LAYER_COMPONENT_NAME.size() + 1;
    static constexpr BASE_NS::string_view LAYER_MASK = "LayerComponent.layerMask";
    static constexpr BASE_NS::string_view LMATRIX_COMPONENT_NAME = "LocalMatrixComponent";
    static constexpr size_t LMATRIX_COMPONENT_NAME_LEN = LMATRIX_COMPONENT_NAME.size() + 1;
    static constexpr BASE_NS::string_view LMATRIX_MATRIX = "LocalMatrixComponent.matrix";
    static constexpr BASE_NS::string_view ENVIRONMENT_COMPONENT_NAME = "EnvironmentComponent";
    static constexpr BASE_NS::string_view RM_COMPONENT_NAME = "RenderMeshComponent";
    static constexpr size_t RM_COMPONENT_NAME_LEN = RM_COMPONENT_NAME.size() + 1;
    static constexpr BASE_NS::string_view RM_HANDLE = "RenderMeshComponent.mesh";

    // implements CORE_NS::IInterface
    const CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) const override;

    CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) override;

    SCENE_NS::NodeState GetAttachedState() const override;

    bool IsConnected();

    void DisableInputHandling();

    bool Connect(const INode::Ptr& parent);

    void Activate() override;

    void Deactivate() override;

    void AttachToHierarchy() override;

    void DetachFromHierarchy() override;

    void SubscribeToNameChanges();

    void UnsubscribeFromNameChanges();

    bool IsResourceClassType();

    //todo
    /*class NameValidator final : public META_NS::NoneSerializableValueValidatorBase<BASE_NS::string> {
    public:
        NameValidator(NodeImpl* self) : self_(self) {};

        [[nodiscard]] ValidationResult Validate(const BASE_NS::string& value) override
        {
            ValidationResult result;
            result.value = value;
            result.isValueAccepted = true;

            // Do not allow to change names of scene nodes if they come from IPrefab.
            if (!self_->IsResourceClassType()) {
                if (self_->IsConnected() && !self_->ownsEntity_) {
                    result.isValueAccepted = false;
                }
            }
            return result;
        }

        NodeImpl* self_ = { nullptr };
    };*/

    bool Build(const IMetadata::Ptr& data) override;

    BASE_NS::string GetName() const override;

public: // ISerialization
    //todo
 //   bool Import(

public: // IEcsObject
    CORE_NS::IEcs::Ptr GetEcs() const override;

    void SetEntity(CORE_NS::IEcs::Ptr ecs, CORE_NS::Entity entity) override;

    CORE_NS::Entity GetEntity() const override;

    void BindObject(CORE_NS::IEcs::Ptr ecsInstance, CORE_NS::Entity entity) override;

    void DefineTargetProperties(
        BASE_NS::unordered_map<BASE_NS::string_view, BASE_NS::vector<BASE_NS::string_view>> names) override;

    BASE_NS::vector<CORE_NS::Entity> GetAttachments() override;

    void AddAttachment(CORE_NS::Entity entity) override;

    void RemoveAttachment(CORE_NS::Entity entity) override;

    void CloneEcs(const BASE_NS::string& name, META_NS::IObject::Ptr target) const;

public: // INodeEcsInterfacePrivate
    SCENE_NS::IEcsScene::Ptr EcsScene() const override;

    SCENE_NS::IEcsObject::Ptr EcsObject() const override;

    SceneHolder::Ptr SceneHolder() const override;

    void ClaimOwnershipOfEntity(bool ownsEntity) override;

    void RemoveIndex(size_t index) override;

    void SetIndex(size_t index) override;

public: // INode
    SCENE_NS::IScene::Ptr GetScene() const override;

    void RenameEntity(const BASE_NS::string& name);

    // Todo, this is assuming moderate size of hierarchy, may have to rethink the recursion if that is not the case
    void RecursivelyEnableCache(bool isActive);

    // Todo, this is assuming moderate size of hierarchy, may have to rethink the recursion if that is not the case
    void UpdateChildrenPath();

    bool Initialize(SCENE_NS::IEcsScene::Ptr& scene, SCENE_NS::IEcsObject::Ptr& ecsObject, SCENE_NS::INode::Ptr parent,
        const BASE_NS::string& path, const BASE_NS::string& name, SceneHolder::WeakPtr sceneHolder,
        CORE_NS::Entity entity) override;

    void BindObject(SCENE_NS::INode::Ptr node);

    bool CompleteInitialization(const BASE_NS::string& path) override;

    static void SetPathWithEcsNode(const BASE_NS::shared_ptr<NodeImpl>& self, const BASE_NS::string& name,
        SceneHolder::Ptr sceneHolder, const CORE3D_NS::ISceneNode* ecsNode);

    static bool SetPathWithoutNode(const BASE_NS::shared_ptr<NodeImpl>& self, const BASE_NS::string& name,
        const BASE_NS::string& fullPath, SceneHolder::Ptr sceneHolder);

    // It is turning out that the path is pretty much the only thing that we can rely when operating asynchronously
    // from application thread to engine thread. Still this can go wrong if subsequent events are not recorded in
    // order.
    void SetPath(const BASE_NS::string& path, const BASE_NS::string& name, CORE_NS::Entity entity) override;

    static constexpr BASE_NS::string_view LIFECYCLE_PROPERTY_NAME = "NodeLifeCycle";

    bool HasLifecycleInfo();

    // Different scenarios currently to be considered
    // 1) Create and bind a new prefab instance
    // -> create and assign ecs node
    // -> create new prefab instance
    // -> bind (read data from prefab)
    // prefab and its children need to know they were generated by prefab
    //
    // 2) Bind a new node to an existing entity:
    // this can take place throud two different routes, either the node is explicitly requested from the scene or
    // restored by de-serialization/import/clone
    // -> just prepare and assign ecs object
    // -> system will try find and sync the state later (potentially bidirectional sync)
    // values that are modified should have set-bits on properties
    //
    // 3) Create new proxy with entity and component with default values:
    // this is either through de-serialization or c++ api
    // -> sync the proxy state to ecs
    // information on entity creation needs to be preserved

    META_NS::IProperty::Ptr GetLifecycleInfo(bool create = true) override;

    void EnsureEcsBinding(SCENE_NS::IEcsScene::Ptr scene, bool force = false);

    BASE_NS::vector<PropertyPair> ListBoundProperties() const override;

    static void BuildChildrenIterateOver(const BASE_NS::shared_ptr<NodeImpl>& self, const SCENE_NS::IEcsScene::Ptr& ecs,
        const BASE_NS::string& fullPath, BASE_NS::array_view<CORE3D_NS::ISceneNode* const> children);

    bool BuildChildren(SCENE_NS::INode::BuildBehavior options) override;

    void CheckChildrenStatus();

    BASE_NS::Math::Mat4X4 GetGlobalTransform() const override;

    void SetGlobalTransform(const BASE_NS::Math::Mat4X4& mat) override;

    struct NodeMonitor {
        using Ptr = BASE_NS::shared_ptr<NodeMonitor>;
        static Ptr Create(SCENE_NS::INode::Ptr& node, NodeImpl& observer)
        {
            return Ptr { new NodeMonitor(node, observer) };
        }
        NodeMonitor(SCENE_NS::INode::Ptr& node, NodeImpl& observer) : node_(node), observer_(observer)
        {
            if (node) {
                handle_ = node->OnBound()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([this]() {
                    if (auto node = static_pointer_cast<NodeImpl>(node_.lock())) {
                        node->META_ACCESS_PROPERTY(Status)->SetValue(SCENE_NS::INode::NODE_STATUS_FULLY_CONNECTED);
                    }
                    observer_.CheckChildrenStatus();
                }));
            }
        }

        ~NodeMonitor()
        {
            if (auto node = node_.lock()) {
                node->OnBound()->RemoveHandler(handle_);
            }
        }

        bool Ready() const
        {
            if (auto node = node_.lock()) {
                return node->Status()->GetValue() == SCENE_NS::INode::NODE_STATUS_FULLY_CONNECTED;
            }
            return true;
        }

        bool Valid() const
        {
            if (auto node = node_.lock()) {
                // If nodes status is disconnected, we failed to find ecs entry for it
                // other statuses indicate that we keep trying or already succeeded
                return node->Status()->GetValue() != SCENE_NS::INode::NODE_STATUS_DISCONNECTED;
            }
            return false;
        }

        bool operator==(const SCENE_NS::INode::Ptr& other) const
        {
            if (auto node = node_.lock()) {
                // If nodes status is disconnected, we failed to find ecs entry for it
                // other statuses indicate that we keep trying or already succeeded
                return other.get() == node.get();
            }
            return false;
        }

        SCENE_NS::INode::WeakPtr node_;
        NodeImpl& observer_;
        META_NS::IEvent::Token handle_ {};
    };
    BASE_NS::vector<NodeMonitor::Ptr> monitored_;
    bool bound_ { false };

    void MonitorChild(SCENE_NS::INode::Ptr node);

    void SetMeshToEngine();

    void SetMesh(SCENE_NS::IMesh::Ptr mesh) override;

    SCENE_NS::IMultiMeshProxy::Ptr CreateMeshProxy(size_t count, SCENE_NS::IMesh::Ptr mesh) override;

    void SetMultiMeshProxy(SCENE_NS::IMultiMeshProxy::Ptr multimesh) override;

    SCENE_NS::IMultiMeshProxy::Ptr GetMultiMeshProxy() const override;

    void ReleaseEntityOwnership();

    void Destroy() override;

    SCENE_NS::IPickingResult::Ptr GetWorldMatrixComponentAABB(bool isRecursive) const override;

    SCENE_NS::IPickingResult::Ptr GetTransformComponentAABB(bool isRecursive) const override;

public: // This and that
    void SetStatus(SCENE_NS::INode::NodeStatus status);

public: // ISerialization
    void SetSuperInstance(const IObject::Ptr& aggr, const IObject::Ptr& super) override;

    void SetParent(const IObject::Ptr& parent) override;

    IObject::Ptr GetParent() const override;

    SCENE_NS::IMesh::Ptr GetMesh() const override;

    static void InitializeMesh(const SCENE_NS::IMesh::Ptr& mesh, const BASE_NS::shared_ptr<NodeImpl>& node,
        const BASE_NS::string_view entityName);

    static void InitializeMesh(const SCENE_NS::IMesh::Ptr& mesh, const BASE_NS::shared_ptr<SCENE_NS::IEcsObject>& self);

    SCENE_NS::IMesh::Ptr GetMeshFromEngine();

    using PropertyNameCriteria = BASE_NS::unordered_map<BASE_NS::string_view, BASE_NS::vector<BASE_NS::string_view>>;
    PropertyNameCriteria& PropertyNameMask();

public:
    IObject::Ptr Resolve(const META_NS::RefUri& uri) const override;

protected:
    // Data for App thread.
    PropertyHandlerArrayHolder propHandler_ {};
    META_NS::IEvent::Token nameChangedToken_ { 0 };

    // META_NS::IContainer* container_{};
    META_NS::IContainable* containable_ {};
    META_NS::IMutableContainable* mutableContainable_ {};

    // These should be primarily accessed in Engine thread
    SCENE_NS::IEcsScene::WeakPtr ecsScene_ {};
    SCENE_NS::IEcsObject::Ptr ecsObject_ {};

    META_NS::ITaskQueue::Token initializeTaskToken_ {};

    bool ShouldExport() const override;

    SceneHolder::WeakPtr sceneHolder_ {};
    META_NS::IObject::WeakPtr parent_;

    bool shouldRetry_ { true };

    SCENE_NS::IMesh::WeakPtr weakMesh_;

    PropertyNameCriteria ecs_property_names_;

    SCENE_NS::IEnvironment::Ptr environment_;

    // when enabled, asks scene to clean up an entity ref keeping the instance alive
    bool ownsEntity_ { false };

    SCENE_NS::INode::BuildBehavior buildBehavior_ { NODE_BUILD_CHILDREN_GRADUAL };

    SCENE_NS::IMultiMeshProxy::Ptr multimesh_;

    size_t removeIndex_ { SIZE_MAX };

    SCENE_NS::NodeState attachedState_ { SCENE_NS::NodeState::DETACHED };

    META_NS::IObjectRegistry* objectRegistry_ { nullptr };
};

SCENE_BEGIN_NAMESPACE()
void RegisterNodeImpl();
void UnregisterNodeImpl();
SCENE_END_NAMESPACE()
#endif
