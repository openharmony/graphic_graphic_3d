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

#ifndef API_RENDER_NODE_CONTEXT_PSO_MANAGER_H
#define API_RENDER_NODE_CONTEXT_PSO_MANAGER_H

#include <cstddef>

#include <render/namespace.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
class Device;
struct PipelineLayout;
struct ViewportDesc;
struct ScissorDesc;
struct RenderPassDesc;

/** \addtogroup group_render_inodecontextpsomanager
 *  @{
 */
/** class INodeContextPsoManager
 * Creates and caches pipeline state objects.
 * Recources are:
 * - Compute psos
 * - Graphics psos
 * Prefer using the methods with handles.
 */
class INodeContextPsoManager {
public:
    INodeContextPsoManager(const INodeContextPsoManager&) = delete;
    INodeContextPsoManager& operator=(const INodeContextPsoManager&) = delete;

    /** Creates new compute psos on demand and caches
     * @param shaderHandle Handle to shader resource
     * @param pipelineLayoutHandle A valid handle to pipeline layout
     * @param shaderSpecialization Shader specialization
     * @return Compute pso handle
     */
    virtual RenderHandle GetComputePsoHandle(const RenderHandle shader, const RenderHandle pipelineLayout,
        const ShaderSpecializationConstantDataView& shaderSpecialization) = 0;

    /** Creates new compute psos on demand and caches
     * @param shaderHandle Handle to shader resource
     * @param pipelineLayout Pipeline layout
     * @param shaderSpecialization Shader specialization
     * @return Compute pso handle
     */
    virtual RenderHandle GetComputePsoHandle(const RenderHandle shader, const PipelineLayout& pipelineLayout,
        const ShaderSpecializationConstantDataView& shaderSpecialization) = 0;

    /** Creates new graphics psos on demand and caches. Prefer using this method.
     * Add additional dynamic state flags that will be combined with the ones in shader handle's GraphicsState
     * Prefer using dynamic viewport and dynamic scissor for less graphics pipelines
     * All inputs are copied, ie. no need to store them
     * @param shaderHandle A valid shader handle
     * @param graphicsStateHandle A valid shader handle
     * @param pipelineLayoutHandle A valid pipeline layout handle
     * @param vertexInputDeclarationHandle If a valid handle, it is used. Otherwise empty vertex input declaration.
     * @param graphicsStateHandle If a valid handle, it overrides the default graphics state which comes from the
     * shader.
     * @param shaderSpecialization Shader specialization
     * @param dynamicStateFlags Dynamic state flags
     * @param graphicsState Override graphics state
     * @return Graphics pso handle
     */
    virtual RenderHandle GetGraphicsPsoHandle(const RenderHandle shader, const RenderHandle graphicsState,
        const RenderHandle pipelineLayout, const RenderHandle vertexInputDeclaration,
        const ShaderSpecializationConstantDataView& shaderSpecialization,
        const DynamicStateFlags dynamicStateFlags) = 0;

    /** Creates new graphics psos on demand and caches.
     * Add additional dynamic state flags that will be combined with the ones in shader handle's GraphicsState
     * Prefer using dynamic viewport and dynamic scissor for less graphics pipelines
     * All inputs are copied, ie. no need to store them
     * @param shaderHandle Handle to shader resource
     * @param pipelineLayout Pipeline layout
     * @param vertexInputDeclarationView Vertex input declaration view
     * @param shaderSpecialization Shader specialization
     * @param dynamicStateFlags Dynamic state flags
     * @return Graphics pso handle
     */
    virtual RenderHandle GetGraphicsPsoHandle(const RenderHandle shader, const RenderHandle graphicsState,
        const PipelineLayout& pipelineLayout, const VertexInputDeclarationView& vertexInputDeclarationView,
        const ShaderSpecializationConstantDataView& shaderSpecialization,
        const DynamicStateFlags dynamicStateFlags) = 0;

    /** Creates new graphics psos on demand and caches. Overrides the default graphics state from shader.
     * Add additional dynamic state flags that will be combined with the ones in shader handle's GraphicsState
     * Prefer using dynamic viewport and dynamic scissor for less graphics pipelines
     * All inputs are copied, ie. no need to store them
     * @param shaderHandle Handle to shader resource
     * @param pipelineLayout Pipeline layout
     * @param vertexInputDeclarationView Vertex input declaration view
     * @param shaderSpecialization Shader specialization
     * @param dynamicStateFlags Dynamic state flags
     * @param graphicsState A graphics state which overrides the default graphics state from the shader
     * @return Graphics pso handle
     */
    virtual RenderHandle GetGraphicsPsoHandle(const RenderHandle shaderHandle, const GraphicsState& graphicsState,
        const PipelineLayout& pipelineLayout, const VertexInputDeclarationView& vertexInputDeclarationView,
        const ShaderSpecializationConstantDataView& shaderSpecialization,
        const DynamicStateFlags dynamicStateFlags) = 0;

protected:
    INodeContextPsoManager() = default;
    virtual ~INodeContextPsoManager() = default;
};
/** @} */
RENDER_END_NAMESPACE()

#endif // API_RENDER_NODE_CONTEXT_PSO_MANAGER_H
