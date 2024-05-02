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
#include "node_impl.h"

#include "task_utils.h"

using SCENE_NS::MakeTask;

#include <scene_plugin/api/material_uid.h>
#include <scene_plugin/api/mesh_uid.h>
#include <scene_plugin/api/scene_uid.h>

#include "intf_multi_mesh_initialization.h"

bool HasChangedProperties(META_NS::IMetadata& meta)
{
    for (auto& property : meta.GetAllProperties()) {
        bool isInternal = META_NS::IsFlagSet(property, META_NS::ObjectFlagBits::INTERNAL);
        if (isInternal) {
            continue;
        }

        bool isSerializable = META_NS::IsFlagSet(property, META_NS::ObjectFlagBits::SERIALIZE);
        if (isSerializable && property->IsValueSet()) {
            return true;
        }
    }

    return false;
}

namespace {

size_t GetChildIndex(SCENE_NS::INode& parent, SCENE_NS::INode& child)
{
    auto object = interface_cast<META_NS::IObject>(&child);

    auto container = interface_cast<META_NS::IContainer>(&parent);
    if (container) {
        for (auto i = 0; i < container->GetSize(); ++i) {
            if (container->GetAt(i).get() == object) {
                return i;
            }
        }
    }

    return SIZE_MAX;
}

} // namespace

#include "bind_templates.inl"

// implements CORE_NS::IInterface
const CORE_NS::IInterface* NodeImpl::GetInterface(const BASE_NS::Uid& uid) const
{
    if (META_NS::TypeId(uid) == SCENE_NS::InterfaceId::IEnvironment.Id()) {
        return environment_.get();
    } else {
        return Fwd::GetInterface(uid);
    }
}

CORE_NS::IInterface* NodeImpl::GetInterface(const BASE_NS::Uid& uid)
{
    if (META_NS::TypeId(uid) == SCENE_NS::InterfaceId::IEnvironment.Id()) {
        return environment_.get();
    } else {
        return Fwd::GetInterface(uid);
    }
}

SCENE_NS::NodeState NodeImpl::GetAttachedState() const
{
    return attachedState_;
}

bool NodeImpl::IsConnected()
{
    if (Status()->GetValue() == SCENE_NS::INode::NODE_STATUS_CONNECTED ||
        Status()->GetValue() == SCENE_NS::INode::NODE_STATUS_FULLY_CONNECTED)
        return true;

    return false;
}

void NodeImpl::DisableInputHandling()
{
    /*auto inputMode = META_ACCESS_PROPERTY(InputMode);
    if (auto i = interface_cast<META_NS::IMetaPropertyInternal>(inputMode)) {
        auto flags = inputMode->Flags();
        flags &= ~META_NS::IMetaProperty::PropertyFlagsValue(META_NS::IMetaProperty::PropertyFlagBits::SERIALIZABLE);
        flags |= META_NS::IMetaProperty::PropertyFlagsValue(META_NS::IMetaProperty::PropertyFlagBits::INTERNAL);
        i->SetFlags(flags);
    }*/
}

bool NodeImpl::Connect(const INode::Ptr& parent)
{
    SCENE_PLUGIN_VERBOSE_LOG("Node::Connect called for: '%s' (%s)", GetName().c_str(), Path()->Get().c_str());
    if (parent) {
        auto sceneObject = interface_pointer_cast<IObject>(parent->GetScene());
        if (sceneObject) {
            if (auto sceneInterface = interface_pointer_cast<SCENE_NS::IEcsScene>(sceneObject)) {
                CORE_ASSERT(sceneInterface);

                BASE_NS::string path = "/";
                if (parent) {
                    path = parent->Path()->GetValue() + parent->Name()->GetValue() + "/";
                }

                META_ACCESS_PROPERTY(Path)->SetValue(path);

                if (!ecsObject_) {
                    EnsureEcsBinding(sceneInterface);
                }

                return true;
            }
        }
    }

    SCENE_PLUGIN_VERBOSE_LOG("Node::Connect - NO PARENT - '%s'", GetName().c_str());
    return false;
}

void NodeImpl::Activate()
{
    auto parent = interface_pointer_cast<INode>(GetParent());

    if (auto sceneHolder = SceneHolder()) {
        size_t index = SIZE_MAX;
        BASE_NS::string path = "/";
        if (parent) {
            path = parent->Path()->GetValue() + parent->Name()->GetValue() + "/";
            index = GetChildIndex(*parent, *this);
        }
        sceneHolder->QueueEngineTask(
            META_NS::MakeCallback<META_NS::ITaskQueueTask>(
                [path, index, e = ecsObject_->GetEntity(), weak_sh = BASE_NS::weak_ptr(sceneHolder)] {
                    auto sh = weak_sh.lock();
                    if (sh) {
                        sh->SetEntityActive(e, true);
                        sh->ReparentEntity(e, path, index);
                    }
                    return false;
                }),
            false);
        META_ACCESS_PROPERTY(Path)->SetValue(path);
        UpdateChildrenPath();
        RecursivelyEnableCache(true);
    }
}

void NodeImpl::Deactivate()
{
    if (auto sceneHolder = SceneHolder()) {
        // TODO: Check if we can set parent to "invalid entity".
        sceneHolder->QueueEngineTask(META_NS::MakeCallback<META_NS::ITaskQueueTask>(
                                         [e = ecsObject_->GetEntity(), weak_sh = BASE_NS::weak_ptr(sceneHolder)] {
                                             auto sh = weak_sh.lock();
                                             if (sh) {
                                                 sh->SetEntityActive(e, false);
                                             }
                                             return false;
                                         }),
            false);

        RecursivelyEnableCache(false);
    }
}

void NodeImpl::AttachToHierarchy()
{
    SCENE_PLUGIN_VERBOSE_LOG(
        "Node::AttachToHierarchy called for: '%s' (%s)", GetName().c_str(), Path()->GetValue().c_str());

    // If we have an entity, that we simply need to activate.
    // If we are unbound, we need to try to bind this node to scene graph.
    if (IsConnected()) {
        Activate();
    } else {
        Connect(interface_pointer_cast<INode>(GetParent()));
    }

    attachedState_ = SCENE_NS::NodeState::ATTACHED;
    SCENE_PLUGIN_VERBOSE_LOG("Node::AttachToHierarchy path is: '%s'", Path()->GetValue().c_str());
}

void NodeImpl::DetachFromHierarchy()
{
    SCENE_PLUGIN_VERBOSE_LOG(
        "Node::DetachFromHierarchy called for: '%s' (%s)", GetName().c_str(), Path()->GetValue().c_str());

    // We are being detachad from the scene graph.
    if (IsConnected()) {
        // We simply need to deactivate.
        Deactivate();
    }

    // TODO: Is there something we need to do here?
    attachedState_ = SCENE_NS::NodeState::DETACHED;

    SCENE_PLUGIN_VERBOSE_LOG("Node::DetachFromHierarchy path is: '%s'", Path()->GetValue().c_str());
}

