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

#include "internal_scene.h"

#include <cinttypes>
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

#include "../component/generic_component.h"
#include "../ecs_animation.h"
#include "ecs_object.h"

SCENE_BEGIN_NAMESPACE()

InternalScene::InternalScene(const IScene::Ptr& scene, META_NS::ITaskQueue::Ptr engine, META_NS::ITaskQueue::Ptr app,
    BASE_NS::shared_ptr<RENDER_NS::IRenderContext> context)
    : scene_(scene), engineQueue_(BASE_NS::move(engine)), appQueue_(BASE_NS::move(app)),
      renderContext_(BASE_NS::move(context))
{
    graphicsContext3D_ = CORE_NS::CreateInstance<CORE3D_NS::IGraphicsContext>(
        *renderContext_->GetInterface<CORE_NS::IClassFactory>(), CORE3D_NS::UID_GRAPHICS_CONTEXT);
    graphicsContext3D_->Init();
}

InternalScene::~InternalScene() {}

bool InternalScene::Initialize()
{
    ecs_.reset(new Ecs);
    if (!ecs_->Initialize(self_.lock())) {
        CORE_LOG_E("failed to initialize ecs");
        return false;
    }
    return true;
}

void InternalScene::Uninitialize()
{
    CORE_LOG_D("InternalScene::Uninitialize");
    {
        std::unique_lock lock { mutex_ };
        syncs_.clear();
    }
    nodes_.clear();

    ecs_->Uninitialize();
}

META_NS::ITaskQueue::Ptr InternalScene::GetEngineTaskQueue() const
{
    return engineQueue_;
}
META_NS::ITaskQueue::Ptr InternalScene::GetAppTaskQueue() const
{
    return appQueue_;
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
    ecs_->SetNodeParentAndName(ent, EntityName(path), parent);
    return ConstructNodeImpl(ent, node);
}

META_NS::IObject::Ptr InternalScene::CreateObject(META_NS::ObjectId id)
{
    auto& r = META_NS::GetObjectRegistry();
    auto md = r.Create<META_NS::IMetadata>(META_NS::ClassId::Object);
    md->AddProperty(META_NS::ConstructProperty<IInternalScene::Ptr>("Scene", self_.lock()));
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

IComponent::Ptr InternalScene::CreateComponent(CORE_NS::IComponentManager* m, const IEcsObject::Ptr& ecsObject) const
{
    auto& r = META_NS::GetObjectRegistry();
    IComponent::Ptr comp;
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
            if (auto comp = CreateComponent(m, ecsObject)) {
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
        auto path = ecs_->GetPath(ent);
        if (!path.empty()) {
            auto it = nodes_.find(ent);
            if (it != nodes_.end()) {
                return it->second;
            }
            return ConstructNode(ent, id);
        }
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

void InternalScene::RecursiveRemoveNodes(const INode::Ptr& node)
{
    if (node) {
        IEcsObject::Ptr eobj;
        if (auto acc = interface_cast<IEcsObjectAccess>(node)) {
            eobj = acc->GetEcsObject();
            nodes_.erase(eobj->GetEntity());
        }
        for (auto& n : node->GetChildren().GetResult()) {
            RecursiveRemoveNodes(n);
        }
        if (eobj) {
            ecs_->RemoveEcsObject(eobj);
        }
    }
}

void InternalScene::ReleaseNode(const INode::Ptr& node)
{
    RecursiveRemoveNodes(node);
    if (auto acc = interface_cast<IEcsObjectAccess>(node)) {
        ecs_->RemoveNode(acc->GetEcsObject()->GetEntity());
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
        }
    }
    return ret;
}
bool InternalScene::AddChild(const BASE_NS::shared_ptr<IEcsObject>& object, const INode::Ptr& child, size_t index)
{
    bool ret = false;
    if (auto node = ecs_->GetNode(object->GetEntity())) {
        if (auto acc = interface_cast<IEcsObjectAccess>(child)) {
            auto ecsobj = acc->GetEcsObject();
            if (auto childNode = ecs_->GetNode(ecsobj->GetEntity())) {
                if (node->InsertChild(index, *childNode)) {
                    nodes_[ecsobj->GetEntity()] = child;
                    ret = true;
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
    BASE_NS::unordered_map<void*, IEcsObject::WeakPtr> syncs;
    {
        std::unique_lock lock { mutex_ };
        // workaround for unordered_map bug where move op= doesn't clear the shift_amount.
        syncs = BASE_NS::unordered_map<void*, IEcsObject::WeakPtr>(BASE_NS::move(syncs_));
    }

    for (auto&& v : syncs) {
        if (auto o = v.second.lock()) {
            o->SyncProperties();
        }
    }
}

void InternalScene::Update()
{
    SyncProperties();

    ecs_->ecs->ProcessEvents();
    using namespace std::chrono;
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
    bool needsRender = false;
    {
        needsRender = ecs_->ecs->Update(totalTime, deltaTime);
    }
    ecs_->ecs->ProcessEvents();

    if (needsRender) {
        auto renderHandles = graphicsContext3D_->GetRenderNodeGraphs(*ecs_->ecs);
        if (!renderHandles.empty()) {
            // The scene needs to be rendered.
            RENDER_NS::IRenderer& renderer = renderContext_->GetRenderer();
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
    return ecs_->SetRenderMode(mode);
}
RenderMode InternalScene::GetRenderMode() const
{
    return ecs_->GetRenderMode();
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
            h.node = FindNode(ecs_->GetPath(v.node), {});
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

SCENE_END_NAMESPACE()
