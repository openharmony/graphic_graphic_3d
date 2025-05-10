
#include "internal_scene.h"

#include <chrono>
#include <inttypes.h>
#include <mutex>
#include <scene/ext/intf_converting_value.h>
#include <scene/ext/intf_create_entity.h>
#include <scene/ext/util.h>
#include <scene/interface/intf_light.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_text.h>

#include <3d/implementation_uids.h>
#include <render/intf_render_context.h>
#include <render/intf_renderer.h>

#include <meta/api/engine/util.h>
#include <meta/interface/intf_startable.h>

#include "../component/generic_component.h"
#include "../resource/ecs_animation.h"
#include "ecs_object.h"

SCENE_BEGIN_NAMESPACE()

InternalScene::InternalScene(const IScene::Ptr& scene, IRenderContext::Ptr context, SceneOptions opts)
    : scene_(scene), context_(BASE_NS::move(context)), options_(BASE_NS::move(opts))
{
    graphicsContext3D_ = CORE_NS::CreateInstance<CORE3D_NS::IGraphicsContext>(
        *context_->GetRenderer()->GetInterface<CORE_NS::IClassFactory>(), CORE3D_NS::UID_GRAPHICS_CONTEXT);
    graphicsContext3D_->Init();
}

InternalScene::~InternalScene() {}

bool InternalScene::Initialize()
{
    ecs_.reset(new Ecs);
    if (!ecs_->Initialize(self_.lock(), options_)) {
        CORE_LOG_E("failed to initialize ecs");
        return false;
    }
    return true;
}

SceneOptions InternalScene::GetOptions() const
{
    return options_;
}

void InternalScene::Uninitialize()
{
    CORE_LOG_D("InternalScene::Uninitialize");
    {
        std::unique_lock lock { mutex_ };
        syncs_.clear();
    }
    nodes_.clear();
    animations_.clear();
    componentFactories_.clear();
    renderingCameras_.clear();

    if (ecs_) {
        ecs_->Uninitialize();
    }

    if (context_->GetRenderer()) {
        // Do a "empty render" to flush out the gpu resources instantly.
        context_->GetRenderer()->GetRenderer().RenderFrame({});
    }
}

static CORE_NS::Entity CreateDefaultEntity(CORE_NS::IEcs& ecs)
{
    CORE_NS::IEntityManager& em = ecs.GetEntityManager();
    return em.Create();
}

INode::Ptr InternalScene::CreateNode(BASE_NS::string_view path, META_NS::ObjectId id)
{
    auto& r = META_NS::GetObjectRegistry();

    if (ecs_->FindNode(path)) {
        CORE_LOG_E("Node exists already [path=%s]", BASE_NS::string(path).c_str());
        return nullptr;
    }

    auto parent = ecs_->FindNodeParent(path);
    if (!parent) {
        CORE_LOG_E("No parent for node [path=%s]", BASE_NS::string(path).c_str());
        return nullptr;
    }

    if (!id.IsValid()) {
        id = ClassId::Node;
    }

    auto node = r.Create<INode>(id);
    if (!node) {
        CORE_LOG_E(
            "Failed to create node object [path=%s, id=%s]", BASE_NS::string(path).c_str(), id.ToString().c_str());
        return nullptr;
    }
    CORE_NS::Entity ent;
    if (auto ce = interface_pointer_cast<ICreateEntity>(node)) {
        ent = ce->CreateEntity(self_.lock());
    } else {
        ent = CreateDefaultEntity(*ecs_->ecs);
    }
    if (!CORE_NS::EntityUtil::IsValid(ent)) {
        CORE_LOG_E("Failed to create entity [path=%s, id=%s]", BASE_NS::string(path).c_str(), id.ToString().c_str());
        return nullptr;
    }
    ecs_->AddDefaultComponents(ent);
    ecs_->SetNodeName(ent, EntityName(path)); // remove when reworking names for scene objects
    if (!ConstructNodeImpl(ent, node)) {
        ecs_->RemoveEntity(ent);
        return nullptr;
        ;
    }
    ecs_->SetNodeParentAndName(ent, EntityName(path), parent);
    return node;
}

META_NS::IObject::Ptr InternalScene::CreateObject(META_NS::ObjectId id)
{
    auto& r = META_NS::GetObjectRegistry();
    auto md = CreateRenderContextArg(context_);
    if (md) {
        md->AddProperty(META_NS::ConstructProperty<IInternalScene::Ptr>("Scene", self_.lock()));
    }
    auto object = r.Create<META_NS::IObject>(id, md);
    if (!object) {
        CORE_LOG_E("Failed to create scene object [id=%s]", id.ToString().c_str());
        return nullptr;
    }
    if (auto acc = interface_cast<IEcsObjectAccess>(object)) {
        CORE_NS::Entity ent;
        if (auto ce = interface_pointer_cast<ICreateEntity>(object)) {
            ent = ce->CreateEntity(self_.lock());
        } else {
            ent = CreateDefaultEntity(*ecs_->ecs);
        }
        if (!CORE_NS::EntityUtil::IsValid(ent)) {
            CORE_LOG_E("Failed to create entity [id=%s]", id.ToString().c_str());
            return nullptr;
        }
        auto eobj = ecs_->GetEcsObject(ent);
        if (!acc->SetEcsObject(eobj)) {
            return nullptr;
        }
    }
    return object;
}

INode::Ptr InternalScene::ConstructNodeImpl(CORE_NS::Entity ent, INode::Ptr node) const
{
    auto acc = interface_cast<IEcsObjectAccess>(node);
    if (!acc) {
        return nullptr;
    }
    auto& r = META_NS::GetObjectRegistry();

    auto eobj = ecs_->GetEcsObject(ent);
    if (!eobj) {
        return nullptr;
    }
    AttachComponents(node, eobj, ent);

    if (!acc->SetEcsObject(eobj)) {
        return nullptr;
    }

    nodes_[ent] = node;
    return node;
}

META_NS::ObjectId InternalScene::DeducePrimaryNodeType(CORE_NS::Entity ent) const
{
    if (ecs_->cameraComponentManager->HasComponent(ent)) {
        return ClassId::CameraNode;
    }
    if (ecs_->lightComponentManager->HasComponent(ent)) {
        return ClassId::LightNode;
    }
    if (ecs_->textComponentManager && ecs_->textComponentManager->HasComponent(ent)) {
        return ClassId::TextNode;
    }
    if (ecs_->renderMeshComponentManager->HasComponent(ent)) {
        return ClassId::MeshNode;
    }
    return ClassId::Node;
}

INode::Ptr InternalScene::ConstructNode(CORE_NS::Entity ent, META_NS::ObjectId id) const
{
    auto& r = META_NS::GetObjectRegistry();
    if (!id.IsValid()) {
        id = DeducePrimaryNodeType(ent);
    }

    auto node = r.Create<INode>(id);
    if (!node) {
        CORE_LOG_E("Failed to create node object [id=%s]", id.ToString().c_str());
        return nullptr;
    }

    return ConstructNodeImpl(ent, node);
}

IComponent::Ptr InternalScene::CreateEcsComponent(const INode::Ptr& node, BASE_NS::string_view componentName)
{
    // First check we already have the component
    auto attach = interface_cast<META_NS::IAttach>(node);
    if (!attach) {
        return {};
    }
    if (auto cont = attach->GetAttachmentContainer(true)) {
        if (auto existing = cont->FindAny<IComponent>(componentName, META_NS::TraversalType::NO_HIERARCHY)) {
            return existing;
        }
    }
    // Then find a component manager with a matching name
    IEcsObject::Ptr ecso;
    if (auto acc = interface_cast<IEcsObjectAccess>(node)) {
        ecso = acc->GetEcsObject();
    }
    if (ecs_ && ecso) {
        if (auto ecs = ecs_->GetNativeEcs()) {
            for (auto&& manager : ecs->GetComponentManagers()) {
                if (manager->GetName() == componentName) {
                    // Also create the Ecs component if not there already
                    if (auto component = CreateComponent(manager, ecso, true)) {
                        // We don't call component->PopulateAllProperties() here, i.e. if the component was newly
                        // created its properties will not be populated
                        attach->Attach(component);
                        return component;
                    }
                }
            }
        }
    }
    return {};
}

