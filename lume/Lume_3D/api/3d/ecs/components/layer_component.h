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

#if !defined(API_3D_ECS_COMPONENTS_LAYER_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_LAYER_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/ecs/components/layer_defines.h>
#include <3d/namespace.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/namespace.h>

CORE3D_BEGIN_NAMESPACE()
#endif
/** Layer component can be used to limit the visibility of the entity in cameras and reflections.
 */
BEGIN_COMPONENT(ILayerComponentManager, LayerComponent)

    /** Defines a layer mask. */
    DEFINE_BITFIELD_PROPERTY(uint64_t, layerMask, "Layer Mask", CORE_NS::PropertyFlags::IS_BITFIELD,
        VALUE(LayerConstants::DEFAULT_LAYER_MASK), LayerFlagBits)

END_COMPONENT(ILayerComponentManager, LayerComponent, "01507270-f290-4f71-82f4-58f1bddeadc5")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif
