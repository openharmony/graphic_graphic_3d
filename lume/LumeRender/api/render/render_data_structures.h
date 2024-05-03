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

#ifndef API_RENDER_RENDER_DATA_STRUCTURES_H
#define API_RENDER_RENDER_DATA_STRUCTURES_H

#include <cstdint>

#include <base/containers/fixed_string.h>
#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <base/util/formats.h>
#include <render/device/gpu_resource_desc.h>
#include <render/device/intf_device.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
/** \addtogroup group_render_renderdatastructures
 *  @{
 */
/** Render data constants */
namespace RenderDataConstants {
static constexpr uint32_t MAX_DEFAULT_NAME_LENGTH { 128 };
using RenderDataFixedString = BASE_NS::fixed_string<RenderDataConstants::MAX_DEFAULT_NAME_LENGTH>;
} // namespace RenderDataConstants

/** Handle to gpu buffer.
    Byte offset to buffer.
    Byte size for vertex buffer.
*/
struct VertexBuffer {
    /** Buffer handle */
    RenderHandle bufferHandle;
    /** Buffer offset */
    uint32_t bufferOffset { 0 };
    /** Byte size */
    uint32_t byteSize { PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE };
};

/** Index buffer */
struct IndexBuffer {
    /** Buffer handle */
    RenderHandle bufferHandle;
    /** Buffer offset */
    uint32_t bufferOffset { 0 };
    /** Byte size */
    uint32_t byteSize { PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE };
    /** Index type */
    IndexType indexType { IndexType::CORE_INDEX_TYPE_UINT32 };
};

/** Vertex buffer with render handle reference */
struct VertexBufferWithHandleReference {
    /** Buffer handle */
    RenderHandleReference bufferHandle;
    /** Buffer offset */
    uint32_t bufferOffset { 0 };
    /** Byte size */
    uint32_t byteSize { PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE };
};

/** Index buffer with render handle reference */
struct IndexBufferWithHandleReference {
    /** Buffer handle */
    RenderHandleReference bufferHandle;
    /** Buffer offset */
    uint32_t bufferOffset { 0 };
    /** Byte size */
    uint32_t byteSize { PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE };
    /** Index type */
    IndexType indexType { IndexType::CORE_INDEX_TYPE_UINT32 };
};

/** Helper struct for descriptor types and their counts.
 */
struct DescriptorCounts {
    /** Typed count */
    struct TypedCount {
        /** Type */
        DescriptorType type { DescriptorType::CORE_DESCRIPTOR_TYPE_MAX_ENUM };
        /** Count */
        uint32_t count { 0u };
    };

    /** Counts list */
    BASE_NS::vector<TypedCount> counts;
};

/** Acceleration AABB */
struct AabbPositions {
    /** Min x */
    float minX { 0.0f };
    /** Min y */
    float minY { 0.0f };
    /** Min z */
    float minZ { 0.0f };
    /** Max x */
    float maxX { 0.0f };
    /** Max y */
    float maxY { 0.0f };
    /** Max z */
    float maxZ { 0.0f };
};

/** Render slot sort type */
enum class RenderSlotSortType : uint32_t {
    /** Node */
    NONE = 0,
    /** Front to back */
    FRONT_TO_BACK = 1,
    /** Back to front */
    BACK_TO_FRONT = 2,
    /** By material */
    BY_MATERIAL = 3,
};

/** Render slot cull type */
enum class RenderSlotCullType : uint32_t {
    /** None */
    NONE = 0,
    /** View frustum cull */
    VIEW_FRUSTUM_CULL = 1,
};

/** Render node graph resource type */
enum class RenderNodeGraphResourceLocationType : uint32_t {
    /** Default, fetch with name */
    DEFAULT = 0,
    /** Get with input index from render node graph share */
    FROM_RENDER_GRAPH_INPUT = 1,
    /** Get with output index from render node graph share */
    FROM_RENDER_GRAPH_OUTPUT = 2,
    /** Get output index from previous render node */
    FROM_PREVIOUS_RENDER_NODE_OUTPUT = 3,
    /** Get output index from named render node */
    FROM_NAMED_RENDER_NODE_OUTPUT = 4,
    /** Get output from the previous render node graph */
    FROM_PREVIOUS_RENDER_NODE_GRAPH_OUTPUT = 5,
};

/** Set for default command buffer recording.
    Requested queue type and index.
*/
struct ContextInitDescription {
    /** Requested queue */
    GpuQueue requestedQueue;
};

