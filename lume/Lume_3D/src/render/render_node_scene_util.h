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

#ifndef CORE__RENDER__RENDER_NODE_SCENE_UTIL_H
#define CORE__RENDER__RENDER_NODE_SCENE_UTIL_H

#include <cstdint>

#include <3d/render/intf_render_node_scene_util.h>
#include <3d/render/render_data_defines_3d.h>
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

    static SceneRenderDataStores GetSceneRenderDataStores(
        const RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr,
        const BASE_NS::string_view sceneDataStoreName);

    static RENDER_NS::ViewportDesc CreateViewportFromCamera(const RenderCamera& camera);
    static RENDER_NS::ScissorDesc CreateScissorFromCamera(const RenderCamera& camera);
    static void UpdateRenderPassFromCamera(const RenderCamera& camera, RENDER_NS::RenderPass& renderPass);
    static void UpdateRenderPassFromCustomCamera(
        const RenderCamera& camera, const bool isNamedCamera, RENDER_NS::RenderPass& renderPass);
    static void GetRenderSlotSubmeshes(const IRenderDataStoreDefaultCamera& dataStoreCamera,
        const IRenderDataStoreDefaultMaterial& dataStoreMaterial, const uint32_t cameraId,
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
};

class RenderNodeSceneUtilImpl : public IRenderNodeSceneUtil {
public:
    RenderNodeSceneUtilImpl() = default;
    ~RenderNodeSceneUtilImpl() override = default;

    SceneRenderDataStores GetSceneRenderDataStores(const RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr,
        BASE_NS::string_view sceneDataStoreName) override;
    RENDER_NS::ViewportDesc CreateViewportFromCamera(const RenderCamera& camera) override;
    RENDER_NS::ScissorDesc CreateScissorFromCamera(const RenderCamera& camera) override;
    void UpdateRenderPassFromCamera(const RenderCamera& camera, RENDER_NS::RenderPass& renderPass) override;
    void UpdateRenderPassFromCustomCamera(
        const RenderCamera& camera, bool isNamedCamera, RENDER_NS::RenderPass& renderPass) override;
    void GetRenderSlotSubmeshes(const IRenderDataStoreDefaultCamera& dataStoreCamera,
        const IRenderDataStoreDefaultMaterial& dataStoreMaterial, uint32_t cameraId,
        const RenderSlotInfo& renderSlotInfo, BASE_NS::vector<SlotSubmeshIndex>& refSubmeshIndices) override;

    SceneBufferHandles GetSceneBufferHandles(
        RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr, const BASE_NS::string_view sceneName) override;
    SceneCameraBufferHandles GetSceneCameraBufferHandles(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr,
        const BASE_NS::string_view sceneName, const BASE_NS::string_view cameraName) override;
    SceneCameraImageHandles GetSceneCameraImageHandles(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr,
        const BASE_NS::string_view sceneName, const BASE_NS::string_view cameraName,
        const RenderCamera& camera) override;

    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;

    void Ref() override;
    void Unref() override;
};
CORE3D_END_NAMESPACE()

#endif // CORE__RENDER__RENDER_NODE_SCENE_UTIL_H
