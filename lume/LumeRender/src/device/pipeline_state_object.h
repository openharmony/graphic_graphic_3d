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
