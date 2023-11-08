/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include <render/device/pipeline_layout_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
/** \addtogroup group_ishaderpipelinebinder
 *  @{
 */
/** Shader pipeline binder object
 */
class IShaderPipelineBinder {
public:
    /** Bindable buffer */
    struct Buffer {
        /** Handle */
        RenderHandleReference handle;
        /** Byte offset to buffer */
        uint32_t byteOffset { 0u };
        /** Byte size for buffer binding */
        uint32_t byteSize { PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE };
    };

    /** Bindable image */
    struct Image {
        /** Handle */
        RenderHandleReference handle;
        /** Mip level for specific binding */
        uint32_t mip { PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS };
        /** Layer level for specific binding */
        uint32_t layer { PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
        /** Sampler handle for combined image sampler */
        RenderHandleReference samplerHandle;
    };

    /** Bindable sampler */
    struct Sampler {
        /** Handle */
        RenderHandleReference handle;
    };

    /** Bindable binding */
    struct Binding {
        /** Binding */
        uint32_t binding { ~0u };
        /** Index to resources arrays */
        uint32_t resIdx { ~0u };
        /** Type for resources arrays */
        RenderHandleType type { RenderHandleType::UNDEFINED };
    };

    /** Bindable resource view
     */
    struct DescriptorSetView {
        BASE_NS::array_view<const Buffer> buffers;
        BASE_NS::array_view<const Image> images;
        BASE_NS::array_view<const Sampler> samplers;

        BASE_NS::array_view<const Binding> bindings;
    };

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
    virtual void Bind(const uint32_t set, const uint32_t binding, const RenderHandleReference& handle) = 0;

    /** Set uniform data which will be bind to shader with UBO in set/binding.
     * @param set Set index
     * @param binding Binding index
     * @param data Uniform data which is bind to shader with UBO in correct set/binding.
     */
    virtual void SetUniformData(
        const uint32_t set, const uint32_t binding, const BASE_NS::array_view<const uint8_t> data) = 0;

    /** Set push constant data for shader access.
     * @param data Uniform data which is bind to shader with UBO in correct set/binding.
     */
    virtual void SetPushConstantData(const BASE_NS::array_view<const uint8_t> data) = 0;

    /** Bind buffer.
     * @param set Set index
     * @param binding Binding index
     * @param resource Binding resource
     * buffer)
     */
    virtual void BindBuffer(const uint32_t set, const uint32_t binding, const Buffer& resource) = 0;

    /** Bind image.
     * @param set Set index
     * @param binding Binding index
     * @param resource Binding resource handle
     */
    virtual void BindImage(const uint32_t set, const uint32_t binding, const Image& resource) = 0;

    /** Bind sampler
     * @param set Set index
     * @param binding Binding index
     * @param handle Binding resource handle
     */
    virtual void BindSampler(const uint32_t set, const uint32_t binding, const Sampler& resource) = 0;

    /** Get bindable resources
     * @return BindableResourceView to resources
     */
    virtual DescriptorSetView GetDescriptorSetView(const uint32_t set) const = 0;

    /** Get push constant data
     * @return array_view of push data
     */
    virtual BASE_NS::array_view<const uint8_t> GetPushData() const = 0;

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
