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

#ifndef API_RENDER_IPIPELINE_DESCRIPTORSET_BINDER_H
#define API_RENDER_IPIPELINE_DESCRIPTORSET_BINDER_H

#include <base/containers/array_view.h>
#include <base/containers/unique_ptr.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
/** @ingroup group_render_ipipelinedescriptorsetbinder */
/** Descriptor set binder interface class */
class IDescriptorSetBinder {
public:
    /** Clear bind handles. This should be used to reset if the same binder is used in many frames.
     */
    virtual void ClearBindings() = 0;

    /** Get descriptor set handle */
    virtual RenderHandle GetDescriptorSetHandle() const = 0;

    /** Get descriptor set layout binding resources */
    virtual DescriptorSetLayoutBindingResources GetDescriptorSetLayoutBindingResources() const = 0;

    /** Get descriptor set layout binding validity. Checks through all bindings that they have valid handles.
     * When the validity of the bindings might be up-to API user, this should be called.
     */
    virtual bool GetDescriptorSetLayoutBindingValidity() const = 0;

    /** Bind buffer. Automatically checks needed flag from pipeline layout which was used for descriptor set binder
     * creation.
     * @param binding Binding index
     * @param handle Binding resource handle
     * @param byteOffset Byte offset
     */
    virtual void BindBuffer(const uint32_t binding, const RenderHandle handle, const uint32_t byteOffset) = 0;

    /** Bind buffer. Automatically checks needed flag from pipeline layout which was used for descriptor set binder
     * creation.
     * @param binding Binding index
     * @param handle Binding resource handle
     * @param byteOffset Byte offset
     * @param byteSize Byte size for binding if needed
     */
    virtual void BindBuffer(
        const uint32_t binding, const RenderHandle handle, const uint32_t byteOffset, const uint32_t byteSize) = 0;

    /** Bind buffer. Automatically checks needed flag from pipeline layout which was used for descriptor set binder
     * creation.
     * @param binding Binding index
     * @param resource Binding resource
     */
    virtual void BindBuffer(const uint32_t binding, const BindableBuffer& resource) = 0;

    /** Bind buffers to descriptor array
     * @param binding Binding index
     * @param resources Binding resources
     */
    virtual void BindBuffers(const uint32_t binding, const BASE_NS::array_view<const BindableBuffer> resources) = 0;

    /** Bind image.
     * @param binding Binding index
     * @param handle Binding resource handle
     */
    virtual void BindImage(const uint32_t binding, const RenderHandle handle) = 0;

    /** Bind image.
     * @param binding Binding index
     * @param handle Binding resource handle
     * @param samplerHandle Binding resource handle for combined image sampler
     */
    virtual void BindImage(const uint32_t binding, const RenderHandle handle, const RenderHandle samplerHandle) = 0;

    /** Bind image.
     * @param binding Binding index
     * @param resource Binding resources
     */
    virtual void BindImage(const uint32_t binding, const BindableImage& resource) = 0;

    /** Bind images to descriptor array
     * @param binding Binding index
     * @param resources Binding resources
     */
    virtual void BindImages(const uint32_t binding, const BASE_NS::array_view<const BindableImage> resources) = 0;

    /** Bind sampler
     * @param binding Binding index
     * @param handle Binding resource handle
     */
    virtual void BindSampler(const uint32_t binding, const RenderHandle handle) = 0;

    /** Bind sampler
     * @param binding Binding index
     * @param resource Binding resource
     */
    virtual void BindSampler(const uint32_t binding, const BindableSampler& resource) = 0;

    /** Bind sampler
     * @param binding Binding index
     * @param handles Binding resource handles
     */
    virtual void BindSamplers(const uint32_t binding, const BASE_NS::array_view<const BindableSampler> resources) = 0;

    /** Bind buffer with additional descriptor flags. Automatically checks needed flag from pipeline layout which was
     * used for descriptor set binder creation.
     * @param binding Binding index
     * @param resource Binding resource
     * @param flags Additional decriptor flags
     */
    virtual void BindBuffer(
        const uint32_t binding, const BindableBuffer& resource, const AdditionalDescriptorFlags flags) = 0;

    /** Bind image with additional descriptor flags.
     * @param binding Binding index
     * @param resource Binding resources
     * @param flags Additional decriptor flags
     */
    virtual void BindImage(
        const uint32_t binding, const BindableImage& resource, const AdditionalDescriptorFlags flags) = 0;

    /** Bind sampler with additional descriptor flags
     * @param binding Binding index
     * @param resource Binding resource
     * @param flags Additional decriptor flags
     */
    virtual void BindSampler(
        const uint32_t binding, const BindableSampler& resource, const AdditionalDescriptorFlags flags) = 0;

    /** Print validity. Checks through all bindings that they have valid handles.
     */
    virtual void PrintDescriptorSetLayoutBindingValidation() const = 0;

    struct Deleter {
        constexpr Deleter() noexcept = default;
        void operator()(IDescriptorSetBinder* ptr) const
        {
            ptr->Destroy();
        }
    };
    using Ptr = BASE_NS::unique_ptr<IDescriptorSetBinder, Deleter>;

protected:
    IDescriptorSetBinder() = default;
    virtual ~IDescriptorSetBinder() = default;
    virtual void Destroy() = 0;
};

