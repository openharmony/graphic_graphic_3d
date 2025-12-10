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

#ifndef CORE__RENDER__RENDER_NODE_SCENE_UTIL_H
#define CORE__RENDER__RENDER_NODE_SCENE_UTIL_H

#include <cstdint>

#include <3d/render/intf_render_node_scene_util.h>
#include <3d/render/render_data_defines_3d.h>
#include <base/containers/array_view.h>
#include <base/containers/vector.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/pipeline_state_desc.h>
#include <render/render_data_structures.h>

RENDER_BEGIN_NAMESPACE()
class IRenderNodeUtil;
class IRenderNodeContextManager;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class IRenderDataStoreDefaultMaterial;
class IRenderDataStoreDefaultCamera;

/**
RenderNodeSceneUtil.
Helper class with scene util functions. The static functions are used internally and RenderNodeSceneUtilImpl implements
the public interface wrapper.
*/
class RenderNodeSceneUtil {
public:
    RenderNodeSceneUtil() = default;
    ~RenderNodeSceneUtil() = default;

    struct FrameGlobalDescriptorSets {
        RENDER_NS::RenderHandle set0;
        RENDER_NS::RenderHandle set1;
        RENDER_NS::RenderHandle set2Default;
        BASE_NS::array_view<const RENDER_NS::RenderHandle> set2;
        bool valid = false;
    };
    enum FrameGlobalDescriptorSetFlagBits : uint32_t {
        /** Set 0 */
        GLOBAL_SET_0 = (1 << 0),
        /** Set 1 */
        GLOBAL_SET_1 = (1 << 1),
        /** Set(s) 2 */
        GLOBAL_SET_2 = (1 << 2),
        /** All sets */
        GLOBAL_SET_ALL = 0xFFFFffff,
    };
    /** Container for frame global descriptor set flag bits */
    using FrameGlobalDescriptorSetFlags = uint32_t;

    static SceneRenderDataStores GetSceneRenderDataStores(
        const RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr,
        const BASE_NS::string_view sceneDataStoreName);
    static BASE_NS::string GetSceneRenderDataStore(const SceneRenderDataStores& sceneRds, BASE_NS::string_view dsName);

    static RENDER_NS::ViewportDesc CreateViewportFromCamera(const RenderCamera& camera);
    static RENDER_NS::ScissorDesc CreateScissorFromCamera(const RenderCamera& camera);
    static void UpdateRenderPassFromCamera(const RenderCamera& camera, RENDER_NS::RenderPass& renderPass);
    static void UpdateRenderPassFromCustomCamera(
        const RenderCamera& camera, const bool isNamedCamera, RENDER_NS::RenderPass& renderPass);
    static void GetRenderSlotSubmeshes(const IRenderDataStoreDefaultCamera& dataStoreCamera,
        const IRenderDataStoreDefaultMaterial& dataStoreMaterial, const uint32_t cameraIndex,
        const BASE_NS::array_view<const uint32_t> addCameraIndices,
        const IRenderNodeSceneUtil::RenderSlotInfo& renderSlotInfo,
        BASE_NS::vector<SlotSubmeshIndex>& refSubmeshIndices);

    static SceneBufferHandles GetSceneBufferHandles(
        RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr, const BASE_NS::string_view sceneName);
    static SceneCameraBufferHandles GetSceneCameraBufferHandles(
        RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr, const BASE_NS::string_view sceneName,
        const BASE_NS::string_view cameraName);
    static SceneCameraImageHandles GetSceneCameraImageHandles(
        RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr, const BASE_NS::string_view sceneName,
        const BASE_NS::string_view cameraName, const RenderCamera& camera);
    static SceneRenderCameraData GetSceneCameraData(const IRenderDataStoreDefaultScene& dataStoreScene,
        const IRenderDataStoreDefaultCamera& dataStoreCamera, const uint64_t cameraId,
        const BASE_NS::string_view cameraName);

    // get multiview camera indices, the mvIndices is reset in the function
    static void GetMultiViewCameraIndices(
        const IRenderDataStoreDefaultCamera& rds, const RenderCamera& cam, BASE_NS::vector<uint32_t>& mvIndices);
    // only valid for ExecuteFrame, call only there and do not store
    static FrameGlobalDescriptorSets GetFrameGlobalDescriptorSets(const RENDER_NS::IRenderNodeContextManager& rncm,
        const SceneRenderDataStores& stores, BASE_NS::string_view cameraName, FrameGlobalDescriptorSetFlags flags);
};

class RenderNodeSceneUtilImpl : public IRenderNodeSceneUtil {
public:
    RenderNodeSceneUtilImpl() = default;
    ~RenderNodeSceneUtilImpl() override = default;

    SceneRenderDataStores GetSceneRenderDataStores(const RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr,
        BASE_NS::string_view sceneDataStoreName) override;
    BASE_NS::string GetSceneRenderDataStore(
        const SceneRenderDataStores& sceneRds, BASE_NS::string_view dsName) override;
    RENDER_NS::ViewportDesc CreateViewportFromCamera(const RenderCamera& camera) override;
    RENDER_NS::ScissorDesc CreateScissorFromCamera(const RenderCamera& camera) override;
    void UpdateRenderPassFromCamera(const RenderCamera& camera, RENDER_NS::RenderPass& renderPass) override;
    void UpdateRenderPassFromCustomCamera(
        const RenderCamera& camera, bool isNamedCamera, RENDER_NS::RenderPass& renderPass) override;
    void GetRenderSlotSubmeshes(const IRenderDataStoreDefaultCamera& dataStoreCamera,
        const IRenderDataStoreDefaultMaterial& dataStoreMaterial, uint32_t cameraIndex,
        const RenderSlotInfo& renderSlotInfo, BASE_NS::vector<SlotSubmeshIndex>& refSubmeshIndices) override;
    void GetRenderSlotSubmeshes(const IRenderDataStoreDefaultCamera& dataStoreCamera,
        const IRenderDataStoreDefaultMaterial& dataStoreMaterial, const uint32_t cameraIndex,
        const BASE_NS::array_view<const uint32_t> addCameraIndices,
        const IRenderNodeSceneUtil::RenderSlotInfo& renderSlotInfo,
        BASE_NS::vector<SlotSubmeshIndex>& refSubmeshIndices) override;

    SceneBufferHandles GetSceneBufferHandles(
        RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr, const BASE_NS::string_view sceneName) override;
    SceneCameraBufferHandles GetSceneCameraBufferHandles(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr,
        const BASE_NS::string_view sceneName, const BASE_NS::string_view cameraName) override;
    SceneCameraImageHandles GetSceneCameraImageHandles(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr,
        const BASE_NS::string_view sceneName, const BASE_NS::string_view cameraName,
        const RenderCamera& camera) override;
    SceneRenderCameraData GetSceneCameraData(const IRenderDataStoreDefaultScene& dataStoreScene,
        const IRenderDataStoreDefaultCamera& dataStoreCamera, const uint64_t cameraId,
        const BASE_NS::string_view cameraName) override;

    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;

    void Ref() override;
    void Unref() override;
};
CORE3D_END_NAMESPACE()

#endif // CORE__RENDER__RENDER_NODE_SCENE_UTIL_H
