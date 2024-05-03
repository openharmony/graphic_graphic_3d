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

#ifndef RENDER_RENDER__NODE__RENDER_STAGING_H
#define RENDER_RENDER__NODE__RENDER_STAGING_H

#include <render/device/intf_gpu_resource_manager.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>

#include "device/gpu_resource_manager.h"

RENDER_BEGIN_NAMESPACE()
class IRenderCommandList;
class IRenderNodeContextManager;
struct RenderNodeGraphInputs;
struct StagingDirectDataCopyConsumeStruct;
struct StagingImageClearConsumeStruct;

class RenderStaging final {
public:
    RenderStaging() = default;
    ~RenderStaging() = default;

    void Init(IRenderNodeContextManager& renderNodeContextMgr);

    // additional clear byte size for GLES backend
    // we need to allocate a CPU/GPU buffer to copy data from CPU to texture
    void PreExecuteFrame(const uint32_t clearByteSize);

    void CopyHostToStaging(
        const IRenderNodeGpuResourceManager& gpuResourceMgr, const StagingConsumeStruct& stagingData);
    void CopyStagingToImages(IRenderCommandList& cmdList, const IRenderNodeGpuResourceManager& gpuResourceMgr,
        const StagingConsumeStruct& stagingData, const StagingConsumeStruct& renderDataStoreStagingData);
    void CopyBuffersToBuffers(IRenderCommandList& cmdList, const StagingConsumeStruct& stagingData,
        const StagingConsumeStruct& renderDataStoreStagingData);
    void CopyImagesToBuffers(IRenderCommandList& cmdList, const IRenderNodeGpuResourceManager& gpuResourceMgr,
        const StagingConsumeStruct& stagingData, const StagingConsumeStruct& renderDataStoreStagingData);
    void CopyImagesToImages(IRenderCommandList& cmdList, const IRenderNodeGpuResourceManager& gpuResourceMgr,
        const StagingConsumeStruct& stagingData, const StagingConsumeStruct& renderDataStoreStagingData);

    void ClearImages(IRenderCommandList& cmdList, const IRenderNodeGpuResourceManager& gpuResourceMgr,
        const StagingImageClearConsumeStruct& imageClearData);

private:
    IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    // GLES cannot directly use graphics API clear commands
    // we still want to do CPU-GPU copy in multi-threaded way, so render front-end is used
    struct AddionalCopyBuffer {
        RenderHandleReference handle;
        uint32_t byteSize { 0u };
        uint32_t byteOffset { 0u };
    };
    AddionalCopyBuffer additionalCopyBuffer_;
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_NODE_STAGING_H