/** Render node resource */
struct RenderNodeResource {
    /** Set of this resource */
    uint32_t set { ~0u };
    /** Binding */
    uint32_t binding { ~0u };
    /** Handle */
    RenderHandle handle {};
    /** Second handle (e.g. sampler for combined image sampler) */
    RenderHandle secondHandle {};
    /** Mip level for image binding */
    uint32_t mip { PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS };
    /** Layer for image binding */
    uint32_t layer { PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
};

/** Render node attachment */
struct RenderNodeAttachment {
    /** Handle */
    RenderHandle handle;

    /** Load operation */
    AttachmentLoadOp loadOp { AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE };
    /** Store operation */
    AttachmentStoreOp storeOp { AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_DONT_CARE };

    /** Stencil load operation */
    AttachmentLoadOp stencilLoadOp { AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE };
    /** Stencil store operation */
    AttachmentStoreOp stencilStoreOp { AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_DONT_CARE };

    /** Clear value */
    ClearValue clearValue;

    /** Mip level */
    uint32_t mip { 0u };
    /** Layer */
    uint32_t layer { 0u };
};

/** RenderNodeHandles.
    Helper struct for inputs that can be defined in pipeline initilization phase.
*/
struct RenderNodeHandles {
    /** Input render pass */
    struct InputRenderPass {
        /** Attachments array */
        BASE_NS::vector<RenderNodeAttachment> attachments;

        /** Subpass index, if subpass index is other that zero, render pass is patched to previous render passes */
        uint32_t subpassIndex { 0u };
        /** Subpass count, automatically calculated from render node graph setup */
        uint32_t subpassCount { 1u };

        /** Render pass subpass contents */
        SubpassContents subpassContents { SubpassContents::CORE_SUBPASS_CONTENTS_INLINE };

        // render pass subpass attachment indices
        /** Depth attachment index */
        uint32_t depthAttachmentIndex { ~0u };
        /** Depth resolve attachment index */
        uint32_t depthResolveAttachmentIndex { ~0u };
        /** Input attachment indices */
        BASE_NS::vector<uint32_t> inputAttachmentIndices;
        /** Color attachment indices */
        BASE_NS::vector<uint32_t> colorAttachmentIndices;
        /** Resolve attachment indices */
        BASE_NS::vector<uint32_t> resolveAttachmentIndices;
        /** Fragment shading rate attachment index */
        uint32_t fragmentShadingRateAttachmentIndex { ~0u };

        /** Depth resolve mode flag bit */
        ResolveModeFlagBits depthResolveModeFlagBit { ResolveModeFlagBits::CORE_RESOLVE_MODE_NONE };
        /** Stencil resolve mode flag bit */
        ResolveModeFlagBits stencilResolveModeFlagBit { ResolveModeFlagBits::CORE_RESOLVE_MODE_NONE };

        /** Shading rate attachment texel size for subpass */
        Size2D shadingRateTexelSize { 1u, 1u };
        /** Multi-view bitfield of view indices. Multi-view is ignored while zero. */
        uint32_t viewMask { 0u };
    };

    /** Input resources */
    struct InputResources {
        /** Buffers */
        BASE_NS::vector<RenderNodeResource> buffers;
        /** Images */
        BASE_NS::vector<RenderNodeResource> images;
        /** Samplers */
        BASE_NS::vector<RenderNodeResource> samplers;

        /** Custom input buffers */
        BASE_NS::vector<RenderNodeResource> customInputBuffers;
        /** Custom output buffers */
        BASE_NS::vector<RenderNodeResource> customOutputBuffers;

        /** Custom input images */
        BASE_NS::vector<RenderNodeResource> customInputImages;
        /** Custom output images */
        BASE_NS::vector<RenderNodeResource> customOutputImages;
    };
};

/** Register all the inputs, outputs, resources etc. for RenderNode to use.
 */
struct RenderNodeGraphInputs {
    /** Resource */
    struct Resource {
        /** Set */
        uint32_t set { ~0u };
        /** Binding */
        uint32_t binding { ~0u };
        /** Name, optionally with FROM_NAMED_RENDER_NODE_OUTPUT the registered name */
        RenderDataConstants::RenderDataFixedString name;
        /** Usage name, optional usage name for some render nodes
         * e.g. "depth", this should be handled as depth image
         */
        RenderDataConstants::RenderDataFixedString usageName;

        /** From where is the input routed. Default uses named GPU resource from GPU resource manager. */
        RenderNodeGraphResourceLocationType resourceLocation { RenderNodeGraphResourceLocationType::DEFAULT };
        /** Index of the routed input. */
        uint32_t resourceIndex { ~0u };
        /** Node name, with FROM_NAMED_RENDER_NODE_OUTPUT */
        RenderDataConstants::RenderDataFixedString nodeName;