/** @ingroup group_render_ipipelinedescriptorsetbinder */
/** Pipeline descriptor set binder interface class */
class IPipelineDescriptorSetBinder {
public:
    IPipelineDescriptorSetBinder(const IPipelineDescriptorSetBinder&) = delete;
    IPipelineDescriptorSetBinder& operator=(const IPipelineDescriptorSetBinder&) = delete;

    /** Clear bind handles. This should be used to reset if the same binder is used in many frames.
     */
    virtual void ClearBindings() = 0;

    /** Get descriptor set handle
     * @param set Set handle
     * @return Descriptor set handle
     */
    virtual RenderHandle GetDescriptorSetHandle(const uint32_t set) const = 0;

    /** Get descriptor set layout binding resources
     * @param set Set index
     * @return Descriptor set layout binding resources
     */
    virtual DescriptorSetLayoutBindingResources GetDescriptorSetLayoutBindingResources(const uint32_t set) const = 0;

    /** Get pipeline descriptor set layout binding validity. Checks through all sets and their bindings that they have
     * valid handles. When the validity of the bindings might be up-to API user, this should be called.
     */
    virtual bool GetPipelineDescriptorSetLayoutBindingValidity() const = 0;

    /** Get descriptor set count
     * @return Descriptor set count
     */
    virtual uint32_t GetDescriptorSetCount() const = 0;

    /** Get first set
     * @return First descriptor set index
     */
    virtual uint32_t GetFirstSet() const = 0;

    /** Get array of set indices
     * @return All descriptor set indices
     */
    virtual BASE_NS::array_view<const uint32_t> GetSetIndices() const = 0;

    /** Get array of descriptor set handles
     * @return All descriptor set handles
     */
    virtual BASE_NS::array_view<const RenderHandle> GetDescriptorSetHandles() const = 0;

    /** Get contiguous range of descriptor set handles for binding
     * @param beginSet Index of the first set
     * @param count Number of sets forward from beginSet
     * @return Specific descriptor set handles
     */
    virtual BASE_NS::array_view<const RenderHandle> GetDescriptorSetHandles(
        const uint32_t beginSet, const uint32_t count) const = 0;

    /** Bind buffer
     * @param set Set index
     * @param binding Binding index
     * @param resource Binding resource
     */
    virtual void BindBuffer(const uint32_t set, const uint32_t binding, const BindableBuffer& resource) = 0;

    /** Bind buffers to descriptor array.
     * Input array_view counts must match. Bind is only valid if (handles.size == byteOffsets.size() ==
     * byteSizes.size())
     * @param set Set index
     * @param binding Binding index
     * @param resources Binding resources
     */
    virtual void BindBuffers(
        const uint32_t set, const uint32_t binding, const BASE_NS::array_view<const BindableBuffer> resources) = 0;

    /** Bind image. (Sampled Image or Storage Image)
     * (e.g. input attachment and color attachment)
     * @param set Set index
     * @param binding Binding index
     * @param resource Binding resource
     */
    virtual void BindImage(const uint32_t set, const uint32_t binding, const BindableImage& resource) = 0;

    /** Bind images to descriptor array
     * @param set Set index
     * @param binding Binding index
     * @param resources Binding resources
     */
    virtual void BindImages(
        const uint32_t set, const uint32_t binding, const BASE_NS::array_view<const BindableImage> resources) = 0;

    /** Bind sampler
     * @param set Set index
     * @param binding Binding index
     * @param resource Binding resource
     */
    virtual void BindSampler(const uint32_t set, const uint32_t binding, const BindableSampler& resource) = 0;

    /** Bind samplers to descriptor array
     * @param set Set index
     * @param binding Binding index
     * @param resources Binding resources
     */
    virtual void BindSamplers(
        const uint32_t set, const uint32_t binding, const BASE_NS::array_view<const BindableSampler> resources) = 0;

    /** Bind buffer with additional descriptor set flags
     * @param set Set index
     * @param binding Binding index
     * @param resource Binding resource
     * @param flags Additional descriptor set flags
     */
    virtual void BindBuffer(const uint32_t set, const uint32_t binding, const BindableBuffer& resource,
        const AdditionalDescriptorFlags flags) = 0;

    /** Bind image with additional descriptor set flags. (Sampled Image or Storage Image)
     * (e.g. input attachment and color attachment)
     * @param set Set index
     * @param binding Binding index
     * @param resource Binding resource
     * @param flags Additional descriptor set flags
     */
    virtual void BindImage(const uint32_t set, const uint32_t binding, const BindableImage& resource,
        const AdditionalDescriptorFlags flags) = 0;

    /** Bind sampler with additional descriptor set flags
     * @param set Set index
     * @param binding Binding index
     * @param resource Binding resource
     * @param flags Additional descriptor set flags
     */
    virtual void BindSampler(const uint32_t set, const uint32_t binding, const BindableSampler& resource,
        const AdditionalDescriptorFlags flags) = 0;

    /** Print validity. Checks through all set bindings that they have valid handles and prints errors.
     */
    virtual void PrintPipelineDescriptorSetLayoutBindingValidation() const = 0;

    struct Deleter {
        constexpr Deleter() noexcept = default;
        void operator()(IPipelineDescriptorSetBinder* ptr) const
        {
            ptr->Destroy();
        }
    };
    using Ptr = BASE_NS::unique_ptr<IPipelineDescriptorSetBinder, Deleter>;

protected:
    IPipelineDescriptorSetBinder() = default;
    virtual ~IPipelineDescriptorSetBinder() = default;
    virtual void Destroy() = 0;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_IPIPELINE_DESCRIPTORSET_BINDER_H
