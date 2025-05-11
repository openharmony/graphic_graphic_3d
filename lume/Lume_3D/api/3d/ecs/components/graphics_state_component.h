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

#if !defined(API_3D_ECS_COMPONENTS_GRAPHICS_STATE_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_GRAPHICS_STATE_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/containers/string.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/scoped_handle.h>
#include <render/device/pipeline_state_desc.h>

CORE3D_BEGIN_NAMESPACE()
#endif
/**
 * Graphics state component simplifies changes to the graphics states. Entity owning the component can be assigned as a
 * MaterialComponent's graphics state.
 */
BEGIN_COMPONENT(IGraphicsStateComponentManager, GraphicsStateComponent)

    /** Graphics state.
     */
    DEFINE_PROPERTY(RENDER_NS::GraphicsState, graphicsState, "Graphics State", 0, VALUE())

    /** Render slot in which the state will be used. If empty default material render slot will be used.
     */
    DEFINE_PROPERTY(BASE_NS::string, renderSlot, "Render Slot", 0, VALUE())

END_COMPONENT(IGraphicsStateComponentManager, GraphicsStateComponent, "6c1c672f-162b-4ae1-95d5-2be474ef835c")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif // API_3D_ECS_COMPONENTS_GRAPHICS_STATE_COMPONENT_H
