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

#if !defined(API_3D_ECS_COMPONENTS_RENDER_CONFIGURATION_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_RENDER_CONFIGURATION_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/containers/string.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_component_manager.h>

CORE3D_BEGIN_NAMESPACE()
#endif

/** Render configuration component can be used to configure scene properties.
 */
BEGIN_COMPONENT(IRenderConfigurationComponentManager, RenderConfigurationComponent)
#if !defined(IMPLEMENT_MANAGER)
    enum class SceneShadowType : uint8_t {
        /* Percentage closer shadow filtering. Filtering done per pixel in screenspace. */
        PCF = 0,
        /* Variance shadow maps. Filtering done in separate blur passes in texture space. */
        VSM = 1,
    };

    enum class SceneShadowQuality : uint8_t {
        /* Low shadow quality. (Low resulution shadow map) */
        LOW = 0,
        /* Normal shadow quality. (Normal resolution shadow map) */
        NORMAL = 1,
        /* High shadow quality. (High resolution shadow map) */
        HIGH = 2,
        /* Ultra shadow quality. (Ultra resolution shadow map) */
        ULTRA = 3,
    };

    enum class SceneShadowSmoothness : uint8_t {
        /* Hard shadows. */
        HARD = 0,
        /* Normal (smooth) shadows. */
        NORMAL = 1,
        /* Extra soft and smooth shadows. */
        SOFT = 2,
    };

    enum SceneRenderingFlagBits : uint8_t {
        /* Create render node graphs automatically in RenderSystem. */
        CREATE_RNGS_BIT = (1 << 0),
    };
    /** Container for scene rendering flag bits */
    using SceneRenderingFlags = uint8_t;
#endif

    /** Entity containing an environment component that is used by default when rendering. Controls indirect and
     * environment lighting options.
     */
    DEFINE_PROPERTY(CORE_NS::Entity, environment, "Environment", 0, )

    /** Entity containing a fog component that is used by default when rendering.
     */
    DEFINE_PROPERTY(CORE_NS::Entity, fog, "Fog", 0, )

    /** Shadow type for the (ECS) scene.
     */
    DEFINE_PROPERTY(SceneShadowType, shadowType, "Shadow Type", 0, VALUE(SceneShadowType::PCF))

    /** Shadow quality for the (ECS) scene.
     */
    DEFINE_PROPERTY(SceneShadowQuality, shadowQuality, "Shadow Quality", 0, VALUE(SceneShadowQuality::NORMAL))

    /** Shadow smoothness for the (ECS) scene. (Might behave differently with shadow types)
     */
    DEFINE_PROPERTY(
        SceneShadowSmoothness, shadowSmoothness, "Shadow Smoothness", 0, VALUE(SceneShadowSmoothness::NORMAL))

    /** Scene rendering processing. Related to RNG processing and creation in RenderSystem.
     * If flags are 0, no processing is done automatically. SceneRenderingFlagBits::CREATE_RNGS_BIT must be enabled for
     * processing
     */
    DEFINE_BITFIELD_PROPERTY(SceneRenderingFlags, renderingFlags, "Rendering Flags", PropertyFlags::IS_BITFIELD,
        VALUE(CREATE_RNGS_BIT), RenderConfigurationComponent::SceneRenderingFlagBits)

    /** Custom scene render node graph file. (Can be patched with some scene specific ids)
     * Replaces the built-in scene render node graph when processing custom render node graphs with ECS (RenderSystem).
     */
    DEFINE_PROPERTY(BASE_NS::string, customRenderNodeGraphFile, "Custom Scene Render Node Graph File", 0, )

    /** Custom post scene render node graph file. (Can be patched with some scene specific ids)
     * Is added as a final render node graph from ECS (RenderSystem) when processing render node graphs.
     */
    DEFINE_PROPERTY(
        BASE_NS::string, customPostSceneRenderNodeGraphFile, "Custom Post Scene Render Node Graph File", 0, )

END_COMPONENT(
    IRenderConfigurationComponentManager, RenderConfigurationComponent, "7e655b3d-3cad-40b9-8179-c749be17f60b")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif
