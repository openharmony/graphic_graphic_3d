/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include <3d/ecs/components/mesh_component.h>
#include <core/ecs/intf_ecs.h>
#include <core/plugin/intf_class_factory.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"

#define IMPLEMENT_MANAGER
#include "PropertyTools/property_macros.h"

CORE_BEGIN_NAMESPACE()
using BASE_NS::vector;
using CORE3D_NS::MeshComponent;

DECLARE_PROPERTY_TYPE(MeshComponent::Submesh);
DECLARE_PROPERTY_TYPE(MeshComponent::Submesh::BufferAccess);
DECLARE_PROPERTY_TYPE(MeshComponent::Submesh::IndexBufferAccess);
DECLARE_PROPERTY_TYPE(vector<MeshComponent::Submesh>);

DECLARE_PROPERTY_TYPE(RENDER_NS::IndexType);
BEGIN_ENUM(IndexTypeMetaData, RENDER_NS::IndexType)
DECL_ENUM(RENDER_NS::IndexType, CORE_INDEX_TYPE_UINT16, "16 bit")
DECL_ENUM(RENDER_NS::IndexType, CORE_INDEX_TYPE_UINT32, "32 bit")
END_ENUM(IndexTypeMetaData, RENDER_NS::IndexType)

BEGIN_ENUM(SubmeshFlagBitsMetaData, MeshComponent::Submesh::FlagBits)
DECL_ENUM(MeshComponent::Submesh::FlagBits, TANGENTS_BIT, "Tangents")
DECL_ENUM(MeshComponent::Submesh::FlagBits, VERTEX_COLORS_BIT, "Vertex Colors")
DECL_ENUM(MeshComponent::Submesh::FlagBits, SKIN_BIT, "Skin")
DECL_ENUM(MeshComponent::Submesh::FlagBits, SECOND_TEXCOORD_BIT, "Second Texcoord")
END_ENUM(SubmeshFlagBitsMetaData, MeshComponent::Submesh::FlagBits)

BEGIN_METADATA(BAMetaData, MeshComponent::Submesh::BufferAccess)
DECL_PROPERTY2(MeshComponent::Submesh::BufferAccess, buffer, "", 0)
DECL_PROPERTY2(MeshComponent::Submesh::BufferAccess, offset, "", 0)
DECL_PROPERTY2(MeshComponent::Submesh::BufferAccess, byteSize, "", 0)
END_METADATA(BAMetaData, MeshComponent::Submesh::BufferAccess)

BEGIN_METADATA(IBAMetaData, MeshComponent::Submesh::IndexBufferAccess)
DECL_PROPERTY2(MeshComponent::Submesh::IndexBufferAccess, buffer, "", 0)
DECL_PROPERTY2(MeshComponent::Submesh::IndexBufferAccess, offset, "", 0)
DECL_PROPERTY2(MeshComponent::Submesh::IndexBufferAccess, byteSize, "", 0)
DECL_PROPERTY2(MeshComponent::Submesh::IndexBufferAccess, indexType, "", 0)
END_METADATA(IBAMetaData, MeshComponent::Submesh::IndexBufferAccess)

BEGIN_METADATA(SMDMetaData, MeshComponent::Submesh)
/* Expose more properties? */
DECL_PROPERTY2(MeshComponent::Submesh, instanceCount, "", 0)
DECL_PROPERTY2(MeshComponent::Submesh, bufferAccess, "", 0)
DECL_PROPERTY2(MeshComponent::Submesh, indexBuffer, "", 0)
DECL_PROPERTY2(MeshComponent::Submesh, indirectArgsBuffer, "", 0)
DECL_PROPERTY2(MeshComponent::Submesh, morphTargetBuffer, "", 0)
DECL_PROPERTY2(MeshComponent::Submesh, vertexCount, "", 0)
DECL_PROPERTY2(MeshComponent::Submesh, indexCount, "", 0)
DECL_PROPERTY2(MeshComponent::Submesh, morphTargetCount, "", 0)

DECL_PROPERTY2(MeshComponent::Submesh, aabbMin, "", 0)
DECL_PROPERTY2(MeshComponent::Submesh, aabbMax, "", 0)
DECL_PROPERTY2(MeshComponent::Submesh, material, "", 0)
DECL_PROPERTY2(MeshComponent::Submesh, additionalMaterials, "", 0)
DECL_BITFIELD_PROPERTY(MeshComponent::Submesh, flags, "", PropertyFlags::IS_BITFIELD, MeshComponent::Submesh::FlagBits)
DECL_PROPERTY2(MeshComponent::Submesh, renderSortLayer, "", 0)
DECL_PROPERTY2(MeshComponent::Submesh, renderSortLayerOrder, "", 0)
END_METADATA(SMDMetaData, MeshComponent::Submesh)

DECLARE_CONTAINER_API(MPD, MeshComponent::Submesh);
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;

class MeshComponentManager final : public BaseManager<MeshComponent, IMeshComponentManager> {
    BEGIN_PROPERTY(MeshComponent, ComponentMetadata)
#include <3d/ecs/components/mesh_component.h>
    END_PROPERTY();
    const array_view<const Property> componentMetaData_ { ComponentMetadata, countof(ComponentMetadata) };

public:
    explicit MeshComponentManager(IEcs& ecs)
        : BaseManager<MeshComponent, IMeshComponentManager>(ecs, CORE_NS::GetName<MeshComponent>())
    {}

    ~MeshComponentManager() = default;

    size_t PropertyCount() const override
    {
        return componentMetaData_.size();
    }

    const Property* MetaData(size_t index) const override
    {
        if (index < componentMetaData_.size()) {
            return &componentMetaData_[index];
        }
        return nullptr;
    }

    array_view<const Property> MetaData() const override
    {
        return componentMetaData_;
    }
};

IComponentManager* IMeshComponentManagerInstance(IEcs& ecs)
{
    return new MeshComponentManager(ecs);
}

void IMeshComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<MeshComponentManager*>(instance);
}
CORE_END_NAMESPACE()