void NodeImpl::SubscribeToNameChanges()
{
    if (!nameChangedToken_) {
        nameChangedToken_ = Name()->OnChanged()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([this] {
            if (IsResourceClassType() || ownsEntity_) {
                RenameEntity(META_ACCESS_PROPERTY(Name)->GetValue());
            }
        }));
    }
}

void NodeImpl::UnsubscribeFromNameChanges()
{
    if (nameChangedToken_) {
        Name()->OnChanged()->RemoveHandler(nameChangedToken_);
        nameChangedToken_ = { 0 };
    }
}

bool NodeImpl::IsResourceClassType()
{
    auto classUid = GetClassId();
    bool isResourceClassType = (classUid == SCENE_NS::ClassId::Material) || // Material
                               (classUid == SCENE_NS::ClassId::Mesh) ||     // Mesh
                               (classUid == SCENE_NS::ClassId::Animation);  // Animation

    return isResourceClassType;
}
/*
bool NodeImpl::Import(
    META_NS::Serialization::IImportContext& context, const META_NS::Serialization::ClassPrimitive& value)
{
    if (!Fwd::Import(context, value)) {
        return false;
    }

    // Backwards compatibility.
    auto prop = GetPropertyByName("SceneUid");
    if (prop) {
        RemoveProperty(prop);
    }

    prop = GetPropertyByName("Path");
    if (auto path = META_NS::property_cast<BASE_NS::string>(prop)) {
        path->Reset();
    }

    return true;
}
*/
bool NodeImpl::Build(const IMetadata::Ptr& data)
{
    ClaimOwnershipOfEntity(false);

    OnMoved()->AddHandler(META_NS::MakeCallback<META_NS::IOnChildMoved>([](const META_NS::ChildMovedInfo& info) {
        auto parent = interface_cast<IObject>(info.parent.lock())->GetName();
        auto object = info.object->GetName();
        SCENE_PLUGIN_VERBOSE_LOG("Child '%s' moved in '%s'", object.c_str(), parent.c_str());
    }));

    PropertyNameMask().clear();
    PropertyNameMask()[TRANSFORM_COMPONENT_NAME] = { TRANSFORM_POSITION.substr(TRANSFORM_COMPONENT_NAME_LEN),
        TRANSFORM_SCALE.substr(TRANSFORM_COMPONENT_NAME_LEN), TRANSFORM_ROTATION.substr(TRANSFORM_COMPONENT_NAME_LEN) };
    PropertyNameMask()[NODE_COMPONENT_NAME] = { NODE_ENABLED.substr(NODE_COMPONENT_NAME_LEN) };
    PropertyNameMask()[LAYER_COMPONENT_NAME] = { LAYER_MASK.substr(LAYER_COMPONENT_NAME_LEN) };
    PropertyNameMask()[LMATRIX_COMPONENT_NAME] = { LMATRIX_MATRIX.substr(LMATRIX_COMPONENT_NAME_LEN) };
    PropertyNameMask()[RM_COMPONENT_NAME] = { RM_HANDLE.substr(RM_COMPONENT_NAME_LEN) };
    PropertyNameMask()[ENVIRONMENT_COMPONENT_NAME] = {}; // read everything if it happens to be present

    objectRegistry_ = &META_NS::GetObjectRegistry();
    return true;
}

BASE_NS::string NodeImpl::GetName() const
{
    return META_NS::GetValue(Name());
}

CORE_NS::IEcs::Ptr NodeImpl::GetEcs() const
{
    if (ecsObject_) {
        return ecsObject_->GetEcs();
    }
    return CORE_NS::IEcs::Ptr {};
}

void NodeImpl::SetEntity(CORE_NS::IEcs::Ptr ecs, CORE_NS::Entity entity)
{
    if (ecsObject_) {
        return ecsObject_->SetEntity(ecs, entity);
    }
}

CORE_NS::Entity NodeImpl::GetEntity() const
{
    if (ecsObject_) {
        return ecsObject_->GetEntity();
    }
    return CORE_NS::Entity {};
}

void NodeImpl::BindObject(CORE_NS::IEcs::Ptr ecsInstance, CORE_NS::Entity entity)
{
    if (ecsObject_) {
        ecsObject_->BindObject(ecsInstance, entity);
    }
}

void NodeImpl::DefineTargetProperties(
    BASE_NS::unordered_map<BASE_NS::string_view, BASE_NS::vector<BASE_NS::string_view>> names)
{
    if (ecsObject_) {
        ecsObject_->DefineTargetProperties(names);
    }
}

BASE_NS::vector<CORE_NS::Entity> NodeImpl::GetAttachments()
{
    if (ecsObject_) {
        return ecsObject_->GetAttachments();
    }
    return BASE_NS::vector<CORE_NS::Entity> {};
}

void NodeImpl::AddAttachment(CORE_NS::Entity entity)
{
    if (ecsObject_) {
        ecsObject_->AddAttachment(entity);
    }
}

void NodeImpl::RemoveAttachment(CORE_NS::Entity entity)
{
    if (ecsObject_) {
        ecsObject_->RemoveAttachment(entity);
    }
}

void NodeImpl::CloneEcs(const BASE_NS::string& name, META_NS::IObject::Ptr target) const
{
    EcsScene()->AddEngineTask(
        MakeTask(
            [name, other = BASE_NS::weak_ptr(target)](auto self) {
                if (auto sceneHolder = self->SceneHolder()) {
                    // We have set the name unique, so for simplicity use it without padding
                    auto clone = sceneHolder->CloneEntity(self->EcsObject()->GetEntity(), name, false);
                    if (CORE_NS::EntityUtil::IsValid(clone)) {
                        sceneHolder->QueueApplicationTask(MakeTask(
                                                              [name](auto self, auto other) {
                                                                  if (auto ret = static_pointer_cast<NodeImpl>(other)) {
                                                                      META_NS::Property<uint32_t>(ret->GetLifecycleInfo())->SetValue(NODE_LC_CLONED);
                                                                      ret->EnsureEcsBinding(self->EcsScene(), true);
                                                                  }
                                                                  return false;
                                                              },
                                                              self, other),
                            false);
                    }
                }
                return false;
            },
            static_pointer_cast<NodeImpl>(GetSelf())),
        false);
}

/*META_NS::IObject::Ptr NodeImpl::GetClone(META_NS::ICloneableObject::CloneBehaviorFlags flags) const
{
    META_NS::IObject::Ptr ret = Super::GetClone(flags);
    auto namebuf = BASE_NS::to_string(interface_cast<META_NS::IObjectInstance>(ret)->GetInstanceId());
    BASE_NS::string name(namebuf.data(), namebuf.size());
    META_NS::SetValue(interface_pointer_cast<META_NS::INamed>(ret)->Name(), name);
    if (flags | META_NS::ICloneableObject::DEEP_CLONE_W_VALUES) {
        // clone also entity
        CloneEcs(name, ret);
    } else {
        SCENE_PLUGIN_VERBOSE_LOG("Cloned %s", name.c_str());
    }

    // ensure ecs bindings w/ new object
    return ret;
}*/

SCENE_NS::IEcsScene::Ptr NodeImpl::EcsScene() const
{
    return ecsScene_.lock();
}

SCENE_NS::IEcsObject::Ptr NodeImpl::EcsObject() const
{
    return ecsObject_;
}

SceneHolder::Ptr NodeImpl::SceneHolder() const
{
    return sceneHolder_.lock();
}

void NodeImpl::ClaimOwnershipOfEntity(bool ownsEntity)
{
    ownsEntity_ = ownsEntity;
    /*
    if (!IsResourceClassType()) {
        // If we do not own the target entity, the name property will be in read-only mode.
        auto nameInternal = interface_pointer_cast<META_NS::IMetaPropertyInternal>(Name());
        if (nameInternal) {
            auto flags = Name()->Flags();

            if (ownsEntity_) {
                flags = META_NS::SetFlag(flags, META_NS::IMetaProperty::PropertyFlagBits::WRITE);
            } else {
                flags = META_NS::ResetFlag(flags, META_NS::IMetaProperty::PropertyFlagBits::WRITE);
            }
            nameInternal->SetFlags(flags);
        }
    }
    */
}

void NodeImpl::RemoveIndex(size_t index)
{
    SCENE_PLUGIN_VERBOSE_LOG("remove node from %zu", index);
    removeIndex_ = index;
}

void NodeImpl::SetIndex(size_t index)
{
    if (removeIndex_ == index) {
        return;
    }

    if (removeIndex_ != SIZE_MAX && removeIndex_ < index) {
        index--;
    }

    if (auto scene = EcsScene()) {
        scene->AddEngineTask(MakeTask(
                                 [index](auto self) {
                                     if (auto sceneHolder = self->SceneHolder()) {
                                         sceneHolder->ReindexEntity(self->EcsObject()->GetEntity(), index);
                                     }
                                     return false;
                                 },
                                 static_pointer_cast<NodeImpl>(GetSelf())),
            false);
    }
    removeIndex_ = SIZE_MAX; // reset, this is not very convenient with subsequent async calls
    SCENE_PLUGIN_VERBOSE_LOG("move node to %zu", index);
}

SCENE_NS::IScene::Ptr NodeImpl::GetScene() const
{
    return interface_pointer_cast<SCENE_NS::IScene>(ecsScene_);
}

void NodeImpl::RenameEntity(const BASE_NS::string& name)
{
    auto ecsScene = EcsScene();
    if (!ecsScene) {
        return;
    }

    ecsScene->AddEngineTask(MakeTask(
                                [name](auto self) {
                                    if (auto sceneHolder = self->SceneHolder()) {
                                        sceneHolder->RenameEntity(self->EcsObject()->GetEntity(), name);
                                    }
                                    return false;
                                },
                                static_pointer_cast<NodeImpl>(GetSelf())),
        false);
    UpdateChildrenPath();
}

// Todo, this is assuming moderate size of hierarchy, may have to rethink the recursion if that is not the case
void NodeImpl::RecursivelyEnableCache(bool isActive)
{
    if (auto scene = GetScene()) {
        scene->SetCacheEnabled(GetSelf<SCENE_NS::INode>(), isActive);
    }

    if (auto container = GetSelf<META_NS::IContainer>()) {
        for (auto& object : container->GetAll()) {
            auto child = static_cast<NodeImpl*>(object.get());

            child->RecursivelyEnableCache(isActive);
        }
    }
}

// Todo, this is assuming moderate size of hierarchy, may have to rethink the recursion if that is not the case
void NodeImpl::UpdateChildrenPath()
{
    if (auto scene=GetScene()) {
        scene->UpdateCachedNodePath(GetSelf<SCENE_NS::INode>());
    }

    if (auto container = GetSelf<META_NS::IContainer>()) {
        auto cachedPath = META_ACCESS_PROPERTY(Path)->GetValue();
        cachedPath.append(Name()->GetValue());
        cachedPath.append("/");
        for (auto& object : container->GetAll()) {
            auto child = dynamic_cast<NodeImpl*>(object.get());
            if (child) {
                child->META_ACCESS_PROPERTY(Path)->SetValue(cachedPath);
                child->UpdateChildrenPath();
            }
        }
    }
}

bool NodeImpl::ShouldExport() const
{
    bool isProxyNode = false;
    auto priv = interface_cast<INodeEcsInterfacePrivate>(GetSelf());
    if (META_NS::Property<uint32_t> lifeCycleInfo = priv->GetLifecycleInfo(false)) {
        if (lifeCycleInfo->GetValue() == NODE_LC_MIRROR_EXISTING) {
            isProxyNode = true;
        }
    }

    if (!isProxyNode) {
        isProxyNode = !ownsEntity_;
    }

    // If we have mesh that needs to be exported, then serialize self.
    auto mesh = GetMesh();
    if (auto privateInterface = interface_cast<INodeEcsInterfacePrivate>(mesh)) {
        if (privateInterface->ShouldExport()) {
            return true;
        }
    }

    // Proxy node is exported if ..
    if (isProxyNode) {
        // It has changed properties.
        if (HasChangedProperties(*GetSelf<META_NS::IMetadata>())) {
            return true;
        }

        // It has attachments.
        if (auto attach = GetSelf<META_NS::IAttach>()) {
            if (attach->GetAttachments({}, false).size() > 0) {
                return true;
            }
        }

        // Any of its child needs to be exported.
        if (auto container = GetSelf<META_NS::IContainer>()) {
            for (auto& childNode : container->GetAll<INode>()) {
                if (auto privateInterface = interface_cast<INodeEcsInterfacePrivate>(childNode)) {
                    if (privateInterface->ShouldExport()) {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    return true;
}

bool NodeImpl::Initialize(SCENE_NS::IEcsScene::Ptr& scene, SCENE_NS::IEcsObject::Ptr& ecsObject,
    SCENE_NS::INode::Ptr parent, const BASE_NS::string& path, const BASE_NS::string& name,
    SceneHolder::WeakPtr sceneHolder, CORE_NS::Entity entity)
{
    if (!ecsObject || !scene) {
        return false;
    }

    ecsScene_ = scene;
    ecsObject_ = ecsObject;
    sceneHolder_ = sceneHolder;

    auto currentParent = GetParent();
    if (!currentParent && parent) {
        interface_cast<IContainer>(parent)->Add(GetSelf());
    }

    META_ACCESS_PROPERTY(ConnectionStatus)->SetBind(ecsObject->ConnectionStatus());

    // Need to set a path before ECS instantiation to enable IContainer to share a proxy beforehand
    SetPath(path, name, entity);
    if (!ecsObject->GetEcs()) {
        // We will not proceed further with construction until we have esc object bound
        return false;
    }

    return true; // CompleteInitialization(path);
}

void NodeImpl::BindObject(SCENE_NS::INode::Ptr node)
{
    auto priv = interface_cast<INodeEcsInterfacePrivate>(node);
    if (!priv) {
        return;
    }

    if (priv->EcsObject()) {
        return;
    }

    bool create = false;

    if (META_NS::Property<uint32_t> creationPolicy = priv->GetLifecycleInfo()) {
        create = (creationPolicy->GetValue() == NODE_LC_CREATED);
    }

    SCENE_PLUGIN_VERBOSE_LOG(
        "Connecting object from property: %s", (GetValue(node->Path()) + GetValue(node->Name())).c_str());
    EcsScene()->BindNodeToEcs(node, GetValue(node->Path()) + GetValue(node->Name()), create);
}

bool NodeImpl::CompleteInitialization(const BASE_NS::string& path)
{
    initializeTaskToken_ = {};

    if (auto scene = EcsScene()) {
        auto meta = interface_pointer_cast<META_NS::IMetadata>(ecsObject_);

        if (auto name = Name()->GetValue(); name != path) {
            RenameEntity(name);
        }

        // Ensure that our cache path is correct now that we have entity connected.
        interface_cast<SCENE_NS::IScene>(scene)->UpdateCachedNodePath(GetSelf<SCENE_NS::INode>());

        propHandler_.Reset();
        propHandler_.SetSceneHolder(SceneHolder());

        // Use node properties as default, in case we have created the entity to ECS.
        propHandler_.SetUseEcsDefaults(!ownsEntity_);

        BindChanges<BASE_NS::Math::Quat>(propHandler_, META_ACCESS_PROPERTY(Rotation), meta, TRANSFORM_ROTATION);
        BindChanges<BASE_NS::Math::Vec3>(propHandler_, META_ACCESS_PROPERTY(Position), meta, TRANSFORM_POSITION);
        BindChanges<BASE_NS::Math::Vec3>(propHandler_, META_ACCESS_PROPERTY(Scale), meta, TRANSFORM_SCALE);
        BindChanges<bool>(propHandler_, META_ACCESS_PROPERTY(Visible), meta, NODE_ENABLED);
        BindChanges<uint64_t>(propHandler_, META_ACCESS_PROPERTY(LayerMask), meta, LAYER_MASK);

        // Restore default behavior.
        propHandler_.SetUseEcsDefaults(true);

        BindChanges<BASE_NS::Math::Mat4X4>(propHandler_, META_ACCESS_PROPERTY(LocalMatrix), meta, LMATRIX_MATRIX);

        auto rh = meta->GetPropertyByName(RM_HANDLE);

        auto mesh = META_NS::GetValue(Mesh());
        if (mesh) {
            // if se have a mesh set, initialize it
            InitializeMesh(mesh, GetSelf<IEcsObject>());
        } else if (rh) {
            // otherwise get mesh from engine only if the component has render mesh present
            GetMeshFromEngine();
        }

        // if the node has render mesh component, subscribe changes from engin
        if (rh) {
            rh->OnChanged()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([this]() { GetMeshFromEngine(); }),
                reinterpret_cast<uint64_t>(this));
        }

        // and set the proxy if user wants to change the mesh through the property
        Mesh()->OnChanged()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([this]() {
            if (auto node = interface_pointer_cast<SCENE_NS::INode>(META_NS::GetValue(Mesh()))) {
                node->Status()->OnChanged()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>(
                    [](const auto& self, const auto& status) {
                        if (self && status && status->GetValue() == SCENE_NS::INode::NODE_STATUS_FULLY_CONNECTED) {
                            static_cast<NodeImpl*>(self.get())->SetMeshToEngine();
                        }
                    },
                    GetSelf(), node->Status()));
                if (auto status = META_NS::GetValue(node->Status());
                    status == SCENE_NS::INode::NODE_STATUS_FULLY_CONNECTED ||
                    status == SCENE_NS::INode::NODE_STATUS_CONNECTED) {
                    SetMeshToEngine();
                }
            }
        }),
            reinterpret_cast<uint64_t>(this));

        if (buildBehavior_ == NODE_BUILD_CHILDREN_GRADUAL) {
            // We are assumingly on the correct thread already, but have deriving classes a slot to complete
            // initialization Before notifying the client
            scene->AddApplicationTask(META_NS::MakeCallback<META_NS::ITaskQueueTask>(
                                          [buildBehavior = buildBehavior_](const auto& node) {
                                              if (node) {
                                                  node->BuildChildren(buildBehavior);
                                              }
                                              return false;
                                          },
                                          GetSelf<SCENE_NS::INode>()),
                true);
        }

        if (auto sceneHolder = SceneHolder()) {
            for (auto& attachment : ecsObject_->GetAttachments()) {
                if (auto animation = sceneHolder->GetAnimation(attachment)) {
                    // Make flat animation array based on name-property + entity id separated by ':'
                    // This is identical to meshes and materials, but the implementation is tad different
                    auto name = META_NS::GetValue(interface_cast<META_NS::INamed>(animation)->Name());
                    name.append(":");
                    name.append(BASE_NS::to_hex(attachment.id));
                    if (auto attachment = GetScene()->GetAnimation(name)) {
                        if (auto typed = interface_cast<META_NS::IAttachment>(attachment)) {
                            Attach(interface_pointer_cast<IObject>(attachment), GetSelf());
                        }
                    }
                }
            }
        }

        SubscribeToNameChanges();

        return true;
    }
    return false;
}

void NodeImpl::SetPathWithEcsNode(const BASE_NS::shared_ptr<NodeImpl>& self, const BASE_NS::string& name,
    SceneHolder::Ptr sceneHolder, const CORE3D_NS::ISceneNode* ecsNode)
{
    BASE_NS::weak_ptr<IObject> me = static_pointer_cast<IObject>(self);
    if (!self->EcsObject()->GetEcs()) {
        // Initialize ECS Object
        SCENE_PLUGIN_VERBOSE_LOG("binding node: %s", name.c_str());
        // We enable layer component just to have it handled consistently across
        // the other properties, this may not be desired
        auto entity = ecsNode->GetEntity();
        sceneHolder->EnableLayerComponent(entity);

        auto ecsObject = self->EcsObject();
        // Introspect the engine components
        if (auto proxyIf = interface_cast<SCENE_NS::IEcsProxyObject>(ecsObject)) {
            proxyIf->SetCommonListener(sceneHolder->GetCommonEcsListener());
        }
        ecsObject->DefineTargetProperties(self->PropertyNameMask());
        ecsObject->SetEntity(sceneHolder->GetEcs(), entity);
        sceneHolder->UpdateAttachments(ecsObject);
        sceneHolder->QueueApplicationTask(
            MakeTask(
                [name](auto selfObject) {
                    if (auto self = static_pointer_cast<NodeImpl>(selfObject)) {
                        self->CompleteInitialization(name);
                        self->SetStatus(SCENE_NS::INode::NODE_STATUS_CONNECTED);
                        if (auto node = interface_cast<SCENE_NS::INode>(selfObject)) {
                            META_NS::Invoke<META_NS::IOnChanged>(node->OnLoaded());
                        }
                    }
                    return false;
                },
                me),
            false);
    } else {
        // ECS was already initialized, presumably we were just re-parented, update index
        sceneHolder->QueueApplicationTask(MakeTask(
                                              [](auto selfObject) {
                                                  if (auto self = static_pointer_cast<NodeImpl>(selfObject)) {
                                                      self->UpdateChildrenPath();
                                                  }
                                                  return false;
                                              },
                                              me),
            false);
    }
}

bool NodeImpl::SetPathWithoutNode(const BASE_NS::shared_ptr<NodeImpl>& self, const BASE_NS::string& name,
    const BASE_NS::string& fullPath, SceneHolder::Ptr sceneHolder)
{
    CORE_NS::Entity entity;
    BASE_NS::weak_ptr<IObject> me = static_pointer_cast<IObject>(self);
    if (sceneHolder->FindResource(name, fullPath, entity)) {
        SCENE_PLUGIN_VERBOSE_LOG("binding resource: %s", name.c_str());
        if (auto proxyIf = interface_cast<SCENE_NS::IEcsProxyObject>(self->EcsObject())) {
            proxyIf->SetCommonListener(sceneHolder->GetCommonEcsListener());
        }
        self->EcsObject()->DefineTargetProperties(self->PropertyNameMask());
        self->EcsObject()->SetEntity(sceneHolder->GetEcs(), entity);
        sceneHolder->QueueApplicationTask(MakeTask(
                                              [name](auto selfObject) {
                                                  if (auto self = static_pointer_cast<NodeImpl>(selfObject)) {
                                                      // There is not much to bind inside node, could presumably
                                                      // fork this as own instance
                                                      self->CompleteInitialization(name);
                                                      self->SetStatus(SCENE_NS::INode::NODE_STATUS_CONNECTED);
                                                      if (self->buildBehavior_ == NODE_BUILD_CHILDREN_ALL) {
                                                          self->BuildChildren(self->buildBehavior_);
                                                      }
                                                      META_NS::Invoke<META_NS::IOnChanged>(self->OnLoaded());
                                                  }
                                                  return false;
                                              },
                                              me),
            false);
    } else if (META_NS::GetValue(self->GetScene()->Asynchronous()) && self->shouldRetry_) {
        // When nodes are deserialized, it is possible that the construction order
        // prepares children before their parent, just letting the task spin once more
        // fixes the problem. ToDo: Get more elegant solution for this
        self->shouldRetry_ = false;
        return true;
    } else {
        // Giving in. If the node arrives later, it may be re-attempted due e.g. parent
        // or name changes, otherwise we are done with it.
        self->shouldRetry_ = true;
        sceneHolder->QueueApplicationTask(MakeTask(
                                              [name](auto selfObject) {
                                                  if (auto self = static_pointer_cast<NodeImpl>(selfObject)) {
                                                      self->SetStatus(SCENE_NS::INode::NODE_STATUS_DISCONNECTED);
                                                  }
                                                  return false;
                                              },
                                              me),
            false);
    }
    return false;
}

// It is turning out that the path is pretty much the only thing that we can rely when operating asynchronously
// from application thread to engine thread. Still this can go wrong if subsequent events are not recorded in
// order.
void NodeImpl::SetPath(const BASE_NS::string& path, const BASE_NS::string& name, CORE_NS::Entity entity)
{
    // BASE_NS::string name;

    META_ACCESS_PROPERTY(Path)->SetValue(path);
    META_ACCESS_PROPERTY(Name)->SetValue(name);

    // SCENE_PLUGIN_VERBOSE_LOG("asking engine to make sure that: '%s' is child of '%s'", name.c_str(),
    if (auto scene = EcsScene()) {
        SetStatus(SCENE_NS::INode::NODE_STATUS_CONNECTING);
        if (auto iscene = interface_cast<SCENE_NS::IScene>(scene)) {
            iscene->UpdateCachedNodePath(GetSelf<SCENE_NS::INode>());
        }

        initializeTaskToken_ = scene->AddEngineTask(
            MakeTask(
                [name, fullpath = path + name](auto selfObject) {
                    if (auto self = static_pointer_cast<NodeImpl>(selfObject)) {
                        if (auto sceneHolder = self->SceneHolder()) {
                            auto parentPath = self->META_ACCESS_PROPERTY(Path)
                                                  ->GetValue(); // should also this be snapshot from App thread?

                            auto ecsNode = sceneHolder->ReparentEntity(parentPath, name);

                            if (ecsNode) {
                                SetPathWithEcsNode(self, name, sceneHolder, ecsNode);
                            } else {
                                // The entity might be valid even if there is no node attached
                                return SetPathWithoutNode(self, name, fullpath, sceneHolder);
                            }
                        }
                    }
                    return false;
                },
                GetSelf()),
            false);
    }
}

// static constexpr BASE_NS::string_view LIFECYCLE_PROPERTY_NAME = "NodeLifeCycle";

bool NodeImpl::HasLifecycleInfo()
{
    return GetLifecycleInfo(false) != nullptr;
}

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

META_NS::IProperty::Ptr NodeImpl::GetLifecycleInfo(bool create)
{
    META_NS::IProperty::Ptr creationPolicy;
    if (auto meta = GetSelf<META_NS::IMetadata>()) {
        META_NS::IProperty::Ptr lifeCycle;
        if (lifeCycle = meta->GetPropertyByName(LIFECYCLE_PROPERTY_NAME); !lifeCycle && create) {
            auto p = META_NS::ConstructProperty<uint32_t>(LIFECYCLE_PROPERTY_NAME, 0, META_NS::DEFAULT_PROPERTY_FLAGS | META_NS::ObjectFlagBits::INTERNAL);
            meta->AddProperty(p);
            lifeCycle = p;
        }

        if (creationPolicy = META_NS::Property<uint32_t>(lifeCycle); !creationPolicy && create) {
            CORE_LOG_E("%s: inconsistent system state. Can not store creation info for node", __func__);
        }
    }
    return creationPolicy;
}

void NodeImpl::EnsureEcsBinding(SCENE_NS::IEcsScene::Ptr scene, bool force)
{
    SCENE_PLUGIN_VERBOSE_LOG("Node::EnsureEcsBinding called for %s (%s)", GetName().c_str(), Path()->Get().c_str());
    if (scene && (force || !EcsScene())) {
        BASE_NS::string fullPath = META_NS::GetValue(META_ACCESS_PROPERTY(Path));
        fullPath.append(META_NS::GetValue(Name()));
        auto self = GetSelf<SCENE_NS::INode>();

        INodeEcsInterfacePrivate* privateParts { nullptr };
        
	bool shouldCreate = true;
        privateParts = interface_cast<INodeEcsInterfacePrivate>(self);
        if (auto lifeCycleInfo = privateParts->GetLifecycleInfo(false)) {
           if (auto value = META_NS::Property<uint32_t>(lifeCycleInfo)->GetValue()) {
                shouldCreate = (value & NODE_LC_CLONED) || (value & NODE_LC_CREATED);
            }
        }
        scene->BindNodeToEcs(self, fullPath, shouldCreate);
    }
}

BASE_NS::vector<SCENE_NS::IProxyObject::PropertyPair> NodeImpl::ListBoundProperties() const
{
    return propHandler_.GetBoundProperties();
}

void NodeImpl::BuildChildrenIterateOver(const BASE_NS::shared_ptr<NodeImpl>& self, const SCENE_NS::IEcsScene::Ptr& ecs,
    const BASE_NS::string& fullPath, BASE_NS::array_view<CORE3D_NS::ISceneNode* const> children)
{
    BASE_NS::weak_ptr<IObject> me = static_pointer_cast<IObject>(self);
    for (const auto& child : children) {
        auto childPath = fullPath;
        auto childName = child->GetName();
        if (!childName.empty() && childName.find("/") == BASE_NS::string_view::npos &&
            !childName.starts_with(MULTI_MESH_CHILD_PREFIX)) {
            childPath.append("/");
            childPath.append(childName);

            ecs->AddApplicationTask(
                MakeTask(
                    [childPath, childName](auto selfObject) {
                        if (auto self = static_pointer_cast<NodeImpl>(selfObject)) {
                            if (auto scene = self->GetScene()) {
                                if (auto child = interface_pointer_cast<SCENE_NS::INode>(self->FindByName(childName))) {
                                    self->MonitorChild(child);
                                    // The call is pretty much costless when the ecs object has been already set, not
                                    // sure if there is a code path that needs this, though
                                    static_pointer_cast<NodeImpl>(child)->EnsureEcsBinding(self->EcsScene());
                                } else {
                                    auto node = scene->GetNode(childPath, META_NS::IObject::UID, self->buildBehavior_);
                                    self->MonitorChild(node);
                                    node->BuildChildren(self->buildBehavior_);
                                }
                            }
                        }
                        return false;
                    },
                    me),
                false);
        }
    }
}

bool NodeImpl::BuildChildren(SCENE_NS::INode::BuildBehavior options)
{
    buildBehavior_ = options;

    if (options == SCENE_NS::INode::NODE_BUILD_CHILDREN_NO_BUILD || !GetScene()) {
        return true;
    }

    if (!ecsObject_ || !ecsObject_->GetEcs()) {
        if (!META_NS::GetValue(GetScene()->Asynchronous())) {
            CORE_LOG_W("%s: no ecs available, cannot inspect children", __func__);
        }
        return false;
    }

    if (auto scene = EcsScene()) {
        BASE_NS::string fullPath = META_ACCESS_PROPERTY(Path)->GetValue();
        fullPath.append(Name()->GetValue());

        if (fullPath.starts_with("/")) {
            fullPath.erase(0, 1);
        }
        if (options == SCENE_NS::INode::NODE_BUILD_ONLY_DIRECT_CHILDREN) {
            // yeah, for now.. let's just do this.
            options = SCENE_NS::INode::NODE_BUILD_CHILDREN_GRADUAL;
        }

        scene->AddEngineTask(MakeTask([me = BASE_NS::weak_ptr(GetSelf()), fullPath]() {
            if (auto self = static_pointer_cast<NodeImpl>(me.lock())) {
                if (auto ecs = self->EcsScene()) {
                    CORE3D_NS::INodeSystem* nodeSystem =
                        static_cast<CORE3D_NS::INodeSystem*>(ecs->GetEcs()->GetSystem(CORE3D_NS::INodeSystem::UID));
                    const auto& root = nodeSystem->GetRootNode();
                    const auto& node = root.LookupNodeByPath(fullPath);
                    if (!node) {
                        CORE_LOG_W("%s: cannot find '%s'", __func__, fullPath.c_str());
                    } else {
                        // 1) on engine queue introspect children nodes
                        const auto& children = node->GetChildren();
                        if (children.size() == 0 &&
                            META_NS::GetValue(self->Status()) != SCENE_NS::INode::NODE_STATUS_FULLY_CONNECTED) {
                            ecs->AddApplicationTask(MakeTask(
                                                        [](auto selfObject) {
                                                            if (auto self = static_pointer_cast<NodeImpl>(selfObject)) {
                                                                self->SetStatus(
                                                                    SCENE_NS::INode::NODE_STATUS_FULLY_CONNECTED);
                                                                META_NS::Invoke<META_NS::IOnChanged>(self->OnBound());
                                                                self->bound_ = true;
                                                            }
                                                            return false;
                                                        },
                                                        me),
                                false);
                        } else {
                            // 2) on app queue instantiate nodes
                            BuildChildrenIterateOver(self, ecs, fullPath, children);

                            // once we have iterated through children, check status once
                            ecs->AddApplicationTask(MakeTask(
                                                        [](auto selfObject) {
                                                            if (auto self = static_pointer_cast<NodeImpl>(selfObject)) {
                                                                self->CheckChildrenStatus();
                                                            }
                                                            return false;
                                                        },
                                                        me),
                                false);
                        }
                    }
                }
            }
            return false;
        }),
            false);
    }
    return true;
}

void NodeImpl::CheckChildrenStatus()
{
    for (const auto& child : monitored_) {
        if (!child->Ready()) {
            return;
        }
    }
    size_t ix = 0;
    while (ix < monitored_.size()) {
        if (monitored_[ix]->Valid()) {
            ix++;
        } else {
            monitored_.erase(monitored_.begin() + ix);
        }
    }

    if (META_NS::GetValue(Status()) != SCENE_NS::INode::NODE_STATUS_FULLY_CONNECTED) {
        SetStatus(SCENE_NS::INode::NODE_STATUS_FULLY_CONNECTED);
    }

    if (!bound_) {
        SCENE_PLUGIN_VERBOSE_LOG("%s: all children completed", META_NS::GetValue(Name()).c_str());
        META_NS::Invoke<META_NS::IOnChanged>(OnBound());
    }

    bound_ = true;
}

BASE_NS::Math::Mat4X4 NodeImpl::GetGlobalTransform() const
{
    auto ret = BASE_NS::Math::IDENTITY_4X4;
    BASE_NS::vector<META_NS::IContainable::Ptr> nodes;
    nodes.push_back(GetSelf<META_NS::IContainable>());
    auto current = nodes.back()->GetParent();
    while (auto containable = interface_pointer_cast<META_NS::IContainable>(current)) {
        nodes.push_back(containable);
        current = nodes.back()->GetParent();
    }

    for (size_t ix = nodes.size(); ix != 0; ix--) {
        ret = ret * META_NS::GetValue(interface_pointer_cast<INode>(nodes[ix - 1])->LocalMatrix());
    }

    return ret;
}

void NodeImpl::SetGlobalTransform(const BASE_NS::Math::Mat4X4& mat)
{
    auto global = BASE_NS::Math::IDENTITY_4X4;
    BASE_NS::vector<META_NS::IContainable::Ptr> nodes;
    nodes.push_back(GetSelf<META_NS::IContainable>());

    auto current = nodes.back()->GetParent();
    while (auto containable = interface_pointer_cast<META_NS::IContainable>(current)) {
        nodes.push_back(containable);
        current = nodes.back()->GetParent();
    }

    for (size_t ix = nodes.size(); ix != 1; ix--) {
        global = global * META_NS::GetValue(interface_pointer_cast<INode>(nodes[ix - 1])->LocalMatrix());
    }

    // META_NS::SetValue(LocalMatrix(), mat / global);
    auto newLocal = BASE_NS::Math::Inverse(global) * mat;
    BASE_NS::Math::Quat rotation;
    BASE_NS::Math::Vec3 scale;
    BASE_NS::Math::Vec3 translate;
    BASE_NS::Math::Vec3 skew;
    BASE_NS::Math::Vec4 persp;
    if (BASE_NS::Math::Decompose(newLocal, scale, rotation, translate, skew, persp)) {
        META_NS::SetValue(Scale(), scale);
        META_NS::SetValue(Rotation(), rotation);
        META_NS::SetValue(Position(), translate);
    }
}

void NodeImpl::MonitorChild(SCENE_NS::INode::Ptr node)
{
    for (auto& child : monitored_) {
        if (*child == node) {
            return;
        }
    }

    monitored_.emplace_back(NodeMonitor::Create(node, *this));
    bound_ = false;
}

void NodeImpl::SetMeshToEngine()
{
    if (auto mesh = META_NS::GetValue(Mesh())) {
        if (auto ecsObject = interface_pointer_cast<SCENE_NS::IEcsObject>(mesh)) {
            if (auto scene = EcsScene()) {
                scene->AddEngineTask(
                    MakeTask(
                        [](const auto self, const auto meshObject) {
                            if (auto sceneHolder = static_cast<NodeImpl*>(self.get())->SceneHolder()) {
                                sceneHolder->SetMesh(self->GetEntity(), meshObject->GetEntity());
                            }
                            return false;
                        },
                        GetSelf<SCENE_NS::IEcsObject>(), interface_pointer_cast<SCENE_NS::IEcsObject>(mesh)),
                    false);
            }
        }
    }
}

void NodeImpl::SetMesh(SCENE_NS::IMesh::Ptr mesh)
{
    if (mesh == META_NS::GetValue(Mesh())) {
        return; // the matching mesh is already set
    }

    META_NS::SetValue(Mesh(), mesh);
}

SCENE_NS::IMultiMeshProxy::Ptr NodeImpl::CreateMeshProxy(size_t count, SCENE_NS::IMesh::Ptr mesh)
{
    // SCENE_NS::IMultiMeshProxy::Ptr ret { SCENE_NS::MultiMeshProxy() };
    SCENE_NS::IMultiMeshProxy::Ptr ret =
        GetObjectRegistry().Create<SCENE_NS::IMultiMeshProxy>(SCENE_NS::ClassId::MultiMeshProxy);

    interface_cast<IMultimeshInitilization>(ret)->Initialize(SceneHolder(), count, EcsObject()->GetEntity());

    if (mesh) {
        ret->Mesh()->SetValue(mesh);
    }

    // return
    SetMultiMeshProxy(ret);
    return ret;
}

void NodeImpl::SetMultiMeshProxy(SCENE_NS::IMultiMeshProxy::Ptr multimesh)
{
    multimesh_ = multimesh;
}

SCENE_NS::IMultiMeshProxy::Ptr NodeImpl::GetMultiMeshProxy() const
{
    return multimesh_;
}

void NodeImpl::ReleaseEntityOwnership()
{
    if (!ecsObject_) {
        return;
    }

    auto entity = ecsObject_->GetEntity();
    if (!CORE_NS::EntityUtil::IsValid(entity)) {
        return;
    }

    auto sh = SceneHolder();
    if (sh) {
        auto attachments = ecsObject_->GetAttachments();
        for (auto attachment : attachments) {
            ecsObject_->RemoveAttachment(attachment);
        }
    }

    if (sh) {
        sh->SetEntityActive(entity, true);

        if (ownsEntity_) {
            sh->DestroyEntity(entity);
        }

        SetEntity(ecsObject_->GetEcs(), {});
    }
}

void NodeImpl::Destroy()
{
    UnsubscribeFromNameChanges();

    META_NS::GetObjectRegistry().Purge();

    if (Name()) {
        SCENE_PLUGIN_VERBOSE_LOG(
            "Tearing down: %s%s", META_NS::GetValue(Path()).c_str(), META_NS::GetValue(Name()).c_str());
        if (auto ecss = ecsScene_.lock()) {
            ecss->CancelEngineTask(initializeTaskToken_);
        }
    }

    ReleaseEntityOwnership();
    Fwd::Destroy();
}

SCENE_NS::IPickingResult::Ptr NodeImpl::GetWorldMatrixComponentAABB(bool isRecursive) const
{
    auto ret = objectRegistry_->Create<SCENE_NS::IPickingResult>(SCENE_NS::ClassId::PendingVec3Request);
    if (ret && SceneHolder()) {
        SceneHolder()->QueueEngineTask(
            META_NS::MakeCallback<META_NS::ITaskQueueTask>([isRecursive, weakRet = BASE_NS::weak_ptr(ret),
                                                           weakSh = sceneHolder_,
                                                           weakSelf = BASE_NS::weak_ptr(ecsObject_)]() {
                if (auto sh = weakSh.lock()) {
                    if (auto ret = weakRet.lock()) {
                        if (auto self = weakSelf.lock()) {
                            if (sh->GetWorldMatrixComponentAABB(ret, self->GetEntity(), isRecursive)) {
                                sh->QueueApplicationTask(META_NS::MakeCallback<META_NS::ITaskQueueTask>([weakRet]() {
                                    if (auto writable =
                                            interface_pointer_cast<SCENE_NS::IPendingRequestData<BASE_NS::Math::Vec3>>(
                                                weakRet)) {
                                        writable->MarkReady();
                                    }
                                    return false;
                                }),
                                    false);
                            }
                        }
                    }
                }
                return false;
            }),
            false);
        return ret;
    }
    return SCENE_NS::IPickingResult::Ptr();
}

SCENE_NS::IPickingResult::Ptr NodeImpl::GetTransformComponentAABB(bool isRecursive) const
{
    auto ret = objectRegistry_->Create<SCENE_NS::IPickingResult>(SCENE_NS::ClassId::PendingVec3Request);
    if (ret && SceneHolder()) {
        SceneHolder()->QueueEngineTask(
            META_NS::MakeCallback<META_NS::ITaskQueueTask>([isRecursive, weakRet = BASE_NS::weak_ptr(ret),
                                                           weakSh = sceneHolder_,
                                                           weakSelf = BASE_NS::weak_ptr(ecsObject_)]() {
                if (auto sh = weakSh.lock()) {
                    if (auto ret = weakRet.lock()) {
                        if (auto self = weakSelf.lock()) {
                            if (sh->GetTransformComponentAABB(ret, self->GetEntity(), isRecursive)) {
                                sh->QueueApplicationTask(META_NS::MakeCallback<META_NS::ITaskQueueTask>([weakRet]() {
                                    if (auto writable =
                                            interface_pointer_cast<SCENE_NS::IPendingRequestData<BASE_NS::Math::Vec3>>(
                                                weakRet)) {
                                        writable->MarkReady();
                                    }
                                    return false;
                                }),
                                    false);
                            }
                        }
                    }
                }
                return false;
            }),
            false);
        return ret;
    }
    return SCENE_NS::IPickingResult::Ptr();
}

void NodeImpl::SetStatus(SCENE_NS::INode::NodeStatus status)
{
    META_ACCESS_PROPERTY(Status)->SetValue(status);
}
/*
bool NodeImpl::Export(
    META_NS::Serialization::IExportContext& context, META_NS::Serialization::ClassPrimitive& value) const
{
    if (!ShouldExport()) {
        return false;
    }

    SCENE_PLUGIN_VERBOSE_LOG("NodeImpl::%s %s", __func__, Name()->Get().c_str());
    return Fwd::Export(context, value);
}
*/
void NodeImpl::SetSuperInstance(const IObject::Ptr& aggr, const IObject::Ptr& super)
{
    Fwd::SetSuperInstance(aggr, super);
    containable_ = interface_cast<IContainable>(super);
    mutableContainable_ = interface_cast<IMutableContainable>(super);
}

void NodeImpl::SetParent(const IObject::Ptr& parent)
{
    if (mutableContainable_) {
        mutableContainable_->SetParent(parent);
    } else {
        parent_ = parent;
    }
}

META_NS::IObject::Ptr NodeImpl::GetParent() const
{
    if (containable_) {
        return containable_->GetParent();
    }

    auto parent = parent_.lock();
    if (parent) {
        return parent;
    }

    return {};
}

SCENE_NS::IMesh::Ptr NodeImpl::GetMesh() const
{
    return META_NS::GetValue(Mesh());
}

void NodeImpl::InitializeMesh(
    const SCENE_NS::IMesh::Ptr& mesh, const BASE_NS::shared_ptr<NodeImpl>& node, const BASE_NS::string_view entityName)
{
    auto scene = node->EcsScene();
    if (auto ecsObject = META_NS::GetObjectRegistry().Create<SCENE_NS::IEcsObject>(SCENE_NS::ClassId::EcsObject)) {
        if (auto privateInit = interface_cast<INodeEcsInterfacePrivate>(mesh)) {
            privateInit->Initialize(scene, ecsObject, {}, "", BASE_NS::string(entityName), node->SceneHolder(), {});
        }
    }
}

void NodeImpl::InitializeMesh(const SCENE_NS::IMesh::Ptr& mesh, const BASE_NS::shared_ptr<SCENE_NS::IEcsObject>& self)
{
    auto strongMe = interface_pointer_cast<IObject>(self);
    if (auto node = static_pointer_cast<NodeImpl>(strongMe)) {
        if (auto sceneHolder = node->SceneHolder()) {
            auto entityName = sceneHolder->GetMeshName(self->GetEntity());
            if (!entityName.empty()) {
                sceneHolder->QueueApplicationTask(MakeTask(
                                                      [entityName, mesh](auto selfObject) {
                                                          if (auto node = static_pointer_cast<NodeImpl>(selfObject)) {
                                                              InitializeMesh(mesh, node, entityName);
                                                          }
                                                          return false;
                                                      },
                                                      strongMe),
                    false);
            }
        }
    }
}

SCENE_NS::IMesh::Ptr NodeImpl::GetMeshFromEngine()
{
    SCENE_NS::IMesh::Ptr ret {};
    if (auto iscene = GetScene()) {
        if (META_NS::GetValue(iscene->Asynchronous()) == false) {
            auto entityName = SceneHolder()->GetMeshName(EcsObject()->GetEntity());
            if (!entityName.empty()) {
                ret = iscene->GetMesh(entityName);
            }
        }
    }

    if (!ret) {
        ret = GetObjectRegistry().Create<SCENE_NS::IMesh>(SCENE_NS::ClassId::Mesh);

        if (auto scene = EcsScene()) {
            scene->AddEngineTask(MakeTask(
                                     [ret](auto selfObject) {
                                         if (auto self = interface_pointer_cast<SCENE_NS::IEcsObject>(selfObject)) {
                                             InitializeMesh(ret, self);
                                         }
                                         return false;
                                     },
                                     GetSelf()),
                false);
        }
    }
    if (auto node = interface_cast<SCENE_NS::INode>(ret)) {
        if (META_NS::GetValue(node->Status()) != SCENE_NS::INode::NODE_STATUS_DISCONNECTED) {
            META_NS::SetValue(Mesh(), ret);
        }
    }

    return ret;
}

using PropertyNameCriteria = BASE_NS::unordered_map<BASE_NS::string_view, BASE_NS::vector<BASE_NS::string_view>>;
PropertyNameCriteria& NodeImpl::PropertyNameMask()
{
    return ecs_property_names_;
}

META_NS::IObject::Ptr NodeImpl::Resolve(const META_NS::RefUri& uri) const
{
    auto p = Super::Resolve(uri);
    if (p) {
        return p;
    }

    INode::Ptr current = GetSelf<INode>();

    META_NS::RefUri ref { uri.RelativeUri() };
    if (ref.IsEmpty()) {
        return interface_pointer_cast<META_NS::IObject>(current);
    }

    if (ref.StartsFromRoot()) {
        ref.SetStartsFromRoot(false);
        auto obj = interface_pointer_cast<META_NS::IObject>(GetScene()->RootNode()->GetValue());
        return obj ? obj->Resolve(BASE_NS::move(ref)) : nullptr;
    }

    return {}; // ResolveSegment(current, BASE_NS::move(ref));
}

SCENE_BEGIN_NAMESPACE()
void RegisterNodeImpl()
{
    META_NS::GetObjectRegistry().RegisterObjectType<NodeImpl>();
}
void UnregisterNodeImpl()
{
    META_NS::GetObjectRegistry().UnregisterObjectType<NodeImpl>();
}
SCENE_END_NAMESPACE()
