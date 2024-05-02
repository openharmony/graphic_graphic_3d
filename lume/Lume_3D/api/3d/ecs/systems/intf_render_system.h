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

#ifndef API_3D_ECS_SYSTEMS_IRENDER_SYSTEM_H
#define API_3D_ECS_SYSTEMS_IRENDER_SYSTEM_H

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <core/ecs/intf_system.h>
#include <core/namespace.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
class IRenderDataStoreManager;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
/** @ingroup group_ecs_systems_irender */
/** IRender system */
class IRenderSystem : public CORE_NS::ISystem {
public:
    static constexpr BASE_NS::Uid UID { "33ebc109-0687-48b4-8002-f129c1d800fc" };
    /** Rendering related data stores to feed data to renderer.
     */
    struct Properties {
        // NOTE: These all should be DEPRECATED

        /** Data store for scene */
        BASE_NS::string dataStoreScene;
        /** Data store for camera */
        BASE_NS::string dataStoreCamera;
        /** Data store for light */
        BASE_NS::string dataStoreLight;
        /** Data store for material */
        BASE_NS::string dataStoreMaterial;
        /** Data store for morphing (passed to rendering) */
        BASE_NS::string dataStoreMorph;

        /** Data store prefix for other data stores in e.g. different plugins */
        BASE_NS::string dataStorePrefix;
    };

    /** Get render node graphs for this ECS render system.
     * There are render node graphs for scene and active cameras.
     * Flags needs to be set-up properly to process / create the render node graphs.
     * @return Array view to all render node graphs.
     */
    virtual BASE_NS::array_view<const RENDER_NS::RenderHandleReference> GetRenderNodeGraphs() const = 0;

protected:
    IRenderSystem() = default;
    ~IRenderSystem() override = default;
    IRenderSystem(const IRenderSystem&) = delete;
    IRenderSystem(IRenderSystem&&) = delete;
    IRenderSystem& operator=(const IRenderSystem&) = delete;
    IRenderSystem& operator=(IRenderSystem&&) = delete;
};

/** @ingroup group_ecs_systems_irender */
/** Return name of this system
 */
inline constexpr BASE_NS::string_view GetName(const IRenderSystem*)
{
    return "RenderSystem";
}
CORE3D_END_NAMESPACE()

#endif // API_3D_ECS_SYSTEMS_IRENDER_SYSTEM_H