IComponent::Ptr InternalScene::CreateComponent(
    CORE_NS::IComponentManager* m, const IEcsObject::Ptr& ecsObject, bool createEcsComponent) const
{
    if (!m) {
        return {};
    }
    auto& r = META_NS::GetObjectRegistry();
    IComponent::Ptr comp;
    ;
    if (createEcsComponent) {
        if (auto entity = ecsObject->GetEntity(); CORE_NS::EntityUtil::IsValid(entity)) {
            if (!m->HasComponent(entity)) {
                m->Create(entity);
            }
        }
    }
    if (auto fac = FindComponentFactory(m->GetUid())) {
        comp = fac->CreateComponent(ecsObject);
    } else {
        auto md = r.Create<META_NS::IMetadata>(META_NS::ClassId::Object);
        md->AddProperty(META_NS::ConstructProperty<BASE_NS::string>("Component", BASE_NS::string(m->GetName())));
        comp = r.Create<IComponent>(ClassId::GenericComponent, md);
        if (auto acc = interface_cast<IEcsObjectAccess>(comp)) {
            if (!acc->SetEcsObject(ecsObject)) {
                return nullptr;
            }
        }
    }
    return comp;
}

void InternalScene::AttachComponents(
    const INode::Ptr& node, const IEcsObject::Ptr& ecsObject, CORE_NS::Entity ent) const
{
    auto& r = META_NS::GetObjectRegistry();
    auto att = interface_cast<META_NS::IAttach>(node);
    if (!att) {
        return;
    }

    auto attachments = att->GetAttachmentContainer(true);

    BASE_NS::vector<CORE_NS::IComponentManager*> managers;
    ecs_->ecs->GetComponents(ent, managers);
    for (auto m : managers) {
        if (!attachments->FindByName(m->GetName())) {
            if (auto comp = CreateComponent(m, ecsObject, false)) {
                att->Attach(comp);
            } else {
                CORE_LOG_E("Failed to construct component for '%s'", BASE_NS::string(m->GetName()).c_str());
            }
        }
    }
}

INode::Ptr InternalScene::FindNode(CORE_NS::Entity ent, META_NS::ObjectId id) const
{
    if (ecs_->IsNodeEntity(ent)) {
        auto it = nodes_.find(ent);
        if (it != nodes_.end()) {
            return it->second;
        }
        return ConstructNode(ent, id);
    }

    CORE_LOG_W("Could not find entity: %" PRIu64, ent.id);
    return nullptr;
}

INode::Ptr InternalScene::FindNode(BASE_NS::string_view path, META_NS::ObjectId id) const
{
    auto npath = NormalisePath(path);
    auto n = ecs_->FindNode(npath);
    if (n) {
        auto it = nodes_.find(n->GetEntity());
        if (it != nodes_.end()) {
            return it->second;
        }
        return ConstructNode(n->GetEntity(), id);
    }

    CORE_LOG_W("Could not find node: %s", BASE_NS::string(npath).c_str());
    return nullptr;
}

INode::Ptr InternalScene::ReleaseCached(NodesType::iterator it)
{
    auto node = BASE_NS::move(it->second);
    nodes_.erase(it);
    if (auto i = interface_cast<INodeNotify>(node)) {
        if (i->IsListening()) {
            ListenNodeChanges(false);
        }
    }
    return node;
}

void InternalScene::ReleaseChildNodes(const IEcsObject::Ptr& eobj)
{
    if (auto n = ecs_->GetNode(eobj->GetEntity())) {
        for (auto&& c : n->GetChildren()) {
            auto it = nodes_.find(c->GetEntity());
            if (it != nodes_.end()) {
                auto nn = it->second;
                ReleaseNode(BASE_NS::move(nn), true);
            }
        }
    }
}

bool InternalScene::ReleaseNode(INode::Ptr&& node, bool recursive)
{
    if (node) {
        IEcsObject::Ptr eobj;
        if (auto acc = interface_cast<IEcsObjectAccess>(node)) {
            eobj = acc->GetEcsObject();
        }
        node.reset();
        if (eobj) {
            auto it = nodes_.find(eobj->GetEntity());
            if (it != nodes_.end()) {
                // are we the only owner?
                if (it->second.use_count() == 1) {
                    node = ReleaseCached(it);
                }
            }
            if (recursive) {
                ReleaseChildNodes(eobj);
            }
            if (node) {
                ecs_->RemoveEcsObject(eobj);
                return true;
            }
        }
    }
    return false;
}

bool InternalScene::RemoveNode(const INode::Ptr& node)
{
    if (node) {
        if (auto acc = interface_cast<IEcsObjectAccess>(node)) {
            if (auto eobj = acc->GetEcsObject()) {
                auto decents = ecs_->GetNodeDescendants(eobj->GetEntity());
                for (auto&& ent : decents) {
                    if (auto it = nodes_.find(ent); it != nodes_.end()) {
                        ReleaseCached(it);
                    }
                    ecs_->RemoveEntity(ent);
                }
                return true;
            }
        }
    }
    return false;
}

/// Returns a list of instantiated child nodes of root (including root) which implement INodeNotify
BASE_NS::vector<INodeNotify::Ptr> InternalScene::GetNotifiableNodesFromHierarchy(CORE_NS::Entity root)
{
    BASE_NS::vector<INodeNotify::Ptr> notify;
    auto findNode = [this](CORE_NS::Entity entity) {
        auto n = nodes_.find(entity);
        return n != nodes_.end() ? interface_pointer_cast<INodeNotify>(n->second) : nullptr;
    };
    // Add root to the list
    if (auto n = findNode(root)) {
        notify.emplace_back(BASE_NS::move(n));
    }
    // Add descendants to the list
    auto descendants = ecs_->GetNodeDescendants(root);
    notify.reserve(descendants.size() + 1);
    for (auto&& d : descendants) {
        if (auto n = findNode(d)) {
            notify.emplace_back(BASE_NS::move(n));
        }
    }
    return notify;
}

void InternalScene::SetEntityActive(const BASE_NS::shared_ptr<IEcsObject>& child, bool active)
{
    if (!child || !ecs_) {
        return;
    }
    const auto entity = child->GetEntity();
    if (!active) {
        for (auto&& node : GetNotifiableNodesFromHierarchy(entity)) {
            node->OnNodeActiveStateChanged(INodeNotify::NodeActiteStateInfo::DEACTIVATING);
        }
    }
    ecs_->SetNodesActive(entity, active);
    if (active) {
        for (auto&& node : GetNotifiableNodesFromHierarchy(entity)) {
            node->OnNodeActiveStateChanged(INodeNotify::NodeActiteStateInfo::ACTIVATED);
        }
    }
}

BASE_NS::vector<INode::Ptr> InternalScene::GetChildren(const IEcsObject::Ptr& obj) const
{
    BASE_NS::vector<INode::Ptr> res;
    if (auto node = ecs_->GetNode(obj->GetEntity())) {
        for (auto&& c : node->GetChildren()) {
            if (auto n = FindNode(c->GetEntity(), {})) {
                res.push_back(n);
            }
        }
    }
    return res;
}
bool InternalScene::RemoveChild(
    const BASE_NS::shared_ptr<IEcsObject>& object, const BASE_NS::shared_ptr<IEcsObject>& child)
{
    bool ret = false;
    if (auto node = ecs_->GetNode(object->GetEntity())) {
        if (auto childNode = ecs_->GetNode(child->GetEntity())) {
            ret = node->RemoveChild(*childNode);
            if (ret) {
                SetEntityActive(child, false);
            }
        }
    }
    return ret;
}
bool InternalScene::AddChild(const BASE_NS::shared_ptr<IEcsObject>& object, const INode::Ptr& child, size_t index)
{
    bool ret = false;
    if (ecs_->IsNodeEntity(object->GetEntity())) {
        if (auto acc = interface_cast<IEcsObjectAccess>(child)) {
            auto ecsobj = acc->GetEcsObject();
            SetEntityActive(ecsobj, true);
            if (auto node = ecs_->GetNode(object->GetEntity())) {
                if (auto childNode = ecs_->GetNode(ecsobj->GetEntity())) {
                    if (node->InsertChild(index, *childNode)) {
                        nodes_[ecsobj->GetEntity()] = child;
                        ret = true;
                    }
                }
            }
        }
    }
    return ret;
}

BASE_NS::shared_ptr<IScene> InternalScene::GetScene() const
{
    return scene_.lock();
}

void InternalScene::SchedulePropertyUpdate(const IEcsObject::Ptr& obj)
{
    std::unique_lock lock { mutex_ };
    syncs_[obj.get()] = obj;
}

void InternalScene::SyncProperties()
{
    UpdateSyncProperties(false);
}

bool InternalScene::UpdateSyncProperties(bool resetPending)
{
    bool pending = false;
    BASE_NS::unordered_map<void*, IEcsObject::WeakPtr> syncs;
    {
        std::unique_lock lock { mutex_ };
        syncs = BASE_NS::move(syncs_);
        pending = pendingRender_;
        if (resetPending) {
            pendingRender_ = false;
        }
    }

    for (auto&& v : syncs) {
        if (auto o = v.second.lock()) {
            o->SyncProperties();
        }
    }
    return pending;
}

