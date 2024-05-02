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

#if !defined(API_3D_ECS_COMPONENTS_RENDER_MESH_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_RENDER_MESH_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/math/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity.h>
#include <core/ecs/intf_component_manager.h>

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
    DEFINE_PROPERTY(CORE_NS::Entity, mesh, "Mesh", 0, )

    /** Render mesh batch entity. */
    DEFINE_PROPERTY(CORE_NS::Entity, renderMeshBatch, "Render Mesh Batch", 0, )

    /** Additional per render mesh custom data for shader customization */
    DEFINE_ARRAY_PROPERTY(BASE_NS::Math::UVec4, RENDER_MESH_CUSTOM_DATA_VEC4_COUNT, customData,
        "Additional Shader Data", 0, ARRAY_VALUE({ 0u, 0u, 0u, 0u }))
END_COMPONENT(IRenderMeshComponentManager, RenderMeshComponent, "311f8817-1878-4363-a6e6-68cc2299ea8d")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif
