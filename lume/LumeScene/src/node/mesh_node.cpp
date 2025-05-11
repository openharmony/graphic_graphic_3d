
#include "mesh_node.h"

#include <3d/ecs/components/mesh_component.h>
#include <3d/util/intf_scene_util.h>

#include <meta/api/make_callback.h>
#include <meta/interface/property/array_property.h>

#include "../ecs_component/entity_owner_component.h"
#include "../mesh/submesh.h"

SCENE_BEGIN_NAMESPACE()

bool MeshNode::SetEcsObject(const IEcsObject::Ptr& o)
{
    if (Super::SetEcsObject(o)) {
        auto att = GetSelf<META_NS::IAttach>()->GetAttachments<IInternalRenderMesh>();
        if (!att.empty()) {
            return Init(att.front());
        }
    }
    return false;
}

CORE_NS::Entity MeshNode::CreateEntity(const IInternalScene::Ptr& scene)
{
    const auto& ecs = scene->GetEcsContext().GetNativeEcs();
    const auto renderMeshManager = CORE_NS::GetManager<CORE3D_NS::IRenderMeshComponentManager>(*ecs);
    const auto ownerManager = CORE_NS::GetManager<IEntityOwnerComponentManager>(*ecs);
    const auto meshManager = CORE_NS::GetManager<CORE3D_NS::IMeshComponentManager>(*ecs);
    if (!renderMeshManager || !meshManager || !ownerManager) {
        return {};
    }
    const auto entity = ecs->GetEntityManager().Create();
    renderMeshManager->Create(entity);
    const auto handle = renderMeshManager->Write(entity);
    if (!handle) {
        ecs->GetEntityManager().Destroy(entity);
        return {};
    }
    handle->mesh = ecs->GetEntityManager().Create();
    meshManager->Create(handle->mesh);

    ownerManager->Create(entity);
    const auto ownerHandle = ownerManager->Write(entity);
    if (!ownerHandle) {
        ecs->GetEntityManager().Destroy(entity);
        ecs->GetEntityManager().Destroy(handle->mesh);
        return {};
    }
    ownerHandle->entity = ecs->GetEntityManager().GetReferenceCounted(handle->mesh);
    return entity;
}

bool MeshNode::Init(const IInternalRenderMesh::Ptr& rmesh)
{
    auto ent = rmesh->Mesh()->GetValue();
    if (!CORE_NS::EntityUtil::IsValid(ent)) {
        return false;
    }
    auto ecsobj = object_->GetScene()->GetEcsContext().GetEcsObject(ent);
    auto res = Init(ecsobj);
    return res;
}

bool MeshNode::Init(const IEcsObject::Ptr& ecsobj)
{
    if (!ecsobj) {
        return false;
    }
    mesh_ = META_NS::GetObjectRegistry().Create<IMesh>(ClassId::Mesh);
    if (!mesh_) {
        return false;
    }
    if (auto acc = interface_cast<IEcsObjectAccess>(mesh_)) {
        if (!acc->SetEcsObject(ecsobj)) {
            return false;
        }
    }
    if (auto morph = GetAttachments({ IMorpher::UID }, false); !morph.empty()) {
        Morpher()->SetValue(interface_pointer_cast<IMorpher>(morph.front()));
    }
    return true;
}

void MeshNode::SetOwnedEntity(CORE_NS::Entity ent)
{
    if (auto obj = GetEcsObject()) {
        auto ecs = obj->GetScene()->GetEcsContext().GetNativeEcs();
        auto ownerManager = CORE_NS::GetManager<IEntityOwnerComponentManager>(*ecs);
        if (ownerManager) {
            if (auto ownerHandle = ownerManager->Write(obj->GetEntity())) {
                ownerHandle->entity = ecs->GetEntityManager().GetReferenceCounted(ent);
            }
        }
    }
}

Future<bool> MeshNode::SetMesh(const IMesh::Ptr& m)
{
    if (auto obj = GetEcsObject()) {
        auto scene = obj->GetScene();
        return scene->AddTask([=] {
            IMesh::Ptr mesh = m;
            // in case it is mesh node, use the embedded mesh
            if (auto acc = interface_cast<IMeshAccess>(mesh)) {
                mesh = acc->GetMesh().GetResult();
            }
            if (auto objacc = interface_cast<IEcsObjectAccess>(mesh)) {
                if (auto obj = objacc->GetEcsObject()) {
                    auto att = GetSelf<META_NS::IAttach>()->GetAttachments<IInternalRenderMesh>();
                    if (!att.empty()) {
                        auto prop = att.front()->Mesh();
                        prop->SetValue(obj->GetEntity());
                        scene->SyncProperty(prop, META_NS::EngineSyncDirection::TO_ENGINE);
                        mesh_ = mesh;
                        SetOwnedEntity(obj->GetEntity());
                        return true;
                    }
                }
            }
            return false;
        });
    }
    return {};
}
Future<IMesh::Ptr> MeshNode::GetMesh() const
{
    if (auto obj = GetEcsObject()) {
        auto scene = obj->GetScene();
        return scene->AddTask([=] { return mesh_; });
    }
    return {};
}

SCENE_END_NAMESPACE()
