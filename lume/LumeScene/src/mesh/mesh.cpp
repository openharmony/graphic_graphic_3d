
#include "mesh.h"

#include <3d/ecs/components/mesh_component.h>
#include <3d/util/intf_picking.h>

#include <meta/api/make_callback.h>

#include "../util_interfaces.h"

SCENE_BEGIN_NAMESPACE()

void Mesh::Destroy()
{
    if (META_NS::ArrayProperty<ISubMesh::Ptr> arr { GetProperty("SubMeshes", META_NS::MetadataQuery::EXISTING) }) {
        for (auto&& v : arr->GetValue()) {
            if (auto i = interface_cast<META_NS::INotifyOnChange>(v)) {
                i->OnChanged()->RemoveHandler(uintptr_t(this));
            }
        }
    }
    Super::Destroy();
}

void Mesh::SetInternalMesh(IInternalMesh::Ptr m)
{
    submeshEvent_.Unsubscribe();
    mesh_ = std::move(m);
    if (mesh_) {
        submeshEvent_.Subscribe<META_NS::IOnChanged>(
            mesh_->SubMeshes()->OnChanged(), [this] { UpdateSubmeshes(SubMeshes()); });
    }
}

IEcsObject::Ptr Mesh::GetMeshEcsObject() const
{
    auto comp = interface_cast<IEcsObjectAccess>(mesh_);
    return comp ? comp->GetEcsObject() : nullptr;
}

bool Mesh::SetEcsObject(const IEcsObject::Ptr& obj)
{
    if (Super::SetEcsObject(obj)) {
        if (auto att = GetSelf<META_NS::IAttach>()) {
            if (auto attc = att->GetAttachmentContainer(false)) {
                if (auto c = attc->FindByName<IInternalMesh>("MeshComponent")) {
                    SetInternalMesh(c);
                }
            }
        }
        if (!mesh_) {
            auto p = META_NS::GetObjectRegistry().Create<IInternalMesh>(ClassId::MeshComponent);
            if (auto acc = interface_cast<IEcsObjectAccess>(p)) {
                if (acc->SetEcsObject(obj)) {
                    SetInternalMesh(p);
                    if (auto scene = obj->GetScene()) {
                        auto ecs = scene->GetEcsContext().GetNativeEcs();
                        meshEntity_ = ecs->GetEntityManager().GetReferenceCounted(obj->GetEntity());
                    }
                }
            }
        }
    }
    return mesh_ != nullptr;
}

CORE_NS::Entity Mesh::CreateEntity(const IInternalScene::Ptr& scene)
{
    const auto& ecs = scene->GetEcsContext().GetNativeEcs();
    const auto meshManager = CORE_NS::GetManager<CORE3D_NS::IMeshComponentManager>(*ecs);
    if (!meshManager) {
        return {};
    }
    auto ent = ecs->GetEntityManager().Create();
    meshManager->Create(ent);
    return ent;
}

bool Mesh::InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    if (p->GetName() == "SubMeshes") {
        return UpdateSubmeshes(META_NS::ArrayProperty<ISubMesh::Ptr>(p));
    }
    return false;
}

void Mesh::RecalculateAABB()
{
    CORE3D_NS::MinAndMax minMax;
    for (const auto& submesh : SubMeshes()->GetValue()) {
        minMax.minAABB = min(minMax.minAABB, submesh->AABBMin()->GetValue());
        minMax.maxAABB = max(minMax.maxAABB, submesh->AABBMax()->GetValue());
    }
    mesh_->AABBMin()->SetValue(minMax.minAABB);
    mesh_->AABBMax()->SetValue(minMax.maxAABB);
}

bool Mesh::UpdateSubmeshes(META_NS::ArrayProperty<ISubMesh::Ptr> prop)
{
    if (!prop) {
        return false;
    }
    auto& r = META_NS::GetObjectRegistry();
    BASE_NS::vector<ISubMesh::Ptr> orig = prop->GetValue();
    BASE_NS::vector<ISubMesh::Ptr> subs;
    auto msubs = mesh_->SubMeshes()->GetValue();
    for (size_t i = 0; i != msubs.size(); ++i) {
        ISubMesh::Ptr t;
        if (i < orig.size()) {
            t = orig[i];
        } else {
            t = r.Create<ISubMesh>(ClassId::SubMesh);
            if (auto si = interface_cast<IArrayElementIndex>(t)) {
                si->SetIndex(i);
            }
            if (auto i = interface_cast<META_NS::INotifyOnChange>(t)) {
                i->OnChanged()->AddHandler(
                    META_NS::MakeCallback<META_NS::IOnChanged>([this] { RecalculateAABB(); }), uintptr_t(this));
            }
        }
        if (auto access = interface_cast<IEcsObjectAccess>(t)) {
            access->SetEcsObject(GetEcsObject());
            subs.push_back(t);
        } else {
            CORE_LOG_E("Failed to create submesh for mesh");
            return false;
        }
    }
    if (orig.size() > subs.size()) {
        for (auto it = orig.begin() + subs.size(); it != orig.end(); ++it) {
            if (auto i = interface_cast<META_NS::INotifyOnChange>(*it)) {
                i->OnChanged()->RemoveHandler(uintptr_t(this));
            }
        }
    }
    RefreshSubmeshes(subs);
    prop->SetValue(subs);
    return true;
}
void Mesh::RefreshSubmeshes(const BASE_NS::vector<ISubMesh::Ptr>& subs)
{
    if (auto scene = GetInternalScene()) {
        scene
            ->AddTask([&] {
                for (auto&& s : subs) {
                    if (auto m = interface_cast<META_NS::IMetadata>(s)) {
                        for (auto&& p : m->GetProperties()) {
                            scene->SyncProperty(p, META_NS::EngineSyncDirection::AUTO);
                        }
                    }
                }
            })
            .Wait();
    }
}
SCENE_END_NAMESPACE()
