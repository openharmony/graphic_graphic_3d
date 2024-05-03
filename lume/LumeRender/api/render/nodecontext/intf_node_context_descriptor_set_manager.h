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

#ifndef API_RENDER_INODE_CONTEXT_DESCRIPTORSET_MANAGER_H
#define API_RENDER_INODE_CONTEXT_DESCRIPTORSET_MANAGER_H

#include <cstddef>

#include <base/containers/array_view.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
class Device;
struct PipelineLayout;

/** @ingroup group_render_inodecontextdescriptorsetmanager */
/**
 * INodeContextDescriptorSetManager
 * Creates and caches descriptor sets.
 *
 * Single descriptor set can only be updated once per frame. I.e you need to create as many descriptor sets as you need
 * per frame. (Descriptor sets are internally buffered for GPU frames).
 *
 * One should use normal descriptor sets and use ResetAndReserve when one needs more sets.
 *
 * There is a possiblity to use one frame descriptor sets which do not need reserving.
 * The methods CreateOneFrameXXX creates a descriptor set which is valid for current frame.
 * This could be used with user inputs where one cannot pre-calculate the need of descriptors properly.
 */
class INodeContextDescriptorSetManager {
public:
    INodeContextDescriptorSetManager(const INodeContextDescriptorSetManager&) = delete;
    INodeContextDescriptorSetManager& operator=(const INodeContextDescriptorSetManager&) = delete;

    /** Reset non-dynamic (not one frame) descriptor sets and reserve descriptors.
     * Reset only when a new pool is needed. (Currently allocated descriptors are not enough)
     * Needs to reserve all the descriptors which are used in a single frame.
     * To reiterate: All old descriptor set handles become invalid when this is called. */
    virtual void ResetAndReserve(const DescriptorCounts& descriptorCounts) = 0;

    /** Reset non-dynamic (not one frame) descriptor sets and reserve descriptors.
     * Reset only when a new pool is needed. (Currently allocated descriptors are not enough)
     * Needs to reserve all the descriptors which are used in a single frame.
     * To reiterate: All old descriptor set handles become invalid when this is called. */
    virtual void ResetAndReserve(const BASE_NS::array_view<DescriptorCounts> descriptorCounts) = 0;

    /** Creates a new descriptor set and gives its handle. */
    virtual RenderHandle CreateDescriptorSet(
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) = 0;
    /** Creates new descriptor sets for all given descriptor sets. */
    virtual BASE_NS::vector<RenderHandle> CreateDescriptorSets(
        const BASE_NS::array_view<const DescriptorSetLayoutBindings> descriptorSetsLayoutBindings) = 0;
    /** Creates a single descriptor set from pipeline layout. */
    virtual RenderHandle CreateDescriptorSet(const uint32_t set, const PipelineLayout& pipelineLayout) = 0;

    /** Create a IDescriptorSetBinder based on a descriptor set handle and layout bindings.
     * Layout bindings are expected to be sorted starting from the smallest index.
     */
    virtual IDescriptorSetBinder::Ptr CreateDescriptorSetBinder(const RenderHandle handle,
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) = 0;

    /** Create a IPipelineDescriptorSetBinder based on pipeline layout.
     * @param pipelineLayout PipelineLayout
     */
    virtual IPipelineDescriptorSetBinder::Ptr CreatePipelineDescriptorSetBinder(
        const PipelineLayout& pipelineLayout) = 0;

    /** Create a IPipelineDescriptorSetBinder based on pipeline layout, handles, and layout bindings.
     * Pipeline layout sets and their bindings are expected to be sorted starting from smallest index.
     * @param pipelineLayout PipelineLayout
     * @param handles Descriptor set handles
     * @param descriptorSetsLayoutBindings Descriptor set layout bindings
     */
    virtual IPipelineDescriptorSetBinder::Ptr CreatePipelineDescriptorSetBinder(const PipelineLayout& pipelineLayout,
        const BASE_NS::array_view<const RenderHandle> handles,
        const BASE_NS::array_view<const DescriptorSetLayoutBindings> descriptorSetsLayoutBindings) = 0;

    /** Creates a one frame new descriptor set and gives its handle. Valid for a one frame and does not need reserve. */
    virtual RenderHandle CreateOneFrameDescriptorSet(
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) = 0;
    /** Creates a one frame new dynamic descriptor sets for all given descriptor sets. Valid for a one frame and does
     * not need reserve. */
    virtual BASE_NS::vector<RenderHandle> CreateOneFrameDescriptorSets(
        const BASE_NS::array_view<const DescriptorSetLayoutBindings> descriptorSetsLayoutBindings) = 0;
    /** Creates a one frame dynamic single descriptor set from pipeline layout. Valid for a one frame and does not need
     * reserve. */
    virtual RenderHandle CreateOneFrameDescriptorSet(const uint32_t set, const PipelineLayout& pipelineLayout) = 0;

protected:
    INodeContextDescriptorSetManager() = default;
    virtual ~INodeContextDescriptorSetManager() = default;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_INODE_CONTEXT_DESCRIPTORSET_MANAGER_H
