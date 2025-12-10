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

#if !defined(API_3D_ECS_COMPONENTS_POST_PROCESS_EFFECT_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_POST_PROCESS_EFFECT_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/containers/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <render/nodecontext/intf_render_post_process.h>

CORE3D_BEGIN_NAMESPACE()
#endif

/** Post-process effect component that can be used with cameras.
 * The given RENDER_NS::IRenderPostProcess effects are executed in the given order.
 */
BEGIN_COMPONENT(IPostProcessEffectComponentManager, PostProcessEffectComponent)
#if !defined(IMPLEMENT_MANAGER)
#endif

    /** Effects.
     */
    DEFINE_PROPERTY(BASE_NS::vector<RENDER_NS::IRenderPostProcess::Ptr>, effects, "Effects", 0, ARRAY_VALUE())

END_COMPONENT(IPostProcessEffectComponentManager, PostProcessEffectComponent, "c4879f56-bd12-48a5-aae5-353c39916fdb")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif
