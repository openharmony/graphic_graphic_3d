/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef API_RENDER_DEVICE_IGPU_RESOURCE_MANAGER_H
#define API_RENDER_DEVICE_IGPU_RESOURCE_MANAGER_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <core/image/intf_image_container.h>
#include <render/device/gpu_resource_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
/** \addtogroup group_igpuresourcemanager
 *  @{
 */
/** Backend specific image descriptor */
struct BackendSpecificImageDesc {};

/** Backend specific buffer descriptor */
struct BackendSpecificBufferDesc {};

class IGpuResourceCache;

/** Gpu resource manager.
 *  Internally synchronized.
 *  (except WaitForIdleAndDestroyGpuResources, MapBuffer, and UnmapBuffer are not internally synchronized)
 *
 *  Handles waiting of gpu resource destruction internally.
 *
 *  Create methods take a unique name (if named).
 *  Object is re-created if the name is already in use.
 *  Therefore do not use scoped handles if you're recreating them.
 *  Create method with a given GpuResourceHandle will replace the handle and return the same valid handle.
 */
class IGpuResourceManager {
public:
    IGpuResourceManager(const IGpuResourceManager&) = delete;
    IGpuResourceManager& operator=(const IGpuResourceManager&) = delete;

    /** Get render handle reference of raw handle.
     *  @param handle Raw render handle
     *  @return Returns A render handle reference for the handle.
     */
    virtual RenderHandleReference Get(const RenderHandle& handle) const = 0;

    /** Create a GpuBuffer with unique buffer name.
     *  @param name Name of buffer
     *  @param desc Descriptor
     *  @return Returns A valid resource handle if the creation was successfull.
     */
    virtual RenderHandleReference Create(const BASE_NS::string_view name, const GpuBufferDesc& desc) = 0;

    /** Create a GpuBuffer with unique buffer name and data.
     *  @param name Name of buffer
     *  @param desc Descriptor
     *  @param data Gpu buffer data
     *  @return Returns A valid resource handle if the creation was successfull.
     */
    virtual RenderHandleReference Create(
        const BASE_NS::string_view name, const GpuBufferDesc& desc, const BASE_NS::array_view<const uint8_t> data) = 0;

    /** Create an unnamed GpuBuffer with data.
     *  @param desc Descriptor
     *  @param data Gpu buffer data
     *  @return Returns A valid resource handle if the creation was successfull.
     */
    virtual RenderHandleReference Create(const GpuBufferDesc& desc, const BASE_NS::array_view<const uint8_t> data) = 0;

    /** Create a new GPU resource for a given GpuResourceHandle. (Old handle and name (if given) are valid)
     *  @param replacedHandle A valid handle which current resource will be destroyed and replaced with a new one.
     *  @param desc Descriptor
     *  @return Returns the same handle that was given if the resource handle was valid.
     */
    virtual RenderHandleReference Create(const RenderHandleReference& replacedHandle, const GpuBufferDesc& desc) = 0;

    /** Create an unnamed GpuBuffer.
     *  @param desc Descriptor
     *  @return Returns A valid resource handle if the creation was successfull.
     */
    virtual RenderHandleReference Create(const GpuBufferDesc& desc) = 0;

    /** Create a GpuImage with unique image name.
     *  @param name Name of image
     *  @param desc Descriptor
     *  @return Returns A valid resource handle if the creation was successfull.
     */
    virtual RenderHandleReference Create(const BASE_NS::string_view name, const GpuImageDesc& desc) = 0;

    /** Create a GpuImage with unique image name and data.
     *  @param name Name of image
     *  @param desc Descriptor
     *  @param data Gpu image data
     *  @return Returns A valid resource handle if the creation was successfull.
     */
    virtual RenderHandleReference Create(
        const BASE_NS::string_view name, const GpuImageDesc& desc, const BASE_NS::array_view<const uint8_t> data) = 0;

    /** Create a GpuImage with unique image name and data with image copy description.
     *  @param name Name of image
     *  @param desc Descriptor
     *  @param data Gpu image data
     *  @param bufferImageCopies Array of buffer image copies
     *  @return Returns A valid resource handle if the creation was successfull.
     */
    virtual RenderHandleReference Create(const BASE_NS::string_view name, const GpuImageDesc& desc,
        const BASE_NS::array_view<const uint8_t> data,
        const BASE_NS::array_view<const BufferImageCopy> bufferImageCopies) = 0;

    /** Create a new GPU resource for a given GpuResourceHandle. (Old handle and name (if given) are valid)
     *  @param replacedHandle A valid handle which current resource will be destroyed and replaced with a new one.
     *  @param desc Descriptor
     *  @return Returns the same handle that was given if the resource handle was valid.
     */
    virtual RenderHandleReference Create(const RenderHandleReference& replacedHandle, const GpuImageDesc& desc) = 0;

    /** Create an unnamed GpuImage.
     *  @param desc Descriptor
     *  @return Returns A valid resource handle if the creation was successfull.
     */
    virtual RenderHandleReference Create(const GpuImageDesc& desc) = 0;

    /** Create an unnamed GpuImage with data.
     *  @param desc Descriptor
     *  @param data Gpu image data
     *  @return Returns A valid resource handle if the creation was successfull.
     */
    virtual RenderHandleReference Create(const GpuImageDesc& desc, const BASE_NS::array_view<const uint8_t> data) = 0;

    /** Create an unnamed GpuImage with data and image copy description.
     *  @param desc Descriptor
     *  @param data Gpu image data
     *  @param bufferImageCopies Array of buffer image copies
     *  @return Returns A valid resource handle if the creation was successfull.
     */
    virtual RenderHandleReference Create(const GpuImageDesc& desc, const BASE_NS::array_view<const uint8_t> data,
        const BASE_NS::array_view<const BufferImageCopy> bufferImageCopies) = 0;

    /** Create GpuImage with unique image name from IImageContainer::Ptr
     *  @param name Name of image
     *  @param desc Descriptor
     *  @param image Image container
     *  @return Returns A valid resource handle if the creation was successfull.
     */
    virtual RenderHandleReference Create(
        const BASE_NS::string_view name, const GpuImageDesc& desc, CORE_NS::IImageContainer::Ptr image) = 0;

    /** Create Unnamed GpuImage from IImageContainer::Ptr
     *  @param desc Descriptor
     *  @param image Image container
     *  @return Returns A valid resource handle if the creation was successfull.
     */
    virtual RenderHandleReference Create(const GpuImageDesc& desc, CORE_NS::IImageContainer::Ptr image) = 0;

    /** Create a GpuImage with unique image name from external image resource
     * NOTE: the external image resource is not owned and not deleted by the manager when the handle is destroyed
     * (e.g. if using hw buffers the hw buffer reference is released)
     *  @param name Name of image
     *  @param desc Descriptor
     *  @param backendSpecificData Image description
     *  @return Returns A valid resource handle if the creation was successfull.
     */
    virtual RenderHandleReference CreateView(const BASE_NS::string_view name, const GpuImageDesc& desc,
        const BackendSpecificImageDesc& backendSpecificData) = 0;

    /** Create a GpuSampler with unique image name.
     *  @param name Name of image
     *  @param desc Descriptor
     *  @return Returns A valid resource handle if the creation was successfull.
     */
    virtual RenderHandleReference Create(const BASE_NS::string_view name, const GpuSamplerDesc& desc) = 0;

    /** Create a GpuSampler with replaced handle.
     *  @param replacedHandle Replaced handle
     *  @param desc Descriptor
     *  @return Returns A valid resource handle if the creation was successfull.
     */
    virtual RenderHandleReference Create(const RenderHandleReference& replacedHandle, const GpuSamplerDesc& desc) = 0;

    /** Create unnamed GpuSampler.
     *  @param desc Descriptor
     *  @return Returns A valid resource handle if the creation was successfull.
     */
    virtual RenderHandleReference Create(const GpuSamplerDesc& desc) = 0;

    /** Create unnamed GpuAccelerationStructure. A GPU buffer handle is returned with additional acceleration structure.
     *  @param desc Descriptor
     *  @return Returns A valid resource handle if the creation was successfull.
     */
    virtual RenderHandleReference Create(const GpuAccelerationStructureDesc& desc) = 0;

    /** Create uniquely named GpuAccelerationStructure. If name is found, the handle is replaced
     *  A GPU buffer handle is returned with additional acceleration structure.
     *  @param name Unique name for the acceleration structure
     *  @param desc Descriptor
     *  @return Returns A valid resource handle if the creation was successfull.
     */
    virtual RenderHandleReference Create(const BASE_NS::string_view name, const GpuAccelerationStructureDesc& desc) = 0;

    /** Create GpuAccelerationStructure and replace the given handle if it's valid (the same is returned in valid case).
     *  A GPU buffer handle is returned with additional acceleration structure.
     *  @param replacedHandle A valid handle which current resource will be destroyed and replaced with a new one.
     *  @param desc Descriptor
     *  @return Returns A valid resource handle if the creation was successfull.
     */
    virtual RenderHandleReference Create(
        const RenderHandleReference& replacedHandle, const GpuAccelerationStructureDesc& desc) = 0;

    /** Get buffer handle. (Invalid handle if not found.)
     *  @param name Name of buffer
     *  @return Returns A valid resource handle if the named resource is available.
     */
    virtual RenderHandleReference GetBufferHandle(const BASE_NS::string_view name) const = 0;

    /** Get image handle. (Invalid handle if not found.)
     *  @param name Name of image
     *  @return Returns A valid resource handle if the named resource is available.
     */
    virtual RenderHandleReference GetImageHandle(const BASE_NS::string_view name) const = 0;

    /** Get sampler handle. (Invalid handle if not found.)
     *  @param name Name of sampler
     *  @return Returns A valid resource handle if the named resource is available.
     */
    virtual RenderHandleReference GetSamplerHandle(const BASE_NS::string_view name) const = 0;

    /** Get buffer descriptor
     *  @param handle Handle to resource
     *  @return Returns A GpuBufferDesc of a given GPU resource handle.
     */
    virtual GpuBufferDesc GetBufferDescriptor(const RenderHandleReference& handle) const = 0;

    /** Get image descriptor
     *  @param handle Handle to resource
     *  @return Returns A GpuImageDesc of a given GPU resource handle.
     */
    virtual GpuImageDesc GetImageDescriptor(const RenderHandleReference& handle) const = 0;

    /** Get sampler descriptor
     *  @param handle Handle to resource
     *  @return Returns A GpuSamplerDesc of a given GPU resource handle.
     */
    virtual GpuSamplerDesc GetSamplerDescriptor(const RenderHandleReference& handle) const = 0;

    /** Get acceleration structure descriptor
     *  @param handle Handle to resource
     *  @return Returns A GpuAccelerationStructureDesc of a given GPU resource handle.
     */
    virtual GpuAccelerationStructureDesc GetAccelerationStructureDescriptor(
        const RenderHandleReference& handle) const = 0;

    /** Wait for Gpu to become idle and destroy all Gpu resources of destroyed handles.
     *  Not internally syncronized. Needs to be called from render thread where renderFrame is called.
     *
     *  This is a hazardous method and should be called with extreme caution.
     *  In normal rendering situation RenderFrame() does the resource management automatically.
     *  This method is provided for forced destruction of Gpu resurces when RenderFrame() is not called.
     *
     *  Do not call this method per frame.
     */
    virtual void WaitForIdleAndDestroyGpuResources() = 0;

    /** Map buffer memory. Always maps from the beginning of buffer.
     *  Preferrably use only for staging kind of data (e.g. fill data in another thread and then pass to staging).
     *  1. Map once and get the pointer to memory
     *  2. Write data to memory
     *  3. Unmap and pass the handle to rendering/staging or whatever (with offset)
     *  The buffer needs to created with CORE_ENGINE_BUFFER_CREATION_MAP_OUTSIDE_RENDERER,
     *  CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE and CORE_ENGINE_BUFFER_CREATION_DEFERRED_DESTROY and cannot be
     *  replaced with name or handle.
     *  @param handle Handle to resource
     */
    virtual void* MapBufferMemory(const RenderHandleReference& handle) const = 0;

    /** Unmap buffer
     *  @param handle Handle to resource
     */
    virtual void UnmapBuffer(const RenderHandleReference& handle) const = 0;

