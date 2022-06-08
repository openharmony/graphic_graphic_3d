/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "render_node_back_buffer_gpu_buffer.h"

#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_util.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

#include "default_engine_constants.h"
#include "util/log.h"

RENDER_BEGIN_NAMESPACE()
namespace {
void WriteImageToBuffer(
    IRenderCommandList& cmdList, RenderHandle imageHandle, uint32_t width, uint32_t height, RenderHandle bufferHandle)
{
    const BufferImageCopy copyRef {
        0, // bufferOffset
        0, // bufferRowLength
        0, // bufferImageHeight
        {
            ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, // imageAspectFlags
            0,                                                // mipLevel
            0,                                                // baseArrayLayer
            1,                                                // layerCount
        },                                                    // imageSubresource
        {},                                                   // imageOffset
        {
            width,  // width
            height, // height
            1,      // depth
        },          // imageExtend
    };
    cmdList.CopyImageToBuffer(imageHandle, bufferHandle, copyRef);
}
} // namespace

void RenderNodeBackBufferGpuBuffer::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
}

void RenderNodeBackBufferGpuBuffer::ExecuteFrame(IRenderCommandList& cmdList)
{
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto* dataStore = renderDataStoreMgr.GetRenderDataStore("RenderDataStorePod");
    if (dataStore) {
        auto const dataView =
            static_cast<IRenderDataStorePod const*>(dataStore)->Get("NodeGraphBackBufferConfiguration");
        const NodeGraphBackBufferConfiguration* data = (const NodeGraphBackBufferConfiguration*)dataView.data();
        if (data) {
            if (data->backBufferType == NodeGraphBackBufferConfiguration::BackBufferType::GPU_IMAGE_BUFFER_COPY) {
                IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
                RenderHandle backbufferHandle = data->backBufferHandle.GetHandle();
                if (data->backBufferName == DefaultEngineGpuResourceConstants::CORE_DEFAULT_BACKBUFFER) {
                    // we need to use the core default backbuffer handle and not the replaced handle in this situation
                    backbufferHandle =
                        gpuResourceMgr.GetImageHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_BACKBUFFER);
                }
                const GpuImageDesc dstImageDesc = gpuResourceMgr.GetImageDescriptor(backbufferHandle);
                if (gpuResourceMgr.IsGpuImage(backbufferHandle) &&
                    gpuResourceMgr.IsGpuBuffer(data->gpuBufferHandle.GetHandle())) {
                    WriteImageToBuffer(cmdList, backbufferHandle, dstImageDesc.width, dstImageDesc.height,
                        data->gpuBufferHandle.GetHandle());
                }
            }
        } else {
            PLUGIN_LOG_E("data store NodeGraphBackBufferConfiguration not found");
        }
    }
}

// for plugin / factory interface
IRenderNode* RenderNodeBackBufferGpuBuffer::Create()
{
    return new RenderNodeBackBufferGpuBuffer();
}

void RenderNodeBackBufferGpuBuffer::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeBackBufferGpuBuffer*>(instance);
}
RENDER_END_NAMESPACE()
