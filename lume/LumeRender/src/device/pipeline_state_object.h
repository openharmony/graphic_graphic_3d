/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef RENDER_GFX__PIPELINE_STATE_OBJECT_H
#define RENDER_GFX__PIPELINE_STATE_OBJECT_H

#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()

/** Graphics pipeline state object.
 * Full compiled pipeline with all the static state data.
 */
class GraphicsPipelineStateObject {
public:
    GraphicsPipelineStateObject() = default;
    virtual ~GraphicsPipelineStateObject() = default;

    GraphicsPipelineStateObject(const GraphicsPipelineStateObject&) = delete;
    GraphicsPipelineStateObject& operator=(const GraphicsPipelineStateObject&) = delete;
};

/** Compute pipeline state obect.
 * Full compiled pipeline.
 */
class ComputePipelineStateObject {
public:
    ComputePipelineStateObject() = default;
    virtual ~ComputePipelineStateObject() = default;

    ComputePipelineStateObject(const ComputePipelineStateObject&) = delete;
    ComputePipelineStateObject& operator=(const ComputePipelineStateObject&) = delete;
};
RENDER_END_NAMESPACE()

#endif // CORE__GFX__PIPELINE_STATE_OBJECT_H
