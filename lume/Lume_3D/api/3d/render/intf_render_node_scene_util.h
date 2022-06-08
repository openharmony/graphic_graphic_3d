/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_3D_RENDER_IRENDER_NODE_SCENE_UTIL_H
#define API_3D_RENDER_IRENDER_NODE_SCENE_UTIL_H

#include <cstdint>

#include <3d/render/render_data_defines_3d.h>
#include <base/containers/vector.h>
#include <render/device/pipeline_state_desc.h>
#include <render/render_data_structures.h>

RENDER_BEGIN_NAMESPACE()
class IRenderNodeUtil;
class IRenderNodeContextManager;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class IRenderDataStoreDefaultMaterial;
class IRenderDataStoreDefaultCamera;

struct SceneRenderDataStores {
    // render data store scene name can be used as prefix for 3D scene resource names
    BASE_NS::fixed_string<RENDER_NS::RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> dataStoreNameScene;
    BASE_NS::fixed_string<RENDER_NS::RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> dataStoreNameMaterial;
    BASE_NS::fixed_string<RENDER_NS::RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> dataStoreNameCamera;
    BASE_NS::fixed_string<RENDER_NS::RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> dataStoreNameLight;
    BASE_NS::fixed_string<RENDER_NS::RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> dataStoreNameMorph;
};

/**
IRenderNodeSceneUtil.
Helper class with scene util functions.
*/
class IRenderNodeSceneUtil : public CORE_NS::IInterface {
public:
    static constexpr BASE_NS::Uid UID { "78b61c97-6ccc-4f1d-aa27-af3cb63cff6d" };

    /** Render slot info for render mesh fetching.
     */
    struct RenderSlotInfo {
        /** Render slot id */
        uint32_t id { ~0u };
        /** Sort type */
        RENDER_NS::RenderSlotSortType sortType { RENDER_NS::RenderSlotSortType::NONE };
        /** Cull type */
        RENDER_NS::RenderSlotCullType cullType { RENDER_NS::RenderSlotCullType::NONE };
        /** One can discard render meshes by material flags
         * (e.g. RENDER_MATERIAL_COMPLEX_BIT or RENDER_MATERIAL_BASIC_BIT)  */
        RenderMaterialFlags materialDiscardFlags { 0u };
    };

    /** Get render data store names mathing the given scene render data store.
     * @param renderNodeContextMgr The node's context manager.
     * @param sceneDataStoreName Name of the scene render data store or empty if default data store is used.
     * @return Collection of data store names.
     */
    virtual SceneRenderDataStores GetSceneRenderDataStores(
        const RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr, BASE_NS::string_view sceneDataStoreName) = 0;

    /** Fills viewport description matching the given camera.
     * @param camera Render camera whos resolution and viewport are used.
     * @return Viewport description.
     */
    virtual RENDER_NS::ViewportDesc CreateViewportFromCamera(const RenderCamera& camera) = 0;

    /** Fills scissor description matching the given camera.
     * @param camera Render camera whos resolution and scissor are used.
     * @return Scissor description.
     */
    virtual RENDER_NS::ScissorDesc CreateScissorFromCamera(const RenderCamera& camera) = 0;

    /** Updates the render pass using the given camera.
     * NOTE: per frame camera update (does not fetch GpuImageDesc for render area evaluation)
     * @param camera Render camera whos resolution, targets, etc. are used.
     * @param renderPass Render pass to update.
     */
    virtual void UpdateRenderPassFromCamera(const RenderCamera& camera, RENDER_NS::RenderPass& renderPass) = 0;

    /** Updates the render pass using the given camera.
     * NOTE: uses RenderCamera based clears (ignores render node graph loaded renderpass clear setup)
     * Legacy support for named camera with a flag.
     * @param camera Render camera whos resolution, targets, etc. are used.
     * @param isNamedCamera Legacy support is only taken into account when this is true.
     * @param renderPass Render pass to update.
     */
    virtual void UpdateRenderPassFromCustomCamera(
        const RenderCamera& camera, bool isNamedCamera, RENDER_NS::RenderPass& renderPass) = 0;

    /** Clears the given slot submesh indices and resizes it based on the slot materials.
     * @param dataStoreCamera Current data store for cameras.
     * @param dataStoreMaterial Current data store for materials.
     * @param cameraId ID of the camera in dataStoreCamera.
     * @param renderSlotInfo for render slot related submesh processing.
     * @param refSubmeshIndices Render slot submesh indices to update.
     */
    virtual void GetRenderSlotSubmeshes(const IRenderDataStoreDefaultCamera& dataStoreCamera,
        const IRenderDataStoreDefaultMaterial& dataStoreMaterial, uint32_t cameraId,
        const RenderSlotInfo& renderSlotInfo, BASE_NS::vector<SlotSubmeshIndex>& refSubmeshIndices) = 0;

protected:
    IRenderNodeSceneUtil() = default;
    virtual ~IRenderNodeSceneUtil() = default;
};

inline constexpr BASE_NS::string_view GetName(const IRenderNodeSceneUtil*)
{
    return "IRenderNodeSceneUtil";
}
CORE3D_END_NAMESPACE()

#endif // API_3D_RENDER_IRENDER_NODE_SCENE_UTIL_H
