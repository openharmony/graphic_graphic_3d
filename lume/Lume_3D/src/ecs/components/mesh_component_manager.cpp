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

#include <3d/ecs/components/mesh_component.h>
#include <core/ecs/intf_ecs.h>
#include <core/plugin/intf_class_factory.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"

#define IMPLEMENT_MANAGER
#include <core/property_tools/property_macros.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::vector;
using CORE3D_NS::MeshComponent;

DECLARE_PROPERTY_TYPE(MeshComponent::Submesh);
DECLARE_PROPERTY_TYPE(MeshComponent::Submesh::BufferAccess);
DECLARE_PROPERTY_TYPE(MeshComponent::Submesh::IndexBufferAccess);
DECLARE_PROPERTY_TYPE(vector<MeshComponent::Submesh>);
DECLARE_PROPERTY_TYPE(RENDER_NS::GraphicsState::InputAssembly);
DECLARE_PROPERTY_TYPE(RENDER_NS::PrimitiveTopology);

ENUM_TYPE_METADATA(RENDER_NS::PrimitiveTopology, ENUM_VALUE(CORE_PRIMITIVE_TOPOLOGY_POINT_LIST, "Point List"),
    ENUM_VALUE(CORE_PRIMITIVE_TOPOLOGY_LINE_LIST, "Line List"),
    ENUM_VALUE(CORE_PRIMITIVE_TOPOLOGY_LINE_STRIP, "Line Strip"),
    ENUM_VALUE(CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, "Triangle Strip"),
    ENUM_VALUE(CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN, "Triangle Fan"),
    ENUM_VALUE(CORE_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY, "Line List With Adjacency"),
    ENUM_VALUE(CORE_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY, "Line Strip With Adjacency"),
    ENUM_VALUE(CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY, "Triangle List With Adjacency"),
    ENUM_VALUE(CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY, "Triangle Strip With Adjacency"),
    ENUM_VALUE(CORE_PRIMITIVE_TOPOLOGY_PATCH_LIST, "Patch List"),
    ENUM_VALUE(CORE_PRIMITIVE_TOPOLOGY_MAX_ENUM, "Undefined"))

DECLARE_PROPERTY_TYPE(RENDER_NS::IndexType);

ENUM_TYPE_METADATA(
    RENDER_NS::IndexType, ENUM_VALUE(CORE_INDEX_TYPE_UINT16, "16 Bit"), ENUM_VALUE(CORE_INDEX_TYPE_UINT32, "32 Bit"))

ENUM_TYPE_METADATA(MeshComponent::Submesh::FlagBits, ENUM_VALUE(TANGENTS_BIT, "Tangents"),
    ENUM_VALUE(VERTEX_COLORS_BIT, "Vertex Colors"), ENUM_VALUE(SKIN_BIT, "Skin"),
    ENUM_VALUE(SECOND_TEXCOORD_BIT, "Second Texcoord"))

DATA_TYPE_METADATA(MeshComponent::Submesh::BufferAccess, MEMBER_PROPERTY(buffer, "Handle", 0),
    MEMBER_PROPERTY(offset, "Offset", 0), MEMBER_PROPERTY(byteSize, "Size In Bytes", 0))

DATA_TYPE_METADATA(MeshComponent::Submesh::IndexBufferAccess, MEMBER_PROPERTY(buffer, "Handle", 0),
    MEMBER_PROPERTY(offset, "Offset", 0), MEMBER_PROPERTY(byteSize, "Size In Bytes", 0),
    MEMBER_PROPERTY(indexType, "Index Type", 0))

DATA_TYPE_METADATA(RENDER_NS::GraphicsState::InputAssembly,
    MEMBER_PROPERTY(enablePrimitiveRestart, "Enable Primitive Restart", 0),
    MEMBER_PROPERTY(primitiveTopology, "Primitive Topology", 0))

/* Expose more properties? */
DATA_TYPE_METADATA(MeshComponent::Submesh, MEMBER_PROPERTY(instanceCount, "Instance Count", 0),
    MEMBER_PROPERTY(bufferAccess, "", 0), MEMBER_PROPERTY(indexBuffer, "Index Buffer", 0),
    MEMBER_PROPERTY(indirectArgsBuffer, "Indirect Buffer", 0),
    MEMBER_PROPERTY(morphTargetBuffer, "Morph Target Buffer", 0), MEMBER_PROPERTY(vertexCount, "Vertex Count", 0),
    MEMBER_PROPERTY(indexCount, "Index Count", 0), MEMBER_PROPERTY(morphTargetCount, "Morph Target Count", 0),
    MEMBER_PROPERTY(aabbMin, "Min AABB", 0), MEMBER_PROPERTY(aabbMax, "Max AABB", 0),
    MEMBER_PROPERTY(material, "Material", 0), MEMBER_PROPERTY(additionalMaterials, "Additional Materials", 0),
    BITFIELD_MEMBER_PROPERTY(flags, "Options", PropertyFlags::IS_BITFIELD, MeshComponent::Submesh::FlagBits),
    MEMBER_PROPERTY(renderSortLayer, "Render Sort Layer", 0),
    MEMBER_PROPERTY(renderSortLayerOrder, "Render Sort Layer Order", 0),
    MEMBER_PROPERTY(inputAssembly, "Input Assembly (Optional)", 0))

CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;

class MeshComponentManager final : public BaseManager<MeshComponent, IMeshComponentManager> {
    BEGIN_PROPERTY(MeshComponent, componentMetaData_)
#include <3d/ecs/components/mesh_component.h>
    END_PROPERTY();

public:
    explicit MeshComponentManager(IEcs& ecs)
        : BaseManager<MeshComponent, IMeshComponentManager>(ecs, CORE_NS::GetName<MeshComponent>())
    {}

    ~MeshComponentManager() = default;

    size_t PropertyCount() const override
    {
        return BASE_NS::countof(componentMetaData_);
    }

    const Property* MetaData(size_t index) const override
    {
        if (index < BASE_NS::countof(componentMetaData_)) {
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
