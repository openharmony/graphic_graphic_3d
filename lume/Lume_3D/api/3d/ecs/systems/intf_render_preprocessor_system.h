/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_3D_ECS_SYSTEMS_IRENDER_PREPROCESSOR_SYSTEM_H
#define API_3D_ECS_SYSTEMS_IRENDER_PREPROCESSOR_SYSTEM_H

#include <3d/namespace.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <core/ecs/intf_system.h>
#include <core/namespace.h>
#include <render/datastore/intf_render_data_store_manager.h>

RENDER_BEGIN_NAMESPACE()
class IRenderDataStoreManager;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
/** @ingroup group_ecs_systems_irender */
/** IRender pre-processor system */
class IRenderPreprocessorSystem : public CORE_NS::ISystem {
public:
    static constexpr BASE_NS::Uid UID { "f0dac6cf-3828-44c2-8eec-60b04f06b7b2" };
    /** Rendering related data stores to feed data to renderer.
     */
    struct Properties {
        /** Data store manager */
        RENDER_NS::IRenderDataStoreManager* dataStoreManager { nullptr };
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
    };

protected:
    IRenderPreprocessorSystem() = default;
    ~IRenderPreprocessorSystem() override = default;
    IRenderPreprocessorSystem(const IRenderPreprocessorSystem&) = delete;
    IRenderPreprocessorSystem(IRenderPreprocessorSystem&&) = delete;
    IRenderPreprocessorSystem& operator=(const IRenderPreprocessorSystem&) = delete;
    IRenderPreprocessorSystem& operator=(IRenderPreprocessorSystem&&) = delete;
};

/** @ingroup group_ecs_systems_irender */
/** Return name of this system
 */
inline constexpr BASE_NS::string_view GetName(const IRenderPreprocessorSystem*)
{
    return "RenderPreprocessorSystem";
}
CORE3D_END_NAMESPACE()

#endif // API_3D_ECS_SYSTEMS_IRENDER_PREPROCESSOR_SYSTEM_H
