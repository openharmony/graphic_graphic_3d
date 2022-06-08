/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#if !defined(RENDER_MESH_BATCH_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define RENDER_MESH_BATCH_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/property_types.h>

CORE3D_BEGIN_NAMESPACE()
#endif
BEGIN_COMPONENT(IRenderMeshBatchComponentManager, RenderMeshBatchComponent)
#if !defined(IMPLEMENT_MANAGER)
    enum class BatchType : uint8_t {
        /* GPU instancing */
        GPU_INSTANCING = 0,
    };
#endif
    DEFINE_PROPERTY(BatchType, batchType, "Batch Type", 0, VALUE(BatchType::GPU_INSTANCING))
END_COMPONENT(IRenderMeshBatchComponentManager, RenderMeshBatchComponent, "f72d1dcf-c68c-4b61-9582-601c3fdbccf8")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif
