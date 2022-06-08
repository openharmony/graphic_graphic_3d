/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#if !defined(RSDZ_MODEL_ID_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define RSDZ_MODEL_ID_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/property_types.h>

CORE3D_BEGIN_NAMESPACE()
#endif // IMPLEMENT_MANAGER

/** RSDZ model id is used to store extra id from glTF2.
 */
BEGIN_COMPONENT(IRSDZModelIdComponentManager, RSDZModelIdComponent)
    /** Model id value in string format
     */
    DEFINE_ARRAY_PROPERTY(char, 64u, modelId, "Model ID", 0, "")

END_COMPONENT(IRSDZModelIdComponentManager, RSDZModelIdComponent, "a4b1877f-0feb-4d24-88a8-3d08eb5f535e")

#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif // IMPLEMENT_MANAGER

#endif // __RSDZ_MODEL_ID_COMPONENT__