        /** Additional binding information */

        /** Mip level for image binding */
        uint32_t mip { PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS };
        /** Layer for image binding */
        uint32_t layer { PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
    };

    /** Attachment */
    struct Attachment {
        /** Load operation */
        AttachmentLoadOp loadOp { AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE };
        /** Store operation */
        AttachmentStoreOp storeOp { AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_DONT_CARE };

        /** Stencil load operation */
        AttachmentLoadOp stencilLoadOp { AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE };
        /** Stencil store operation */
        AttachmentStoreOp stencilStoreOp { AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_DONT_CARE };

        /** Clear value */
        ClearValue clearValue {};

        /** Name, optionally with FROM_NAMED_RENDER_NODE_OUTPUT the registered name */
        RenderDataConstants::RenderDataFixedString name;
        /** From where is the input routed. Default uses named GPU resource from GPU resource manager. */
        RenderNodeGraphResourceLocationType resourceLocation { RenderNodeGraphResourceLocationType::DEFAULT };
        /** Index of the routed input. */
        uint32_t resourceIndex { ~0u };
        /** Node name, with FROM_NAMED_RENDER_NODE_OUTPUT */
        RenderDataConstants::RenderDataFixedString nodeName;

        /** Mip level */
        uint32_t mip { 0u };
        /** Layer */
        uint32_t layer { 0u };
    };

    /** Shader input */
    struct ShaderInput {
        /** Name */
        RenderDataConstants::RenderDataFixedString name;
    };

    /** Input render pass */
    struct InputRenderPass {
        /** Render pass attachments */
        BASE_NS::vector<Attachment> attachments;

        /** Subpass index, if subpass index is not zero, this subpass is patched to previous render pass */
        uint32_t subpassIndex { 0u };
        /** Subpass count, calculated automatically when loading render node graph */
        uint32_t subpassCount { 1u };

        /** Render pass subpass contents */
        SubpassContents subpassContents { SubpassContents::CORE_SUBPASS_CONTENTS_INLINE };

        /** Depth attachment index */
        uint32_t depthAttachmentIndex { ~0u };
        /** Depth resolve attachment index */
        uint32_t depthResolveAttachmentIndex { ~0u };
        /** Input attachment indices */
        BASE_NS::vector<uint32_t> inputAttachmentIndices;
        /** Color attachment indices */
        BASE_NS::vector<uint32_t> colorAttachmentIndices;
        /** Resolve attachment indices */
        BASE_NS::vector<uint32_t> resolveAttachmentIndices;
        /** Fragment shading rate attachment index */
        uint32_t fragmentShadingRateAttachmentIndex { ~0u };

        /** Depth resolve mode flag bit */
        ResolveModeFlagBits depthResolveModeFlagBit { ResolveModeFlagBits::CORE_RESOLVE_MODE_NONE };
        /** Stencil resolve mode flag bit */
        ResolveModeFlagBits stencilResolveModeFlagBit { ResolveModeFlagBits::CORE_RESOLVE_MODE_NONE };

        /** Shading rate attachment texel size for subpass
         */
        Size2D shadingRateTexelSize { 1u, 1u };
        /** Multi-view bitfield of view indices. Multi-view is ignored while zero. */
        uint32_t viewMask { 0u };
    };

    /** Input resources (Descriptor sets etc.) */
    struct InputResources {
        // descriptors
        /** Buffers */
        BASE_NS::vector<Resource> buffers;
        /** Images */
        BASE_NS::vector<Resource> images;
        /** Samplers */
        BASE_NS::vector<Resource> samplers;

        /** Custom input buffers */
        BASE_NS::vector<Resource> customInputBuffers;
        /** Custom output buffers */
        BASE_NS::vector<Resource> customOutputBuffers;

        /** Custom input images */
        BASE_NS::vector<Resource> customInputImages;
        /** Custom output images */
        BASE_NS::vector<Resource> customOutputImages;
    };

    /** Render node graph GPU buffer descriptor */
    struct RenderNodeGraphGpuBufferDesc {
        /** Name */
        RenderDataConstants::RenderDataFixedString name;
        /** Dependency buffer name */
        RenderDataConstants::RenderDataFixedString dependencyBufferName;
        /** Render node graph share name */
        RenderDataConstants::RenderDataFixedString shareName;
        /** Buffer descriptor (Contains: usage flags, memory property flags, engine buffer creation flags and byte size)
         */
        GpuBufferDesc desc;
    };

