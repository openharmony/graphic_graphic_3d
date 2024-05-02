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

#ifndef API_RENDER_RENDER_NODE_UTIL_H
#define API_RENDER_RENDER_NODE_UTIL_H

#include <base/containers/array_view.h>
#include <base/containers/unique_ptr.h>
#include <render/datastore/intf_render_data_store_post_process.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
struct PipelineLayout;

/** @ingroup group_render_irendernodeutil */
/** Interface for helper class to create different required rendering types from inputs.
 *  Can create objects through IRenderNodeContextManager.
 */
class IRenderNodeUtil {
public:
    /** Create input render pass from json data input render pass.
     * @param renderPass Render pass from json format.
     * @return Input render pass with handles. I.e. resources and their type-mapping used for rendering.
     */
    virtual RenderNodeHandles::InputRenderPass CreateInputRenderPass(
        const RenderNodeGraphInputs::InputRenderPass& renderPass) const = 0;

    /** Create input resources from json data input render pass.
     * @param inputResources Resources from json format.
     * @return Input resources with handles. I.e. resources and their type-mapping used for rendering.
     */
    virtual RenderNodeHandles::InputResources CreateInputResources(
        const RenderNodeGraphInputs::InputResources& inputResources) const = 0;

    /** Create a render pass based on render node handles.
     * @param renderPass Input render pass.
     * @return Render pass struct.
     */
    virtual RenderPass CreateRenderPass(const RenderNodeHandles::InputRenderPass& renderPass) const = 0;

    /** Create pipeline layout based on PL which is attached to the shader or if not found create from shader
     * reflection.
     * @param shaderHandle Shader handle.
     * @return Pipeline layout that was reflected from the given shader.
     */
    virtual PipelineLayout CreatePipelineLayout(const RenderHandle& shaderHandle) const = 0;

    /** Get descriptor counts from pipeline layout.
     * @param pipelineLayout Valid pipeline layout which matches upcoming bindings.
     * @return Descriptor counts struct used to pass for reserving render node specific descriptor sets.
     */
    virtual DescriptorCounts GetDescriptorCounts(const PipelineLayout& pipelineLayout) const = 0;

    /** Get descriptor counts from desriptor set layout bindings.
     * @param bindings Bindings.
     * @return Descriptor counts struct used to pass for reserving render node specific descriptor sets.
     */
    virtual DescriptorCounts GetDescriptorCounts(
        const BASE_NS::array_view<DescriptorSetLayoutBinding> bindings) const = 0;

    /** Create pipeline descriptor set binder.
     * @param renderNodeContextMgr Access to render node resource managers.
     * @param pipelineLayout Pipeline layout.
     * @return A pipeline descriptor set binder based on pipeline layout.
     */
    virtual IPipelineDescriptorSetBinder::Ptr CreatePipelineDescriptorSetBinder(
        const PipelineLayout& pipelineLayout) const = 0;

    /** Bind render node handle input resources to binder.
     * @param resources Render node handle input resources.
     * @param pipelineDescriptorSetBinder Pipeline descriptor set binder.
     */
    virtual void BindResourcesToBinder(const RenderNodeHandles::InputResources& resources,
        IPipelineDescriptorSetBinder& pipelineDescriptorSetBinder) const = 0;

    /** Create default viewport based on render pass attachments render area.
     * @param renderPass Render pass.
     * @return A viewport desc based on render pass attachments.
     */
    virtual ViewportDesc CreateDefaultViewport(const RenderPass& renderPass) const = 0;

    /** Create default scissor based on render pass attachments render area.
     * @param renderPass Render pass.
     * @return A scissor descriptor based on render pass attachments.
     */
    virtual ScissorDesc CreateDefaultScissor(const RenderPass& renderPass) const = 0;

    /** Create post process configuration for shader usage.
     * @param postProcessConfiguration Post process configuration.
     * @return A RenderPostProcessConfiguration.
     */
    virtual RenderPostProcessConfiguration GetRenderPostProcessConfiguration(
        const PostProcessConfiguration& postProcessConfiguration) const = 0;

    /** Create post process configuration for shader usage.
     * @param postProcessConfiguration Post process configuration.
     * @return A RenderPostProcessConfiguration.
     */
    virtual RenderPostProcessConfiguration GetRenderPostProcessConfiguration(
        const IRenderDataStorePostProcess::GlobalFactors& globalFactors) const = 0;

    /** Has resources in render pass that might change every frame.
     * For example broadcasted resources from different render nodes might change every frame.
     * @param renderPass Input render pass for evaluation.
     * @return Boolean if one should re-check/re-fetch new handles every frame.
     */
    virtual bool HasChangeableResources(const RenderNodeGraphInputs::InputRenderPass& renderPass) const = 0;

    /** Has resources as inputs that might change every frame.
     * For example broadcasted resources from different render nodes might change every frame.
     * @param resources Input resources for evaluation.
     * @return Boolean if one should re-check/re-fetch new handles every frame.
     */
    virtual bool HasChangeableResources(const RenderNodeGraphInputs::InputResources& resources) const = 0;

protected:
    IRenderNodeUtil() = default;
    virtual ~IRenderNodeUtil() = default;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_RENDER_NODE_UTIL_H
