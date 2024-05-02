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

#if !defined(API_3D_ECS_COMPONENTS_RENDER_MESH_BATCH_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_RENDER_MESH_BATCH_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>

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