void InternalScene::Update(bool syncProperties)
{
    using namespace std::chrono;
    bool pending;
    if (syncProperties) {
        pending = UpdateSyncProperties(true);
    } else {
        std::unique_lock lock { mutex_ };
        pending = pendingRender_;
        pendingRender_ = false;
    }

    ecs_->ecs->ProcessEvents();

    const auto currentTime =
        static_cast<uint64_t>(duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch()).count());

    if (firstTime_ == ~0u) {
        previousFrameTime_ = firstTime_ = currentTime;
    }
    auto deltaTime = currentTime - previousFrameTime_;
    constexpr auto limitHz = duration_cast<microseconds>(duration<float, std::ratio<1, 15u>>(1)).count();
    if (deltaTime > limitHz) {
        deltaTime = limitHz; // clamp the time step to no longer than 15hz.
    }
    previousFrameTime_ = currentTime;
    const uint64_t totalTime = currentTime - firstTime_;

    bool needsRender = ecs_->ecs->Update(totalTime, deltaTime);

    ecs_->ecs->ProcessEvents();

    if ((needsRender && mode_ != RenderMode::MANUAL) || pending) {
        auto renderHandles = graphicsContext3D_->GetRenderNodeGraphs(*ecs_->ecs);
        if (!renderHandles.empty()) {
            // The scene needs to be rendered.
            RENDER_NS::IRenderer& renderer = context_->GetRenderer()->GetRenderer();
            renderer.RenderDeferred(renderHandles);

            NotifyRenderingCameras();
        }
    }
}

void InternalScene::NotifyRenderingCameras()
{
    for (auto it = renderingCameras_.begin(); it != renderingCameras_.end();) {
        if (auto c = it->lock()) {
            c->NotifyRenderTargetChanged();
            ++it;
        } else {
            it = renderingCameras_.erase(it);
        }
    }
}

BASE_NS::vector<ICamera::Ptr> InternalScene::GetCameras() const
{
    BASE_NS::vector<ICamera::Ptr> ret;

    for (size_t i = 0; i != ecs_->cameraComponentManager->GetComponentCount(); ++i) {
        if (auto n = interface_pointer_cast<ICamera>(
                FindNode(ecs_->cameraComponentManager->GetEntity(i), ClassId::CameraNode))) {
            ret.push_back(n);
        }
    }

    return ret;
}

BASE_NS::vector<META_NS::IAnimation::Ptr> InternalScene::GetAnimations() const
{
    BASE_NS::vector<META_NS::IAnimation::Ptr> ret;

    for (size_t i = 0; i != ecs_->animationComponentManager->GetComponentCount(); ++i) {
        auto ent = ecs_->animationComponentManager->GetEntity(i);
        META_NS::IAnimation::Ptr anim;
        if (auto it = animations_.find(ent); it != animations_.end()) {
            anim = it->second.lock();
        }
        if (!anim) {
            anim = META_NS::GetObjectRegistry().Create<META_NS::IAnimation>(ClassId::EcsAnimation);
            if (auto acc = interface_cast<IEcsObjectAccess>(anim)) {
                acc->SetEcsObject(ecs_->GetEcsObject(ent));
                animations_[ent] = anim;
            }
        }
        if (anim) {
            ret.push_back(anim);
        } else {
            CORE_LOG_W("Failed to create EcsAnimation object");
        }
    }

    return ret;
}

void InternalScene::RegisterComponent(const BASE_NS::Uid& id, const IComponentFactory::Ptr& p)
{
    componentFactories_[id] = p;
}

void InternalScene::UnregisterComponent(const BASE_NS::Uid& id)
{
    componentFactories_.erase(id);
}

BASE_NS::shared_ptr<IComponentFactory> InternalScene::FindComponentFactory(const BASE_NS::Uid& id) const
{
    auto it = componentFactories_.find(id);
    return it != componentFactories_.end() ? it->second : nullptr;
}

bool InternalScene::SyncProperty(const META_NS::IProperty::ConstPtr& p, META_NS::EngineSyncDirection dir)
{
    META_NS::IEngineValue::Ptr value = GetEngineValueFromProperty(p);
    if (!value) {
        if (auto i = META_NS::GetFirstValueFromProperty<IConvertingValue>(p)) {
            value = GetEngineValueFromProperty(i->GetTargetProperty());
        }
    }
    META_NS::InterfaceUniqueLock valueLock { value };
    return value && value->Sync(dir);
}

void InternalScene::AddRenderingCamera(const IInternalCamera::Ptr& camera)
{
    for (auto&& v : renderingCameras_) {
        if (camera == v.lock()) {
            return;
        }
    }
    renderingCameras_.push_back(camera);
}
void InternalScene::RemoveRenderingCamera(const IInternalCamera::Ptr& camera)
{
    for (auto it = renderingCameras_.begin(); it != renderingCameras_.end(); ++it) {
        if (camera == it->lock()) {
            renderingCameras_.erase(it);
            return;
        }
    }
}

bool InternalScene::SetRenderMode(RenderMode mode)
{
    mode_ = mode;
    ecs_->ecs->SetRenderMode(
        mode == RenderMode::IF_DIRTY ? CORE_NS::IEcs::RENDER_IF_DIRTY : CORE_NS::IEcs::RENDER_ALWAYS);
    return true;
}
RenderMode InternalScene::GetRenderMode() const
{
    return mode_;
}
void InternalScene::RenderFrame()
{
    std::unique_lock lock { mutex_ };
    pendingRender_ = true;
}
bool InternalScene::HasPendingRender() const
{
    std::unique_lock lock { mutex_ };
    return pendingRender_;
}

NodeHits InternalScene::MapHitResults(
    const BASE_NS::vector<CORE3D_NS::RayCastResult>& res, const RayCastOptions& options) const
{
    NodeHits result;
    CORE3D_NS::ISceneNode* n = nullptr;
    if (auto obj = interface_cast<IEcsObjectAccess>(options.node)) {
        if (auto ecs = obj->GetEcsObject()) {
            n = ecs_->GetNode(ecs->GetEntity());
        }
    }

    for (auto&& v : res) {
        NodeHit h;
        if (v.node && (!n || n->IsAncestorOf(*v.node))) {
            h.node = FindNode(v.node->GetEntity(), {});
            h.distance = v.distance;
            h.distanceToCenter = v.centerDistance;
            h.position = v.worldPosition;
            result.push_back(BASE_NS::move(h));
        }
    }
    return result;
}

NodeHits InternalScene::CastRay(
    const BASE_NS::Math::Vec3& pos, const BASE_NS::Math::Vec3& dir, const RayCastOptions& options) const
{
    NodeHits result;
    if (ecs_->picking) {
        result = MapHitResults(ecs_->picking->RayCast(*ecs_->ecs, pos, dir, options.layerMask), options);
    }
    return result;
}
NodeHits InternalScene::CastRay(
    const IEcsObject::ConstPtr& entity, const BASE_NS::Math::Vec2& pos, const RayCastOptions& options) const
{
    NodeHits result;
    if (ecs_->picking) {
        result = MapHitResults(
            ecs_->picking->RayCastFromCamera(*ecs_->ecs, entity->GetEntity(), pos, options.layerMask), options);
    }
    return result;
}
BASE_NS::Math::Vec3 InternalScene::ScreenPositionToWorld(
    const IEcsObject::ConstPtr& entity, const BASE_NS::Math::Vec3& pos) const
{
    BASE_NS::Math::Vec3 result;
    if (ecs_->picking) {
        result = ecs_->picking->ScreenToWorld(*ecs_->ecs, entity->GetEntity(), pos);
    }
    return result;
}
BASE_NS::Math::Vec3 InternalScene::WorldPositionToScreen(
    const IEcsObject::ConstPtr& entity, const BASE_NS::Math::Vec3& pos) const
{
    BASE_NS::Math::Vec3 result;
    if (ecs_->picking) {
        result = ecs_->picking->WorldToScreen(*ecs_->ecs, entity->GetEntity(), pos);
    }
    return result;
}

void InternalScene::ListenNodeChanges(bool enabled)
{
    bool change = false;
    if (enabled) {
        change = !nodeListening_++;
    } else if (nodeListening_ > 0) {
        change = !--nodeListening_;
    }
    if (change) {
        ecs_->ListenNodeChanges(enabled);
    }
}

void InternalScene::OnChildChanged(
    META_NS::ContainerChangeType type, CORE_NS::Entity parent, CORE_NS::Entity childEntity, size_t index)
{
    if (auto it = nodes_.find(parent); it != nodes_.end()) {
        if (auto i = interface_cast<INodeNotify>(it->second)) {
            if (auto child = FindNode(childEntity, {})) {
                i->OnChildChanged(type, child, index);
            } else {
                CORE_LOG_W("child changed but cannot construct it?!");
            }
        }
    }
}

SCENE_END_NAMESPACE()
