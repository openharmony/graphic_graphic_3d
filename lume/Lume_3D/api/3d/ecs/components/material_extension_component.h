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

#if !defined(API_3D_ECS_COMPONENTS_MATERIAL_EXTENSION_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_MATERIAL_EXTENSION_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_component_manager.h>

CORE3D_BEGIN_NAMESPACE()
/** \addtogroup group_material_materialextensioncomponent
 *  @{
 */
#endif

/** Material extension component properties.
 * NOTE: "DEPRECATED" prefer using MaterialComponent::customResources and EnvironmentComponent::customResources
 * Can extend default material model with additional resources for custom shaders.
 * GPU resources are owned by the component and destroyed when the component is destroyed.
 * Can be used to default material render slots and default material env node.
 */
BEGIN_COMPONENT(IMaterialExtensionComponentManager, MaterialExtensionComponent)
#if !defined(IMPLEMENT_MANAGER)
    /** Custom resources which will be bound to pre-defined set and their bindings in order */
    enum ResourceIndex : uint8_t {
        /** Custom resource which will be bound to pre-defined set and binding 0 */
        RESOURCE_INDEX_0,
        /** Custom resource which will be bound to pre-defined set and binding 1 */
        RESOURCE_INDEX_1,
        /** Custom resource which will be bound to pre-defined set and binding 2 */
        RESOURCE_INDEX_2,
        /** Custom resource which will be bound to pre-defined set and binding 3 */
        RESOURCE_INDEX_3,
        /** Number of resources */
        RESOURCE_COUNT,
    };
#endif

    /** Array of resources. With default material model these are bound in order to predefined set.
     * One needs to use custom shader with custom pipeline layout (predefined set) to access these in shader.
     * Resources are owned by the component and destroyed when it's destroyed.
     */
    DEFINE_ARRAY_PROPERTY(
        CORE_NS::EntityReference, ResourceIndex::RESOURCE_COUNT, resources, "Custom Material Resources", 0, )

END_COMPONENT(IMaterialExtensionComponentManager, MaterialExtensionComponent, "98416b5a-8780-4444-be4f-d0c972a09a2f")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif
