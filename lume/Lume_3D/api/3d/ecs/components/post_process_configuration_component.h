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

#if !defined(API_3D_ECS_COMPONENTS_POST_PROCESS_CONFIGURATION_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_POST_PROCESS_CONFIGURATION_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/containers/string.h>
#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/intf_property_handle.h>
#include <render/datastore/render_data_store_render_pods.h>

CORE3D_BEGIN_NAMESPACE()
#endif

/** Post-process component that can be used with cameras.
 * Uses the core configuration of PostProcessConfiguration, expect fog control not used/implemented.
 */
BEGIN_COMPONENT(IPostProcessConfigurationComponentManager, PostProcessConfigurationComponent)
#if !defined(IMPLEMENT_MANAGER)
    /** Custom post process effect */
    struct PostProcessEffect {
        /** Name of the post process (e.g. "custom_effect"). The name enables finding of the data. */
        BASE_NS::string name {};
        /** Optional: global user factor index if the post process factor is to be used in generic pipelines. */
        uint32_t globalUserFactorIndex { ~0u };
        /** Optional: custom post process shader from which the custom properties are tried to fetch. */
        CORE_NS::EntityReference shader {};

        /** Enabled. The built-in post processing render nodes support enabling these effects. */
        bool enabled { false };

        /** Additional customization flags. */
        uint32_t flags { 0u };

        /** Factor data. Used in global user factor and in local post process push data. */
        BASE_NS::Math::Vec4 factor { 0.0f, 0.0f, 0.0f, 0.0f };
        /** Custom effect property data. Used in local factors. Tries to fetch these from the shader. */
        CORE_NS::IPropertyHandle* customProperties { nullptr };
    };
#endif

    /** Post processes.
     */
    DEFINE_PROPERTY(BASE_NS::vector<PostProcessEffect>, postProcesses, "Post Processes", 0, )

    /** Custom post process render node graph file. (Can be patched with e.g. post process ids etc.)
     * Chosen automatically to camera post processes if the entity of the component is attached
     * to CameraComponent::postProcess
     */
    DEFINE_PROPERTY(BASE_NS::string, customRenderNodeGraphFile, "Custom Post Process RNG File", 0, )

END_COMPONENT(IPostProcessConfigurationComponentManager, PostProcessConfigurationComponent,
    "050af984-be02-49ae-b950-19b829252769")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif
