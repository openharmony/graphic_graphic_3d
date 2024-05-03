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

#ifndef RENDER_BACKEND_H
#define RENDER_BACKEND_H

#include <base/containers/vector.h>
#include <core/namespace.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
class Device;
class GpuResourceManager;
struct RenderCommandContext;
struct RenderCommandFrameData;

struct RenderBackendBackBufferConfiguration {
    struct SwapchainData {
        RenderHandle handle;
        // state after render node graph processing
        GpuResourceState backBufferState;
        // layout after render node graph processing
        ImageLayout layout { ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED };
        // configuration for the back buffer
        NodeGraphBackBufferConfiguration config;
    };
    BASE_NS::vector<SwapchainData> swapchainData;
};

class RenderBackend {
public:
    RenderBackend() = default;
    virtual ~RenderBackend() = default;

    virtual void Render(RenderCommandFrameData& renderCommandFrameData,
        const RenderBackendBackBufferConfiguration& backBufferConfig) = 0;
    virtual void Present(const RenderBackendBackBufferConfiguration& backBufferConfig) = 0;

private:
};
RENDER_END_NAMESPACE()

#endif // RENDER_BACKEND_H
