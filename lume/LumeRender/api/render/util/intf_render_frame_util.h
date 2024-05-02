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

#ifndef API_RENDER_UTIL_IRENDER_FRAME_UTIL_H
#define API_RENDER_UTIL_IRENDER_FRAME_UTIL_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/byte_array.h>
#include <base/containers/string.h>
#include <base/containers/unique_ptr.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
/** @ingroup group_util_irenderframeutil */

/** Interface for render frame utilities.
 * CopyToCpu is internally synchronized
 * SetBackBufferConfiguration is internally synchronized
 */
class IRenderFrameUtil {
public:
    /** Copy flag bits */
    enum CopyFlagBits {
        /** Wait for idle is called before copying the data */
        WAIT_FOR_IDLE = (1 << 0),
        /** GPU copy to CPU accessable buffer */
        GPU_BUFFER_ONLY = (1 << 1),
    };
    /** Copy flag bits container */
    using CopyFlags = uint32_t;

    /** Low level type of the opaque signal resource handle */
    enum class SignalResourceType : uint32_t {
        /** Undefined */
        UNDEFINED,
        /** GPU semaphore */
        GPU_SEMAPHORE,
        /** GPU fence */
        GPU_FENCE,
    };

    /** Frame copy data */
    struct FrameCopyData {
        /** handle for reference/check-up */
        BASE_NS::unique_ptr<BASE_NS::ByteArray> byteBuffer;
        /** frame index */
        uint64_t frameIndex { 0 };
        /** handle for reference/check-up */
        RenderHandleReference handle {};
        /** copy flags */
        CopyFlags copyFlags { 0u };
        /** additional buffer handle if doing GPU -> GPU readable copy */
        RenderHandleReference bufferHandle {};
    };
    /** Back buffer configuration
     * Which overwrites NodeGraphBackBufferConfiguration POD
     */
    struct BackBufferConfiguration {
        /** Back buffer type */
        enum class BackBufferType : uint32_t {
            /** Undefined */
            UNDEFINED,
            /** Swapchain */
            SWAPCHAIN,
            /** GPU image */
            GPU_IMAGE,
            /** GPU image buffer copy */
            GPU_IMAGE_BUFFER_COPY,
        };

        /** Name of the GpuImage in render node graph json where rendering happens.
         * Image with this name must have been created by the application (or by the renderer). */
        BASE_NS::string backBufferName { "CORE_DEFAULT_BACKBUFFER" };

        /** Back buffer type */
        BackBufferType backBufferType = { BackBufferType::UNDEFINED };

        /** Handle to the final target.
         * If backbufferType is SWAPCHAIN this handle is not used.
         * If backbufferType is GPU_IMAGE this must point to a valid GpuImage. */
        RenderHandleReference backBufferHandle;
        /* If backbufferType is GPU_IMAGE_BUFFER_COPY this must point to a valid GpuBuffer where image data is copied.
         */
        RenderHandleReference gpuBufferHandle;

        /** Present */
        bool present { false };

        /** Binary semaphore for signaling end of frame, i.e. when rendered to back buffer */
        uint64_t gpuSemaphoreHandle { 0 };
    };

    /** Signal resource object which can be retrieved after render frame has been processed
     */
    struct SignalData {
        /** Handle if it was attached to some handle */
        RenderHandleReference handle;
        /** Quarantee that the semaphore object has been signaled */
        bool signaled { false };
        /** Low level signal resource handle (In Vulkan GPU semaphore) */
        uint64_t gpuSignalResourceHandle { 0 };
        /** Low level signal resource type */
        SignalResourceType signalResourceType {};
    };

    /** Copy data to CPU the next frame if waiting for idle.
     * @param handle Render handle reference of the resource.
     * @param flags Copy flag bits.
     */
    virtual void CopyToCpu(const RenderHandleReference& handle, const CopyFlags flags) = 0;

    /** Get copied frame data. Valid after RenderFrame and stays valid for next RenderFrame call.
     * NOTE: do not call during RenderFrame() not thread safe during rendering.
     * @return Array view of copied data.
     */
    virtual BASE_NS::array_view<FrameCopyData> GetFrameCopyData() = 0;

    /** Get const reference to copied frame data by id.
     * Valid after RenderFrame and stays valid for next RenderFrame call.
     * NOTE: do not call during RenderFrame() not thread safe during rendering.
     * @param handle The handle which was used when copy was added.
     * @return Const reference of copy data.
     */
    virtual const FrameCopyData& GetFrameCopyData(const RenderHandleReference& handle) = 0;

    /** Set backbuffer configuration.
     * If this is set once, it will update the NodeGraphBackBufferConfiguration POD every frame.
     * @param backBufferConfiguration which will be hold and updated every frame
     */
    virtual void SetBackBufferConfiguration(const BackBufferConfiguration& backBufferConfiguration) = 0;

    /** Add GPU only signal to be signaled this frame.
     * Guarantees that the signal will be signaled if anything is processed in the backend.
     * Does not guarantee that the signal is immediate after the specific handle has been processed on the GPU.
     * If the signal resource handle is zero rendering side resource is created automatically.
     * @param signalData All the values are optional.
     */
    virtual void AddGpuSignal(const SignalData& signalData) = 0;

    /** Get GPU frame signal data.
     * Get all signal data for processed frame.
     * Be sure to check the boolean flag, and the gpuSemaphoreHandle before casting.
     * @return array view of frame signal data.
     */
    virtual BASE_NS::array_view<SignalData> GetFrameGpuSignalData() = 0;

    /** Get frame signal data for a specific handle.
     * Make sure to check the boolean flag every time before using.
     * Be sure to check the boolean flag, and the gpuSemaphoreHandle before casting
     * @param handle The handle which was used when signal was added.
     * @return semaphore data if it was found.
     */
    virtual SignalData GetFrameGpuSignalData(const RenderHandleReference& handle) = 0;

protected:
    IRenderFrameUtil() = default;
    virtual ~IRenderFrameUtil() = default;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_UTIL_IRENDER_FRAME_UTIL_H