    /** Checks if resource is a GPU buffer */
    virtual bool IsGpuBuffer(const RenderHandleReference& handle) const = 0;
    /** Checks if resource is a GPU image */
    virtual bool IsGpuImage(const RenderHandleReference& handle) const = 0;
    /** Checks if resource is a GPU sampler */
    virtual bool IsGpuSampler(const RenderHandleReference& handle) const = 0;
    /** Checks if resource is a GPU acceleration structure. If is GPU acceleration structure, it is GPU buffer as well.
     */
    virtual bool IsGpuAccelerationStructure(const RenderHandleReference& handle) const = 0;
    /** Image has been created to be used as a swapchain image. Fast check based on handle.
     * Due to possible remapping with built-in CORE_DEFAULT_BACKBUFFER might not be an actual swapchain.
     */
    virtual bool IsSwapchain(const RenderHandleReference& handle) const = 0;

    /** Checks if resource is a client mappable */
    virtual bool IsMappableOutsideRender(const RenderHandleReference& handle) const = 0;

    /** Returns supported flags for given resource handle format. */
    virtual FormatProperties GetFormatProperties(const RenderHandleReference& handle) const = 0;
    /** Returns supported flags for given format. */
    virtual FormatProperties GetFormatProperties(const BASE_NS::Format format) const = 0;

    /** Returns GpuImageDesc based on image container's ImageDesc.
     * Conversion helper when loading images with image loaders.
     * Sets automatically basic values which are needed for typical image usage:
     * imageTiling = CORE_IMAGE_TILING_OPTIMAL
     * usageFlags |= CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT
     * memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
     * if mips are requested adds mips related flags
     * @param desc ImageDesc from IImageContainer.
     * @return GpuImageDesc GPU image desc based on input.
     */
    virtual GpuImageDesc CreateGpuImageDesc(const CORE_NS::IImageContainer::ImageDesc& desc) const = 0;

    /** Returns name for the resource "" if no name found.
     * NOTE: method should not be used during run-time on hot path.
     * @param handle Handle of the resource.
     * @return string Name of the resource.
     */
    virtual BASE_NS::string GetName(const RenderHandleReference& handle) const = 0;

    /** Returns all valid GPU buffer render handles.
     * @return vector of handles.
     */
    virtual BASE_NS::vector<RenderHandleReference> GetGpuBufferHandles() const = 0;
    /** Returns all valid GPU image render handles.
     * @return vector of handles.
     */
    virtual BASE_NS::vector<RenderHandleReference> GetGpuImageHandles() const = 0;
    /** Returns all valid GPU sampler render handles.
     * @return vector of handles.
     */
    virtual BASE_NS::vector<RenderHandleReference> GetGpuSamplerHandles() const = 0;

    /** Force default GPU buffer creation usage flags. Used as bitwise OR with given descs.
     * NOTE: Reduced performance expected
     * Use only in situtation when necessary and not in real products.
     * @param bufferUsageFlags Default buffer usage flags. With zero no flags (default)
     */
    virtual void SetDefaultGpuBufferCreationFlags(const BufferUsageFlags usageFlags) = 0;
    /** Force default GPU image creation usage flags. Used as bitwise OR with given descs.
     * NOTE: Reduced performance expected
     * Use only in situtation when necessary and not in real products.
     * @param bufferUsageFlags Default buffer usage flags. With zero no flags (default)
     */
    virtual void SetDefaultGpuImageCreationFlags(const ImageUsageFlags usageFlags) = 0;

    /** Get gpu resource cache.
     * @return Reference to GPU resource cache interface.
     */
    virtual IGpuResourceCache& GetGpuResourceCache() const = 0;

    /** Get image aspect flags for a given resource handle format.
     * @param handle Render handle reference.
     * @return Image aspect flags for a given format.
     */
    virtual ImageAspectFlags GetImageAspectFlags(const RenderHandleReference& handle) const = 0;

    /** Get image aspect flags for a given format.
     * @param format Format.
     * @return Image aspect flags for a given format.
     */
    virtual ImageAspectFlags GetImageAspectFlags(const BASE_NS::Format format) const = 0;

protected:
    IGpuResourceManager() = default;
    virtual ~IGpuResourceManager() = default;
};

