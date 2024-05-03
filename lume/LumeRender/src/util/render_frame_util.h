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

#ifndef RENDER_UTIL_RENDER_FRAME_UTIL_H
#define RENDER_UTIL_RENDER_FRAME_UTIL_H

#include <mutex>

#include <base/containers/vector.h>
#include <render/util/intf_render_frame_util.h>

#include "device/gpu_semaphore.h"

RENDER_BEGIN_NAMESPACE()
class IDevice;
class IRenderContext;
class IRenderDataStoreDefaultStaging;
class IRenderDataStoreDefaultGpuResourceDataCopy;

class RenderFrameUtil : public IRenderFrameUtil {
public:
    explicit RenderFrameUtil(const IRenderContext& renderContext);
    ~RenderFrameUtil() override = default;

    void BeginFrame();
    void EndFrame();

    bool HasGpuSignals() const;
    BASE_NS::array_view<BASE_NS::unique_ptr<GpuSemaphore>> GetGpuSemaphores();

    void CopyToCpu(const RenderHandleReference& handle, const CopyFlags flags) override;
    BASE_NS::array_view<IRenderFrameUtil::FrameCopyData> GetFrameCopyData() override;
    const FrameCopyData& GetFrameCopyData(const RenderHandleReference& handle) override;

    void SetBackBufferConfiguration(const BackBufferConfiguration& backBufferConfiguration) override;

    void AddGpuSignal(const SignalData& signalData) override;
    BASE_NS::array_view<SignalData> GetFrameGpuSignalData() override;
    SignalData GetFrameGpuSignalData(const RenderHandleReference& handle) override;

private:
    struct InternalFrameCopyData {
        uint64_t frameIndex { 0 };
        RenderHandleReference handle {};
        CopyFlags copyFlags { 0u };
    };
    struct CopyData {
        BASE_NS::vector<InternalFrameCopyData> copyData;
    };

    bool ValidateInput(const RenderHandleReference& handle);
    void ProcessFrameCopyData();
    void ProcessFrameInputCopyData(const CopyData& copyData);
    void ProcessFrameSignalData();
    void ProcessFrameBackBufferConfiguration();
    void ProcessFrameSignalDeferredDestroy();

    const IRenderContext& renderContext_;
    IDevice& device_;

    IRenderDataStoreDefaultStaging* dsStaging_ { nullptr };
    IRenderDataStoreDefaultGpuResourceDataCopy* dsCpuToGpuCopy_ { nullptr };

    // used for all
    mutable std::mutex mutex_;

    CopyData preFrame_;
    CopyData postFrame_;
    // buffered for non-wait-for-idle copies, resized for command buffering count + 1
    BASE_NS::vector<CopyData> bufferedPostFrame_;
    uint32_t bufferedIndex_ { 0u };

    // will hold the actual results from where the array_view is created for the user
    BASE_NS::vector<FrameCopyData> thisFrameCopiedData_;
    FrameCopyData defaultCopyData_;

    BASE_NS::vector<SignalData> preSignalData_;
    BASE_NS::vector<SignalData> postSignalData_;
    // will hold the actual results from where the array_view is created for the user
    struct FrameSignalData {
        BASE_NS::vector<BASE_NS::unique_ptr<GpuSemaphore>> gpuSemaphores;
        BASE_NS::vector<SignalData> signalData;
    };
    FrameSignalData thisFrameSignalData_;

    struct FrameBackBufferConfiguration {
        BackBufferConfiguration bbc;
        bool force { false };
    };
    FrameBackBufferConfiguration preBackBufferConfig_;
    FrameBackBufferConfiguration postBackBufferConfig_;

    struct GpuSignalBufferedDestroy {
        uint64_t frameUseIndex { 0 };
        BASE_NS::unique_ptr<GpuSemaphore> gpuSemaphore;
    };
    BASE_NS::vector<GpuSignalBufferedDestroy> gpuSignalDeferredDestroy_;

    // if there's even a single copy operation with wait we can copy all data
    bool frameHasWaitForIdle_ { false };
};
RENDER_END_NAMESPACE()

#endif // RENDER_UTIL_RENDER_FRAME_UTIL_H
