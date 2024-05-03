/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_RENDER_3D_LUME_CUSTOM_RENDER_H
#define OHOS_RENDER_3D_LUME_CUSTOM_RENDER_H

#include <algorithm>
#include <vector>
#include <string>
#include <securec.h>

#include <3d/intf_graphics_context.h>

#include <base/math/vector.h>

#include <core/intf_engine.h>
#include <core/log.h>
#include <core/namespace.h>

#include <render/datastore/intf_render_data_store_default_staging.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>

#include "3d_widget_adapter_log.h"
#include "custom/shader_input_buffer.h"

namespace OHOS::Render3D {

struct CustomRenderInput {
    CORE_NS::IEngine::Ptr engine_;
    CORE3D_NS::IGraphicsContext::Ptr graphicsContext_;
    RENDER_NS::IRenderContext::Ptr renderContext_;
    CORE_NS::IEcs::Ptr ecs_;
    uint32_t width_ { 0U };
    uint32_t height_ { 0U };
};

class LumeCustomRender {
public:
    LumeCustomRender(bool needsFrameCallback): needsFrameCallback_(needsFrameCallback) {};
    virtual ~LumeCustomRender();

    virtual BASE_NS::vector<RENDER_NS::RenderHandleReference> GetRenderHandles();
    void Initialize(const CustomRenderInput& input);
    void LoadImages(const std::vector<std::string>& imageUris);
    void RegistorShaderPath(const std::string& shaderPath);
    virtual bool UpdateShaderInputBuffer(const std::shared_ptr<ShaderInputBuffer>& shaderInputBuffer);
    void UpdateShaderSpecialization(const std::vector<uint32_t>& values);
    virtual void OnSizeChange(int32_t width, int32_t height);
    virtual void OnDrawFrame();
    void LoadRenderNodeGraph(const std::string& rngUri, const RENDER_NS::RenderHandleReference& output);
    void UnloadImages();
    void UnloadRenderNodeGraph();
    bool NeedsFrameCallback()
    {
        return needsFrameCallback_;
    }
    void SetScaleInfo(float widthScale, float heightScale)
    {
        widthScale_ = widthScale;
        heightScale_ = heightScale;
    }
protected:
    float widthScale_ = 1.0f;
    float heightScale_ = 1.0f;
    uint32_t width_ = 0U;
    uint32_t height_ = 0U;

private:
    const RENDER_NS::RenderHandleReference GetRenderHandle();
    void SetRenderOutput(const RENDER_NS::RenderHandleReference& output);
    void LoadImage(const std::string& imageUri);
    void GetDefaultStaging();
    void PrepareResolutionInputBuffer();
    void DestroyBuffer();
    void DestroyDataStorePod();
    void DestroyRes();

    CORE_NS::IEngine::Ptr engine_;
    CORE3D_NS::IGraphicsContext::Ptr graphicsContext_;
    RENDER_NS::IRenderContext::Ptr renderContext_;
    RENDER_NS::IRenderDataStoreDefaultStaging* renderDataStoreDefaultStaging_ { nullptr };

    RENDER_NS::RenderHandleReference shaderInputBufferHandle_;
    RENDER_NS::RenderHandleReference resolutionBufferHandle_;
    RENDER_NS::RenderHandleReference renderHandle_;
    RENDER_NS::GpuBufferDesc bufferDesc_ {
        RENDER_NS::CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT | RENDER_NS::CORE_BUFFER_USAGE_TRANSFER_DST_BIT,
        RENDER_NS::CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RENDER_NS::CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        RENDER_NS::CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, 0u };

    CORE_NS::EntityReference envCubeHandle_;
    CORE_NS::EntityReference bgHandle_;
    CORE_NS::IEcs::Ptr ecs_;

    bool needsFrameCallback_ = false;
    std::vector<std::pair<std::string, CORE_NS::EntityReference>> images_;
    std::shared_ptr<ShaderInputBuffer> shaderInputBuffer_;
    ShaderInputBuffer resolutionBuffer_;

    const char* const RENDER_DATA_STORE_DEFAULT_STAGING = "RenderDataStoreDefaultStaging";
    const char* const RENDER_DATA_STORE_POD = "RenderDataStorePod";
    const char* const SPECIALIZATION_TYPE_NAME = "ShaderSpecializationRenderPod";
    const char* const SPECIALIZATION_CONFIG_NAME = "ShaderSpecializationConfig";
    const char* const IMAGE_NAME = "IMAGE";
    const char* const INPUT_BUFFER = "INPUT_BUFFER";
    const char* const RESOLUTION_BUFFER = "RESOLUTION_BUFFER";
};

} // namespace name

#endif //OHOS_RENDER_3D_LUME_CUSTOM_RENDER_H
