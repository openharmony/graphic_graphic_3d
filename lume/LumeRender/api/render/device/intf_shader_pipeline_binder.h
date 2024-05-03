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

#ifndef API_RENDER_DEVICE_ISHADER_PIPELINE_BINDER_H
#define API_RENDER_DEVICE_ISHADER_PIPELINE_BINDER_H

#include <base/containers/array_view.h>
#include <base/containers/unique_ptr.h>
#include <core/property/intf_property_handle.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
/** \addtogroup group_ishaderpipelinebinder
 *  @{
 */
/** Shader pipeline binder object
 */
class IShaderPipelineBinder {
public:
    /** Dispatch data */
    struct DispatchCommand {
        /**
         * Render handle for dispatching.
         * 1. If image -> imageSize / tgs
         * 2. If buffer -> dispatch indirect
         */
        RenderHandleReference handle {};
        /**
         * Thread group count.
         * Only used if the handle is invalid.
         */
        ShaderThreadGroup threadGroupCount;

        /** Indirect args buffer offset */
        uint32_t argsOffset { 0U };
    };
    /** Draw command */
    struct DrawCommand {
        /** Vertex count */
        uint32_t vertexCount { 0U };
        /** Index count */
        uint32_t indexCount { 0U };
        /** Instance count */
        uint32_t instanceCount { 1U };

        /** Indirect args */
        RenderHandleReference argsHandle {};
        /** Indirect args buffer offset */
        uint32_t argsOffset { 0U };
    };
    /** Property binding data */
    struct PropertyBindingView {
        /** Descriptor set index */
        uint32_t set { ~0U };
        /** Descriptor set binding index */
        uint32_t binding { ~0U };

        /** Array view to data */
        BASE_NS::array_view<const uint8_t> data;
    };
    /** Resource binding */
    struct ResourceBinding {
        /** Descriptor set index */
        uint32_t set { PipelineLayoutConstants::INVALID_INDEX };
        /** Descriptor set binding index */
        uint32_t binding { PipelineLayoutConstants::INVALID_INDEX };
        /** Descriptor count */
        uint32_t descriptorCount { 0U };
        /** Descriptor set binding array offset for array resources. Offset to first array resource. */
        uint32_t arrayOffset { 0U };

        /** Resource handle */
        RenderHandleReference handle;
    };

    /** Clear bindings.
     */
    virtual void ClearBindings() = 0;

    /** Get binding validity. Checks through all bindings that they have valid handles.
     */
    virtual bool GetBindingValidity() const = 0;

    /** Get shader handle.
     * Shader handle is given with the creation with shader manager and it cannot be changed.
     */
    virtual RenderHandleReference GetShaderHandle() const = 0;

    /** Bind any resource with defaults. Automatically checks needed flags from shader pipeline layout.
     * @param set Set index
     * @param binding Binding index
     * @param handle Binding resource handle
     */
    virtual void Bind(uint32_t set, uint32_t binding, const RenderHandleReference& handle) = 0;

    /** Set push constant data for shader access.
     * @param data Uniform data which is bind to shader with UBO in correct set/binding.
     */
    virtual void SetPushConstantData(BASE_NS::array_view<const uint8_t> data) = 0;

    /** Bind buffer.
     * @param set Set index
     * @param binding Binding index
     * @param resource Binding resource
     * buffer)
     */
    virtual void BindBuffer(uint32_t set, uint32_t binding, const BindableBufferWithHandleReference& resource) = 0;

    /** Bind buffers for array binding.
     * @param set Set index
     * @param binding Binding index
     * @param resources Binding resources
     * buffer)
     */
    virtual void BindBuffers(
        uint32_t set, uint32_t binding, BASE_NS::array_view<const BindableBufferWithHandleReference> resources) = 0;
    
    /** Bind image.
     * @param set Set index
     * @param binding Binding index
     * @param resource Binding resource handle
     */
    virtual void BindImage(uint32_t set, uint32_t binding, const BindableImageWithHandleReference& resource) = 0;
    
    /** Bind images for array bindings.
     * @param set Set index
     * @param binding Binding index
     * @param resource Binding resource handle
     */
    virtual void BindImages(
        uint32_t set, uint32_t binding, BASE_NS::array_view<const BindableImageWithHandleReference> resources) = 0;

    /** Bind sampler
     * @param set Set index
     * @param binding Binding index
     * @param handle Binding resource handle
     */
    virtual void BindSampler(uint32_t set, uint32_t binding, const BindableSamplerWithHandleReference& resource) = 0;

    /** Bind sampler for array bindings.
     * @param set Set index
     * @param binding Binding index
     * @param handle Binding resource handle
     */
    virtual void BindSamplers(
        uint32_t set, uint32_t binding, BASE_NS::array_view<const BindableSamplerWithHandleReference> resources) = 0;
    
    /** Bind vertex buffers
     * @param vertexBuffers vertex buffers
     */
    virtual void BindVertexBuffers(BASE_NS::array_view<const VertexBufferWithHandleReference> vertexBuffers) = 0;

    /** Bind index Buffer
     * @param indexBuffer vertex Buffer
     */
    virtual void BindIndexBuffer(const IndexBufferWithHandleReference& indexBuffer) = 0;

    /** Set draw command
     * @param indexBuffer vertex Buffer
     */
    virtual void SetDrawCommand(const DrawCommand& drawCommand) = 0;

    /** Bind dispatch command
     * @param indexBuffer vertex Buffer
     */
    virtual void SetDispatchCommand(const DispatchCommand& dispatchCommand) = 0;

    /** Get render time bindable resources
     * @return DescriptorSetLayoutBindingResources to resources
     */
    virtual DescriptorSetLayoutBindingResources GetDescriptorSetLayoutBindingResources(uint32_t set) const = 0;

    /** Get push constant data
     * @return array_view of push data
     */
    virtual BASE_NS::array_view<const uint8_t> GetPushConstantData() const = 0;

    /** Get vertex buffers
     * @return View of vertex buffers
     */
    virtual BASE_NS::array_view<const VertexBufferWithHandleReference> GetVertexBuffers() const = 0;

    /** Get index buffers
     * @return IndexBuffer
     */
    virtual IndexBufferWithHandleReference GetIndexBuffer() const = 0;

    /** Get draw command
     * @return DrawCommand
     */
    virtual DrawCommand GetDrawCommand() const = 0;

    /** Get dispatch command
     * @return DispatchCommand
     */
    virtual DispatchCommand GetDispatchCommand() const = 0;

    /** Get pipeline Layout
     * @return PipelineLayout
     */
    virtual PipelineLayout GetPipelineLayout() const = 0;

    /** Get property handle for built-in custom, material properties. Check th pointer always.
     *  @return Pointer to property handle if properties present, nullptr otherwise.
     */
   virtual CORE_NS::IPropertyHandle* GetProperties() = 0;

   /** Get binding property handle for built-in binding properties. Check the pointer always.
    * @return Pointer to property handle if properties present, nullptr otherwise.
    */
   virtual CORE_NS::IPropertyHandle* GetBindingProperties() = 0;

    /** Get resource binding.
     * @param set Set index
     * @param binding Binding index
     * @return Resource binding
     */
    virtual ResourceBinding GetResourceBinding(uint32_t set, uint32_t binding) const = 0;

    /** Get property binding data and binding information
     *  the data is not automatically move to GPU access (render node(s) should handle that)
     * @return PropertyBindingView
     */
    virtual PropertyBindingView GetPropertyBindingView() const = 0;

    /** Get property binding byte size.
     * @return Data byte size.
     */
    virtual uint32_t GetPropertyBindingByteSize() const = 0;

    using Ptr = BASE_NS::refcnt_ptr<IShaderPipelineBinder>;

protected:
    // reference counting
    // take a new reference of the object
    virtual void Ref() = 0;
    // releases one reference of the object.
    // no methods of the class shall be called after unref.
    // The object could be destroyed, if last reference
    virtual void Unref() = 0;
    // allow refcnt_ptr to call Ref/Unref.
    friend Ptr;

    IShaderPipelineBinder() = default;
    virtual ~IShaderPipelineBinder() = default;

    IShaderPipelineBinder(const IShaderPipelineBinder&) = delete;
    IShaderPipelineBinder& operator=(const IShaderPipelineBinder&) = delete;
    IShaderPipelineBinder(IShaderPipelineBinder&&) = delete;
    IShaderPipelineBinder& operator=(IShaderPipelineBinder&&) = delete;
};
/** @} */
RENDER_END_NAMESPACE()

#endif // API_RENDER_DEVICE_ISHADER_PIPELINE_BINDER_H