    /** Render node graph GPU image descriptor */
    struct RenderNodeGraphGpuImageDesc {
        /** Name */
        RenderDataConstants::RenderDataFixedString name;
        /** Dependency image name */
        RenderDataConstants::RenderDataFixedString dependencyImageName;
        /** Render node graph share name */
        RenderDataConstants::RenderDataFixedString shareName;
        /** Dependency flag bits */
        enum DependencyFlagBits : uint32_t {
            /** Format */
            FORMAT = 1,
            /** Size */
            SIZE = 2,
            /** Mip count */
            MIP_COUNT = 4,
            /** Layer count */
            LAYER_COUNT = 8,
            /** Samples */
            SAMPLES = 16,
            /** Max enumeration */
            MAX_DEPENDENCY_FLAG_ENUM = 0x7FFFFFFF
        };
        /** Container for dependency flag bits */
        using DependencyFlags = uint32_t;
        /** Dependency flags */
        DependencyFlags dependencyFlags { 0 };
        /** Dependency size scale (scales only the size if size is a dependency) */
        float dependencySizeScale { 1.0f };
        /** Fragment shading rate requested texel size
         * Will check the valid values and divides the size
         */
        Size2D shadingRateTexelSize { 1u, 1u };
        /** Image descriptor (GpuImageDesc) */
        GpuImageDesc desc;
    };

    /** Resource creation description */
    struct ResourceCreationDescription {
        /** GPU buffer descriptors */
        BASE_NS::vector<RenderNodeGraphGpuBufferDesc> gpuBufferDescs;
        /** GPU image descriptors */
        BASE_NS::vector<RenderNodeGraphGpuImageDesc> gpuImageDescs;
    };

    /** CPU dependencies */
    struct CpuDependencies {
        /** Render node type based dependancy. Finds previous render node of type */
        BASE_NS::vector<RenderDataConstants::RenderDataFixedString> typeNames;
        /** Render node name based dependancy. Finds previous render node of name */
        BASE_NS::vector<RenderDataConstants::RenderDataFixedString> nodeNames;
    };

    /** GPU queue wait signal dependencies */
    struct GpuQueueWaitForSignals {
        /** Render node type based dependancy. Finds previous render node of type */
        BASE_NS::vector<RenderDataConstants::RenderDataFixedString> typeNames;
        /** Render node name based dependancy. Finds previous render node of name */
        BASE_NS::vector<RenderDataConstants::RenderDataFixedString> nodeNames;
    };

    /** Render data store */
    struct RenderDataStore {
        /** Data store name */
        RenderDataConstants::RenderDataFixedString dataStoreName;
        /** Type name */
        RenderDataConstants::RenderDataFixedString typeName;
        /** Configuration name */
        RenderDataConstants::RenderDataFixedString configurationName;
    };

    /** Queue */
    GpuQueue queue;
    /** CPU dependency */
    CpuDependencies cpuDependencies;
    /** GPU queue wait for signals */
    GpuQueueWaitForSignals gpuQueueWaitForSignals;
    /** Base render data store hook */
    RenderDataConstants::RenderDataFixedString nodeDataStoreName;
};

/** Rendering configuration */
struct RenderingConfiguration {
    /** Render backend */
    DeviceBackendType renderBackend { DeviceBackendType::VULKAN };
    /** NDC origin enumeration */
    enum class NdcOrigin {
        /** Undefined */
        UNDEFINED = 0,
        /** "Topleft (Vulkan, Default)" */
        TOP_LEFT = 1,
        /** "Bottomleft (OpenGL, OpenGL ES)" */
        BOTTOM_LEFT = 2
    };
    /** NDC origin */
    NdcOrigin ndcOrigin { NdcOrigin::TOP_LEFT };
    /** Render timings
     *(.x = delta time (ms), .y = tick delta time (ms), .z = tick total time (s), .w = frame index (asuint)) */
    BASE_NS::Math::Vec4 renderTimings { 0.0f, 0.0f, 0.0f, 0.0f };
};

/** Render node graph data */
struct RenderNodeGraphData {
    /** Render node graph name */
    RenderDataConstants::RenderDataFixedString renderNodeGraphName;
    /** Name of the first (access point) data store */
    RenderDataConstants::RenderDataFixedString renderNodeGraphDataStoreName;
    /** Rendering configuration */
    RenderingConfiguration renderingConfiguration;
};
/** @} */
RENDER_END_NAMESPACE()

#endif // API_RENDER_RENDER_DATA_STRUCTURES_H
