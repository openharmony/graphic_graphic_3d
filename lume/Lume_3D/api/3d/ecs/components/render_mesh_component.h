/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#if !defined(RENDER_MESH_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define RENDER_MESH_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/math/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/property_types.h>

CORE3D_BEGIN_NAMESPACE()
#endif
/** Render Mesh component can be used to associate a mesh with an object, and is used by the rendering system.
 */
BEGIN_COMPONENT(IRenderMeshComponentManager, RenderMeshComponent)
#if !defined(IMPLEMENT_MANAGER)
    /** Additional custom data size which is bind with render mesh structure to shader
     * Should match api/shaders/common/3d_dm_structures_common.h DefaultMaterialSingleMeshStruct customData
     */
    static constexpr uint32_t RENDER_MESH_CUSTOM_DATA_VEC4_COUNT { 2u };
#endif
    /** Mesh entity. */
    DEFINE_PROPERTY(CORE_NS::Entity, mesh, "Mesh", 0,)

    /** Render mesh batch entity. */
    DEFINE_PROPERTY(CORE_NS::Entity, renderMeshBatch, "Render mesh batch", 0,)

    /** Additional per render mesh custom data for shader customization */
    DEFINE_ARRAY_PROPERTY(BASE_NS::Math::UVec4, RENDER_MESH_CUSTOM_DATA_VEC4_COUNT, customData,
        "Additional shader custom data", 0, ARRAY_VALUE({ 0u, 0u, 0u, 0u }))
END_COMPONENT(IRenderMeshComponentManager, RenderMeshComponent, "311f8817-1878-4363-a6e6-68cc2299ea8d")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif
