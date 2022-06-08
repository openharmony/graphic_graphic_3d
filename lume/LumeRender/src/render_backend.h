/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef RENDER_BACKEND_H
#define RENDER_BACKEND_H

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
    // state after render node graph processing
    GpuResourceState backBufferState;
    // layout after render node graph processing
    ImageLayout layout { ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED };
    // configuration for the back buffer
    NodeGraphBackBufferConfiguration config;
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
