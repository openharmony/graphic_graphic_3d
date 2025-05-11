
#ifndef SCENE_SRC_NODE_MESH_NODE_H
#define SCENE_SRC_NODE_MESH_NODE_H

#include <scene/ext/intf_create_entity.h>
#include <scene/interface/intf_mesh.h>

#include "../component/mesh_component.h"
#include "../mesh/mesh.h"
#include "node.h"

SCENE_BEGIN_NAMESPACE()

class MeshNode : public META_NS::IntroduceInterfaces<Node, IMesh, IMeshAccess, ICreateEntity, IMorphAccess> {
    META_OBJECT(MeshNode, ClassId::MeshNode, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_FORWARDED_PROPERTY_DATA(IMesh, BASE_NS::Math::Vec3, AABBMin)
    META_STATIC_FORWARDED_PROPERTY_DATA(IMesh, BASE_NS::Math::Vec3, AABBMax)
    META_STATIC_FORWARDED_ARRAY_PROPERTY_DATA(IMesh, ISubMesh::Ptr, SubMeshes)
    META_STATIC_PROPERTY_DATA(IMorphAccess, IMorpher::Ptr, Morpher)
    META_END_STATIC_DATA()

    META_FORWARD_READONLY_PROPERTY(BASE_NS::Math::Vec3, AABBMin, mesh_->AABBMin())
    META_FORWARD_READONLY_PROPERTY(BASE_NS::Math::Vec3, AABBMax, mesh_->AABBMax())
    META_FORWARD_READONLY_ARRAY_PROPERTY(ISubMesh::Ptr, SubMeshes, mesh_->SubMeshes())
    META_IMPLEMENT_READONLY_PROPERTY(IMorpher::Ptr, Morpher)

    Future<bool> SetMesh(const IMesh::Ptr&) override;
    Future<IMesh::Ptr> GetMesh() const override;

    bool SetEcsObject(const IEcsObject::Ptr&) override;

public:
    CORE_NS::Entity CreateEntity(const IInternalScene::Ptr& scene) override;
    void SetOwnedEntity(CORE_NS::Entity ent);

private:
    bool Init(const IInternalRenderMesh::Ptr&);
    bool Init(const IEcsObject::Ptr&);

private:
    IMesh::Ptr mesh_;
};

SCENE_END_NAMESPACE()

#endif