/** IRenderNodeGpuResourceManager.
 *  Gpu resource manager interface to be used inside render nodes.
 *  Has management for per render node graph resources.
 *  All resources, created through this interface, are automatically tracked
 *  and destroyed when the render node graph which uses this render node is destroyed.
 *
 *  Create methods take a unique name (if named).
 *  Object is re-created if the name is already in use.
 *  Therefore do not use scoped handles if you're recreating them.
 */
class IRenderNodeGpuResourceManager {
public:
    /** Get render handle reference of raw handle.
     *  @param handle Raw render handle
     *  @return Returns A render handle reference for the handle.
     */
    virtual RenderHandleReference Get(const RenderHandle& handle) const = 0;

    /** Create a GpuBuffer.
     *  @param desc Descriptor
     */
    virtual RenderHandleReference Create(const GpuBufferDesc& desc) = 0;

    /** Create a GpuBuffer with unique buffer name.
     *  @param name Name used for creation
     *  @param desc Descriptor
     */
    virtual RenderHandleReference Create(const BASE_NS::string_view name, const GpuBufferDesc& desc) = 0;

    /** Create a GpuBuffer with replaced handle.
     *  @param handle Replaced handle
     *  @param desc Descriptor
     */
    virtual RenderHandleReference Create(const RenderHandleReference& handle, const GpuBufferDesc& desc) = 0;

    /** Create a GpuBuffer with unique buffer name and data.
     *  @param name Name for buffer
     *  @param desc Descriptor
     *  @param data Buffer data
     */
    virtual RenderHandleReference Create(
        const BASE_NS::string_view name, const GpuBufferDesc& desc, const BASE_NS::array_view<const uint8_t> data) = 0;

    /** Create a GpuImage.
     *  @param desc Descriptor
     */
    virtual RenderHandleReference Create(const GpuImageDesc& desc) = 0;

    /** Create a GpuImage with unique image name.
     *  @param name Name for image
     *  @param desc Descriptor
     */
    virtual RenderHandleReference Create(const BASE_NS::string_view name, const GpuImageDesc& desc) = 0;

    /** Create a GpuImage with replaced handle.
     *  @param handle Replaced handle
     *  @param desc Descriptor
     */
    virtual RenderHandleReference Create(const RenderHandleReference& handle, const GpuImageDesc& desc) = 0;

    /** Create a GpuImage with unique image name and data.
     *  @param name Name for image
     *  @param desc Descriptor
     *  @param data Image buffer data
     */
    virtual RenderHandleReference Create(
        const BASE_NS::string_view name, const GpuImageDesc& desc, const BASE_NS::array_view<const uint8_t> data) = 0;

    /** Create a GpuSampler.
     *  @param desc Descriptor
     */
    virtual RenderHandleReference Create(const GpuSamplerDesc& desc) = 0;

    /** Create a GpuSampler with unique sampler name.
     *  @param name Name for sampler
     *  @param desc Descriptor
     */
    virtual RenderHandleReference Create(const BASE_NS::string_view name, const GpuSamplerDesc& desc) = 0;

    /** Create a GpuSampler with replaced handle.
     *  @param handle Replaced handle
     *  @param desc Descriptor
     */
    virtual RenderHandleReference Create(const RenderHandleReference& handle, const GpuSamplerDesc& desc) = 0;

    /** Create a GpuAccelerationStructureDesc.
     *  @param desc Descriptor
     */
    virtual RenderHandleReference Create(const GpuAccelerationStructureDesc& desc) = 0;

    /** Create a GpuAccelerationStructureDesc with unique name.  A GPU buffer handle is returned with additional
     * acceleration structure.
     *  @param name Name for acceleration structure.
     *  @param desc Descriptor
     */
    virtual RenderHandleReference Create(const BASE_NS::string_view name, const GpuAccelerationStructureDesc& desc) = 0;

    /** Create a GpuAccelerationStructureDesc with replaced handle. A GPU buffer handle is returned with additional
     * acceleration structure.
     *  @param handle Replaced handle
     *  @param desc Descriptor
     */
    virtual RenderHandleReference Create(
        const RenderHandleReference& handle, const GpuAccelerationStructureDesc& desc) = 0;

