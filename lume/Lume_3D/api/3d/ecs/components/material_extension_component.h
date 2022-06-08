/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#if !defined(MATERIAL_EXTENSION_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define MATERIAL_EXTENSION_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/property_types.h>
#include <render/resource_handle.h>

CORE3D_BEGIN_NAMESPACE()
/** \addtogroup group_material_materialextensioncomponent
 *  @{
 */
#endif

/** Material extension component properties.
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
    DEFINE_ARRAY_PROPERTY(CORE_NS::EntityReference, ResourceIndex::RESOURCE_COUNT, resources, "", 0,)

END_COMPONENT(IMaterialExtensionComponentManager, MaterialExtensionComponent, "98416b5a-8780-4444-be4f-d0c972a09a2f")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif
