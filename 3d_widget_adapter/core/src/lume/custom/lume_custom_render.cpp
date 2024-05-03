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

#include "custom/lume_custom_render.h"

#include <securec.h>

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/light_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/util/intf_scene_util.h>

#include <base/containers/string_view.h>
#include <base/math/matrix_util.h>
#include <base/math/quaternion.h>
#include <base/math/quaternion_util.h>
#include <base/math/vector.h>
#include <base/math/vector_util.h>

#include <core/ecs/intf_system_graph_loader.h>
#include <core/image/intf_image_loader_manager.h>
#include <core/io/intf_file_manager.h>

#include <render/device/intf_shader_manager.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>

#include "3d_widget_adapter_log.h"

namespace OHOS::Render3D {
void LumeCustomRender::Initialize(const CustomRenderInput& input)
{
    ecs_ = input.ecs_;
    graphicsContext_ = input.graphicsContext_;
    engine_ = input.engine_;
    renderContext_ = input.renderContext_;

    if (!ecs_ || !graphicsContext_ || !renderContext_ || !engine_) {
        WIDGET_LOGD("invalid input ecs %d, graphic context%d, render context %d, engine %d",
            ecs_ != nullptr, graphicsContext_ != nullptr, renderContext_ != nullptr, engine_ != nullptr);
        return;
    }

    PrepareResolutionInputBuffer();
    GetDefaultStaging();
    OnSizeChange(input.width_, input.height_);
}

void LumeCustomRender::RegistorShaderPath(const std::string& shaderPath)
{
    WIDGET_LOGD("lume custom render registor shader path");
    engine_->GetFileManager().RegisterPath("shaders", shaderPath.c_str(), false);
    static constexpr const RENDER_NS::IShaderManager::ShaderFilePathDesc desc { "shaders://" };
    renderContext_->GetDevice().GetShaderManager().LoadShaderFiles(desc);
}

void LumeCustomRender::SetRenderOutput(const RENDER_NS::RenderHandleReference& output)
{
    if (output) {
        RENDER_NS::IRenderNodeGraphManager& graphManager = renderContext_->GetRenderNodeGraphManager();
        graphManager.SetRenderNodeGraphResources(GetRenderHandle(), {}, { &output, 1u });
    }
}

void LumeCustomRender::GetDefaultStaging()
{
    renderDataStoreDefaultStaging_ = reinterpret_cast<RENDER_NS::IRenderDataStoreDefaultStaging*>(
        renderContext_->GetRenderDataStoreManager().GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_STAGING));
    if (renderDataStoreDefaultStaging_ == nullptr) {
        WIDGET_LOGE("Get default staging error");
    }
}

void LumeCustomRender::PrepareResolutionInputBuffer()
{
    const uint32_t resolutionSize = 2U;
    if (!resolutionBuffer_.Alloc(resolutionSize)) {
        WIDGET_LOGE("alloc resolution input buffer error!");
    }
    resolutionBuffer_.Update(0.0, 0U);
    resolutionBuffer_.Update(0.0, 1U);
}

void LumeCustomRender::DestroyBuffer()
{
    shaderInputBufferHandle_ = {};
    resolutionBuffer_ = {};
}

void LumeCustomRender::UpdateShaderSpecialization(const std::vector<uint32_t>& values)
{
    RENDER_NS::IRenderDataStorePod* dataStore = static_cast<RENDER_NS::IRenderDataStorePod*>(
        renderContext_->GetRenderDataStoreManager().GetRenderDataStore(RENDER_DATA_STORE_POD));
    if (dataStore) {
        RENDER_NS::ShaderSpecializationRenderPod shaderSpecialization;
        auto count = std::min(static_cast<uint32_t>(values.size()),
            RENDER_NS::ShaderSpecializationRenderPod::MAX_SPECIALIZATION_CONSTANT_COUNT);
        shaderSpecialization.specializationConstantCount = count;

        for (auto i = 0U; i < count; i++) {
            shaderSpecialization.specializationFlags[i].value = values[i];
        }
        dataStore->Set(SPECIALIZATION_CONFIG_NAME, BASE_NS::arrayviewU8(shaderSpecialization));
    }
}

void LumeCustomRender::DestroyDataStorePod()
{
}

void  LumeCustomRender::LoadImages(const std::vector<std::string>& imageUris)
{
    for (auto& imageUri : imageUris) {
        LoadImage(imageUri);
    }
    const std::string& turboTexture = "OhosRawFile://assets/blue_ball_compressed.png";
    if (imageUris.back().find("ball_compressed") != std::string::npos) {
        LoadImage(turboTexture);
    }
}

void LumeCustomRender::LoadImage(const std::string& imageUri)
{
    auto& imageManager = engine_->GetImageLoaderManager();
    auto& gpuResourceMgr = renderContext_->GetDevice().GetGpuResourceManager();
    auto handleManager = CORE_NS::GetManager<CORE3D_NS::IRenderHandleComponentManager>(*ecs_);

    auto result = imageManager.LoadImage(imageUri.c_str(), 0);
    if (!result.success) {
        WIDGET_LOGE("3D image update fail %s error", imageUri.c_str());
        return;
    }

    RENDER_NS::GpuImageDesc gpuImageDesc = gpuResourceMgr.CreateGpuImageDesc(result.image->GetImageDesc());
    std::string name = std::string(IMAGE_NAME) + "_" + std::to_string(images_.size());
    auto handle = gpuResourceMgr.Create(name.c_str(), gpuImageDesc, std::move(result.image));

    auto entity = ecs_->GetEntityManager().CreateReferenceCounted();
    handleManager->Create(entity);
    handleManager->Write(entity)->reference = BASE_NS::move(handle);
    images_.push_back(std::make_pair(imageUri, entity));
}

void LumeCustomRender::UnloadImages()
{
    images_.clear();
}

void LumeCustomRender::DestroyRes()
{
    DestroyBuffer();
    DestroyDataStorePod();
    UnloadImages();
    UnloadRenderNodeGraph();
}

LumeCustomRender::~LumeCustomRender()
{
    DestroyRes();
}

void LumeCustomRender::OnSizeChange(int32_t width, int32_t height)
{
    uint32_t floatSize = 2u;
    if (width <= 0 || height <= 0) {
        WIDGET_LOGE("width and height must be larger than zero");
        return;
    }
    width_ = width;
    height_ = height;
    const float* buffer = resolutionBuffer_.Map(floatSize);
    if (!buffer) {
        WIDGET_LOGE("custom render resolution resolutionBuffer error!");
        return;
    }

    auto bSize = resolutionBuffer_.ByteSize();
    if (!resolutionBufferHandle_) {
        RENDER_NS::GpuBufferDesc bufferDesc {
            RENDER_NS::CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT | RENDER_NS::CORE_BUFFER_USAGE_TRANSFER_DST_BIT,
            RENDER_NS::CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RENDER_NS::CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            RENDER_NS::CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, 0u };
        bufferDesc.byteSize = bSize;
        resolutionBufferHandle_ =
            renderContext_->GetDevice().GetGpuResourceManager().Create(RESOLUTION_BUFFER, bufferDesc);
        WIDGET_LOGD("create resolution buffer handle");
    }

    auto fWidth = static_cast<float>(width);
    auto fHeight = static_cast<float>(height);
    if ((buffer[0] == fWidth) && (buffer[1] == fHeight)) {
        return;
    }

    WIDGET_LOGD("update custom shader resolution %f X %f", fWidth, fHeight);
    resolutionBuffer_.Update(fWidth, 0U);
    resolutionBuffer_.Update(fHeight, 1U);
    BASE_NS::array_view<const uint8_t> data(reinterpret_cast<const uint8_t*>(buffer), bSize);
    const RENDER_NS::BufferCopy bufferCopy { 0, 0, bSize };
    renderDataStoreDefaultStaging_->CopyDataToBufferOnCpu(data, resolutionBufferHandle_, bufferCopy);
}

BASE_NS::vector<RENDER_NS::RenderHandleReference> LumeCustomRender::GetRenderHandles()
{
    return { renderHandle_ };
}

const RENDER_NS::RenderHandleReference LumeCustomRender::GetRenderHandle()
{
    return renderHandle_;
}

void LumeCustomRender::LoadRenderNodeGraph(const std::string& rngUri,
    const RENDER_NS::RenderHandleReference& output)
{
    RENDER_NS::IRenderNodeGraphManager& graphManager = renderContext_->GetRenderNodeGraphManager();
    auto* loader = &graphManager.GetRenderNodeGraphLoader();
    auto graphUri = BASE_NS::string_view(rngUri.c_str());

    auto const result = loader->Load(graphUri);
    if (!result.error.empty()) {
        WIDGET_LOGE("3D render node graph load fail: %s, uri %s", result.error.c_str(),
            rngUri.c_str());
        return;
    }

    renderHandle_ = graphManager.Create(
        RENDER_NS::IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC, result.desc);
}

void LumeCustomRender::UnloadRenderNodeGraph()
{
    renderHandle_ = {};
}

bool LumeCustomRender::UpdateShaderInputBuffer(const std::shared_ptr<ShaderInputBuffer>& shaderInputBuffer)
{
    if (!shaderInputBuffer || !shaderInputBuffer->IsValid()) {
        WIDGET_LOGE("3D shader input buffer update fail: invalid shaderInputBuffer");
        return false;
    }

    if (renderContext_ == nullptr) {
        WIDGET_LOGE("3D shader input buffer update fail: Call UpdateBuffer before Initiliaze error");
        return false;
    }

    auto fSize = shaderInputBuffer->FloatSize();
    auto bSize = shaderInputBuffer->ByteSize();
    if (bufferDesc_.byteSize != bSize) {
        bufferDesc_.byteSize = bSize;
        shaderInputBufferHandle_ = renderContext_->GetDevice().GetGpuResourceManager().Create(INPUT_BUFFER,
            bufferDesc_);
    }

    const float* buffer = shaderInputBuffer->Map(fSize);
    if (!buffer) {
        WIDGET_LOGE("3D shader input buffer update fail: map shaderInputBuffer error!");
        return false;
    }

    BASE_NS::array_view<const uint8_t> data(reinterpret_cast<const uint8_t*>(buffer), bSize);
    const RENDER_NS::BufferCopy bufferCopy{0, 0, bSize};

    renderDataStoreDefaultStaging_->CopyDataToBufferOnCpu(data, shaderInputBufferHandle_, bufferCopy);
    return true;
}

void LumeCustomRender::OnDrawFrame()
{
}

} // namespace name