    /** Get buffer handle
     *  @param name Name of buffer
     */
    virtual RenderHandle GetBufferHandle(const BASE_NS::string_view name) const = 0;

    /** Get image handle
     *  @param name Name of image
     */
    virtual RenderHandle GetImageHandle(const BASE_NS::string_view name) const = 0;

    /** Get sampler handle
     *  @param name Name of sampler
     */
    virtual RenderHandle GetSamplerHandle(const BASE_NS::string_view name) const = 0;

    /** Get buffer descriptor
     *  @param handle Handle to resource
     */
    virtual GpuBufferDesc GetBufferDescriptor(const RenderHandle& handle) const = 0;

    /** Get image descriptor
     *  @param handle Handle to resource
     */
    virtual GpuImageDesc GetImageDescriptor(const RenderHandle& handle) const = 0;

    /** Get sampler descriptor
     *  @param handle Handle to resource
     */
    virtual GpuSamplerDesc GetSamplerDescriptor(const RenderHandle& handle) const = 0;

    /** Get acceleration structure descriptor
     *  @param handle Handle to resource
     */
    virtual GpuAccelerationStructureDesc GetAccelerationStructureDescriptor(const RenderHandle& handle) const = 0;

    /** Map buffer, Only available in render nodes.
     *  Automatically advances buffered dynamic ring buffer offset.
     *  @param handle Handle to resource
     */
    virtual void* MapBuffer(const RenderHandle& handle) const = 0;

    /** Map buffer memory, Only available in render nodes.
     *  Always maps from the beginning of buffer.
     *  @param handle Handle to resource
     */
    virtual void* MapBufferMemory(const RenderHandle& handle) const = 0;

    /** Unmap buffer
     *  @param handle Handle to resource
     */
    virtual void UnmapBuffer(const RenderHandle& handle) const = 0;

    /** Checks validity of a handle */
    virtual bool IsValid(const RenderHandle& handle) const = 0;
    /** Checks if resource is a GPU buffer */
    virtual bool IsGpuBuffer(const RenderHandle& handle) const = 0;
    /** Checks if resource is a GPU image */
    virtual bool IsGpuImage(const RenderHandle& handle) const = 0;
    /** Checks if resource is a GPU sampler */
    virtual bool IsGpuSampler(const RenderHandle& handle) const = 0;
    /** Checks if resource is a GPU acceleration structure. If is GPU acceleration structure, it is GPU buffer as
     * well. */
    virtual bool IsGpuAccelerationStructure(const RenderHandle& handle) const = 0;
    /** Image has been created to be used as a swapchain image. Fast check based on handle.
     * Due to possible remapping with built-in CORE_DEFAULT_BACKBUFFER might not be an actual swapchain.
     */
    virtual bool IsSwapchain(const RenderHandle& handle) const = 0;

    /** Returns supported flags for given resource handle format. */
    virtual FormatProperties GetFormatProperties(const RenderHandle& handle) const = 0;
    /** Returns supported flags for given format. */
    virtual FormatProperties GetFormatProperties(const BASE_NS::Format format) const = 0;

    /** Returns name for the resource "" if no name found.
     * NOTE: method should not be used during run-time on hot path.
     * @param handle Handle of the resource.
     * @return string Name of the resource.
     */
    virtual BASE_NS::string GetName(const RenderHandle& handle) const = 0;

    /** Get gpu resource cache.
     * @return Reference to GPU resource cache interface.
     */
    virtual IGpuResourceCache& GetGpuResourceCache() const = 0;

    /** Get image aspect flags for given format.
     * @param handle Render handle of the resource
     * @return Image aspect flags for given format.
     */
    virtual ImageAspectFlags GetImageAspectFlags(const RenderHandle& handle) const = 0;

    /** Get image aspect flags for given format.
     * @param format Format.
     * @return Image aspect flags for given format.
     */
    virtual ImageAspectFlags GetImageAspectFlags(const BASE_NS::Format format) const = 0;

protected:
    IRenderNodeGpuResourceManager() = default;
    virtual ~IRenderNodeGpuResourceManager() = default;
};
/** @} */
RENDER_END_NAMESPACE()

#endif // API_RENDER_DEVICE_IGPU_RESOURCE_MANAGER_H
