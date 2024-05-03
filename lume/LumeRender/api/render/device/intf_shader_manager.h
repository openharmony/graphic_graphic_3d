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

#ifndef API_RENDER_DEVICE_ISHADER_MANAGER_H
#define API_RENDER_DEVICE_ISHADER_MANAGER_H

#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <core/json/json.h>
#include <core/namespace.h>
#include <render/device/intf_shader_pipeline_binder.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()

/** \addtogroup group_ishadermanager
 *  @{
 */
/** Shader manager interface.
 *  Not internally synchronized.
 *
 *  Shaders are automatically loaded from json files with LoadShaderFiles(prefix).
 *  One can explicitly (Re)LoadShaderFile(...) but LoadShaderFiles would be preferred method.
 *  Beware that shader manager is not internally synchronized. Create methods should only be called before
 *  calling RenderFrame (and not in render nodes).
 *
 *  Loading shader data:
 *  By default shader data is loaded from default paths:
 *  Shaders "<prefix>shaders://"
 *  Shader states ""<prefix>shaderstates://";
 *  Vertex input declarations "<prefix>vertexinputdeclarations://"
 *  Pipeline layouts "<prefix>pipelinelayouts://"
 *
 *  Naming of shader resources
 *  Shaders:                        .shader
 *  Shader states (graphics state): .shadergs
 *  Vertex input declaration:       .shadervid
 *  Pipeline layout:                .shaderpl
 *
 *  Calling LoadShaderFiles effectively does the previous.
 *  In application one needs to load all plugin shader assets.
 *  This enables faster loading times when one can control if and when default shader files are needed.
 *
 *  LoadShaderFile(uri)
 *  Loads a specified shader data file. Identifies the type and loads it's data.
 *
 *  Shaders are created with default name which is the path.
 *
 *  Methods that have a name as a parameter print error message if resource not found.
 *  Methods that have a handle as a parameter return invalid handles or default data if not found.
 *
 *  Shader language is (vulkan):
 *  #version 460 core
 *  #extension GL_ARB_separate_shader_objects : enable
 *  #extension GL_ARB_shading_language_420pack : enable
 *
 *  Default specilization that can be used (handled automatically):
 *  layout(constant_id = 0) const uint CORE_BACKEND_TYPE = 0;
 *  Vulkan: CORE_BACKEND_TYPE = 0
 *  GL and GLES: CORE_BACKEND_TYPE = 1
 *
 *  NOTE: Do not set this automatically.
 *
 *  Specialization constant ids need to be smaller than 256.
 *  Engine uses special low level specializations with ids 256+.
 *  Prefer using every id from zero onwards.

 */
class IShaderManager {
public:
    struct ShaderModulePath {
        /* Shader module path */
        BASE_NS::string_view path;
        /* Shader stage flags */
        ShaderStageFlags shaderStageFlags { 0u };
    };
    struct ComputeShaderCreateInfo {
        /* Path, used as a name */
        BASE_NS::string_view path;
        /* Shader modules for this shader */
        BASE_NS::array_view<const ShaderModulePath> shaderPaths;

        /* Optional pipeline layout handle */
        RenderHandle pipelineLayout;
        /* Optional render slot mask */
        uint32_t renderSlotId { ~0u };
        /* Optional category id */
        uint32_t categoryId { ~0u };
    };
    struct ShaderCreateInfo {
        /* Path, used as a name */
        BASE_NS::string_view path;
        /* Shader modules for this shader */
        BASE_NS::array_view<const ShaderModulePath> shaderPaths;

        /* Optional default graphics state handle for the shader */
        RenderHandle graphicsState;
        /* Optional pipeline layout handle */
        RenderHandle pipelineLayout;
        /* Optional vertex input declaration handle */
        RenderHandle vertexInputDeclaration;
        /* Optional render slot mask */
        uint32_t renderSlotId { ~0u };
        /* Optional category id */
        uint32_t categoryId { ~0u };
    };

    struct PipelineLayoutCreateInfo {
        /* Path, used as a name */
        BASE_NS::string_view path;
        /* Reference to pipeline layout */
        const PipelineLayout& pipelineLayout;
    };
    struct GraphicsStateCreateInfo {
        /* Path, used as a name */
        BASE_NS::string_view path;
        /* Reference to graphics state */
        const GraphicsState& graphicsState;
    };
    struct GraphicsStateVariantCreateInfo {
        /* Render slot for the variant */
        BASE_NS::string_view renderSlot;
        /* Variant name for the graphics state */
        BASE_NS::string_view variant;
        /* Base graphics state name */
        BASE_NS::string_view baseShaderState;
        /* Base graphics state variant name */
        BASE_NS::string_view baseVariant;
        /* Forced graphics state flags */
        GraphicsStateFlags stateFlags { 0u };
    };
    struct VertexInputDeclarationCreateInfo {
        /* Path, used as a name */
        BASE_NS::string_view path;
        /* Reference to vertex input declaration view */
        const VertexInputDeclarationView& vertexInputDeclarationView;
    };
    /* Id Description which can be fetched for handle */
    struct IdDesc {
        /* Unique path */
        BASE_NS::string path;
        /* Variant name */
        BASE_NS::string variant;
        /* Display name */
        BASE_NS::string displayName;
        /* Category name */
        BASE_NS::string category;
        /* Render slot id */
        BASE_NS::string renderSlot;
        /* Frame index when the shader was loaded. (Can be used as a "timestamp" for caching) */
        uint64_t frameIndex { 0 };
    };
    /* Render slot data. Used for default render slot data handles */
    struct RenderSlotData {
        /** Render slot ID */
        uint32_t renderSlotId { ~0u };
        /** Shader handle */
        RENDER_NS::RenderHandleReference shader;
        /** Graphics state handle */
        RENDER_NS::RenderHandleReference graphicsState;
    };

    /* Shader files loading */
    struct ShaderFilePathDesc {
        /* Shaders path */
        BASE_NS::string_view shaderPath;
        /* Shader states path */
        BASE_NS::string_view shaderStatePath;
        /* Pipeline layouts path */
        BASE_NS::string_view pipelineLayoutPath;
        /* Vertex input declarations path */
        BASE_NS::string_view vertexInputDeclarationPath;
    };

    /** Get render handle reference of raw handle.
     *  @param handle Raw render handle
     *  @return Returns A render handle reference for the handle.
     */
    virtual RenderHandleReference Get(const RenderHandle& handle) const = 0;

    /** Create a compute shader. Prefer loading shaders from json files.
     *  @param createInfo A create info with valid parameters.
     *  @return Returns compute shader gpu resource handle.
     */
    virtual RenderHandleReference CreateComputeShader(const ComputeShaderCreateInfo& createInfo) = 0;

    /** Create a compute shader with a variant name. Prefer loading shaders from json files.
     *  @param createInfo A create info with valid parameters.
     *  @param baseShaderPath A base shader path/name to which the added variant is for.
     *  @param variantName A variant name.
     *  @return Returns compute shader gpu resource handle.
     */
    virtual RenderHandleReference CreateComputeShader(const ComputeShaderCreateInfo& createInfo,
        const BASE_NS::string_view baseShaderPath, const BASE_NS::string_view variantName) = 0;

    /** Create a shader. Prefer loading shaders from json files.
     *  @param createInfo A create info with valid parameters.
     *  @return Returns shader gpu resource handle.
     */
    virtual RenderHandleReference CreateShader(const ShaderCreateInfo& createInfo) = 0;

    /** Create a shader. Prefer loading shaders from json files.
     *  @param createInfo A create info with valid parameters.
     *  @param baseShaderPath A base shader path/name to which the added variant is for.
     *  @param variantName A variant name.
     *  @return Returns shader gpu resource handle.
     */
    virtual RenderHandleReference CreateShader(const ShaderCreateInfo& createInfo,
        const BASE_NS::string_view baseShaderPath, const BASE_NS::string_view variantName) = 0;

    /** Create a pipeline layout. Prefer loading pipeline layouts from json files.
     *  @param createInfo A pipeline layout create info.
     *  @return Returns pipeline layout render handle.
     */
    virtual RenderHandleReference CreatePipelineLayout(const PipelineLayoutCreateInfo& createInfo) = 0;

    /** Create a vertex input declaration. Prefer laoding vertex input declaration from json files.
     *  @param createInfo A vertex input declaration create info.
     *  @return Returns vertex input declaration handle.
     */
    virtual RenderHandleReference CreateVertexInputDeclaration(const VertexInputDeclarationCreateInfo& createInfo) = 0;

    /** Create a graphics state. Prefer loading graphics states from json files.
     *  @param name Unique name of the graphics state. (Optional name, can be empty string view)
     *  @param createInfo A graphics state create info.
     *  @return Returns graphics state render handle.
     */
    virtual RenderHandleReference CreateGraphicsState(const GraphicsStateCreateInfo& createInfo) = 0;

    /** Create a graphics state with additional variant info. Prefer loading graphics states from json files.
     *  @param createInfo A graphics state create info.
     *  @param variantCreateInfo A create info for shader graphics state variant.
     *  @return Returns graphics state render handle.
     */
    virtual RenderHandleReference CreateGraphicsState(
        const GraphicsStateCreateInfo& createInfo, const GraphicsStateVariantCreateInfo& variantCreateInfo) = 0;

    /** Create a render slot id for render slot name. If render slot already created for the name the slot id is
     * returned.
     *  @param name Unique name of the render slot id.
     *  @return Returns render slot id for the name.
     */
    virtual uint32_t CreateRenderSlotId(const BASE_NS::string_view name) = 0;

    /** Set render slot default data.
     *  @param renderSlotId Render slot id.
     *  @param shaderHandle Render slot default shader handle. (Does not replace if not valid)
     *  @param stateHandle Render slot default graphics state handle. (Does not replace if not valid)
     */
    virtual void SetRenderSlotData(const uint32_t renderSlotId, const RenderHandleReference& shaderHandle,
        const RenderHandleReference& stateHandle) = 0;

    /** Get shader handle.
     *  @param path Path/Name of the shader.
     *  @return Returns shader handle.
     */
    virtual RenderHandleReference GetShaderHandle(const BASE_NS::string_view path) const = 0;

    /** Get shader variant handle. One can fetch and explicit handle to variant.
     *  @param path Path/Name of the base shader.
     *  @param variantName Name of variant.
     *  @return Returns shader gpu resource handle.
     */
    virtual RenderHandleReference GetShaderHandle(
        const BASE_NS::string_view path, const BASE_NS::string_view variantName) const = 0;

    /** Get shader handle. Tries to find a shader variant for provided render slot id.
     *  @param handle Shader handle.
     *  @param renderSlotId Render slot id.
     *  @return Returns shader handle for given render slot id.
     */
    virtual RenderHandleReference GetShaderHandle(
        const RenderHandleReference& handle, const uint32_t renderSlotId) const = 0;

    /** Get shaders
     *  @param renderSlotId Id of render slot
     *  @return Returns shader handles for given render slot id.
     */
    virtual BASE_NS::vector<RenderHandleReference> GetShaders(const uint32_t renderSlotId) const = 0;

    /** Get graphics state based on path name
     *  @param path Path/Name of the graphics state
     */
    virtual RenderHandleReference GetGraphicsStateHandle(const BASE_NS::string_view path) const = 0;

    /** Get graphics state based on name and variant name
     *  @param path Path/Name of the graphics state
     *  @param name Variant name of the graphics state
     */
    virtual RenderHandleReference GetGraphicsStateHandle(
        const BASE_NS::string_view path, const BASE_NS::string_view variantName) const = 0;

    /** Get graphics state handle. Tries to find a graphics state variant for provided render slot id.
     *  Returns the given handle if it's own slot matches.
     *  @param handle Graphics state handle.
     *  @param renderSlotId Render slot id.
     *  @return Returns shader handle for given render slot id.
     */
    virtual RenderHandleReference GetGraphicsStateHandle(
        const RenderHandleReference& handle, const uint32_t renderSlotId) const = 0;

    /** Get graphics state based on graphics state hash.
     *  @param hash A hash created with HashGraphicsState().
     *  @return Graphics state handle.
     */
    virtual RenderHandleReference GetGraphicsStateHandleByHash(const uint64_t hash) const = 0;

    /** Get graphics state handle based on shader handle. The default graphics in shader json.
     *  @param handle Valid shader handle.
     *  @return Graphics state handle.
     */
    virtual RenderHandleReference GetGraphicsStateHandleByShaderHandle(const RenderHandleReference& handle) const = 0;

    /** Get graphics state.
     *  @param handle Graphics state render handle.
     *  @return Graphics state for the handle.
     */
    virtual GraphicsState GetGraphicsState(const RenderHandleReference& handle) const = 0;

    /** Get graphics states.
     *  @param renderSlotId Id of render slot.
     *  @return Returns all graphics state handles for the render slot id.
     */
    virtual BASE_NS::vector<RenderHandleReference> GetGraphicsStates(const uint32_t renderSlotId) const = 0;

    /** Get render slot ID
     *  @param renderSlot Name of render slot
     *  @return render slot id if found (~0u invalid)
     */
    virtual uint32_t GetRenderSlotId(const BASE_NS::string_view renderSlot) const = 0;

    /** Get render slot ID.
     *  @param handle Handle of shader or graphics state.
     *  @return render slot id if found (~0u invalid)
     */
    virtual uint32_t GetRenderSlotId(const RenderHandleReference& handle) const = 0;

    /** Get render slot data. Get default data of render slot.
     *  @param renderSlotId Render slot id.
     *  @return render slot data.
     */
    virtual RenderSlotData GetRenderSlotData(const uint32_t renderSlotId) const = 0;

    /** Get render slot name.
     *  @param renderSlotId Render slot id.
     *  @return render slot name.
     */
    virtual BASE_NS::string GetRenderSlotName(const uint32_t renderSlotId) const = 0;

    /** Get vertex input declaration handle by shader handle.
     *  @param handle A handle of the shader which vertex input declaration data handle will be returned.
     *  @return Returns a data handle for vertex input declaration.
     */
    virtual RenderHandleReference GetVertexInputDeclarationHandleByShaderHandle(
        const RenderHandleReference& handle) const = 0;

    /** Get vertex input declaration handle by vertex input declaration name.
     *  @param path Path/Name of the vertex input declaration given in data.
     *  @return Returns a data handle for vertex input declaration.
     */
    virtual RenderHandleReference GetVertexInputDeclarationHandle(const BASE_NS::string_view path) const = 0;

    /** Get vertex input declaration view by vertex input declaration data handle.
     *  The return value should not be hold into as it contains array_views to data which my change.
     *  @param handle A handle to the vertex input declaration data.
     *  @return Returns a view to vertex input declaration data.
     */
    virtual VertexInputDeclarationView GetVertexInputDeclarationView(const RenderHandleReference& handle) const = 0;

    /** Get pipeline layout handle by shader handle.
     *  @param handle A handle of the shader which pipeline layout data handle will be returned.
     *  @return Returns a handle to pipeline layout.
     */
    virtual RenderHandleReference GetPipelineLayoutHandleByShaderHandle(const RenderHandleReference& handle) const = 0;

    /** Get pipeline layout by shader or compute shader pipeline layout handle.
     *  @param handle A handle to pipeline layout.
     *  @return Returns a const reference to pipeline layout.
     */
    virtual PipelineLayout GetPipelineLayout(const RenderHandleReference& handle) const = 0;

    /** Get pipeline layout handle by pipeline layout name.
     *  @param path Path/Name of the pipeline layout.
     *  @return Returns a handle to pipeline layout.
     */
    virtual RenderHandleReference GetPipelineLayoutHandle(const BASE_NS::string_view path) const = 0;

    /** Get pipeline layout handle for reflected shader data.
     *  @param handle A handle to a valid shader or compute shader.
     *  @return Returns a handle to pipeline layout.
     */
    virtual RenderHandleReference GetReflectionPipelineLayoutHandle(const RenderHandleReference& handle) const = 0;

    /** Get pipeline layout reflected from the shader or compute shader pipeline layout handle.
     *  @param handle A handle to a valid shader or compute shader.
     *  @return Returns a const reference to pipeline layout.
     */
    virtual PipelineLayout GetReflectionPipelineLayout(const RenderHandleReference& handle) const = 0;

    /** Get specialization reflected from the given shader.
     *  @param handle A handle to a valid shader or compute shader.
     *  @return Returns a view to shader specialization. The struct should not be holded unto.
     */
    virtual ShaderSpecializationConstantView GetReflectionSpecialization(const RenderHandleReference& handle) const = 0;

    /** Get vertex input declaration reflected from the shader. (Zero values if shader does not have one)
     *  @param handle A handle to a valid shader.
     *  @return Returns a view to vertex input declaration view. The struct should not be holded unto.
     */
    virtual VertexInputDeclarationView GetReflectionVertexInputDeclaration(
        const RenderHandleReference& handle) const = 0;

    /** Get thread group size of a given shader. (Zero values if shader does not have thread group)
     *  @param handle A handle to a valid compute shader.
     *  @return Returns a shader thread group size.
     */
    virtual ShaderThreadGroup GetReflectionThreadGroupSize(const RenderHandleReference& handle) const = 0;

    /** Hash graphics state.
     *  @return Returns a hash for shader related graphics state.
     */
    virtual uint64_t HashGraphicsState(const GraphicsState& graphicsState) const = 0;

    /** Checks if resource is a compute shader */
    virtual bool IsComputeShader(const RenderHandleReference& handle) const = 0;
    /** Checks if resource is a shader */
    virtual bool IsShader(const RenderHandleReference& handle) const = 0;

    /** Load shader files.
     * Looks for json files under paths for shaders, shader states, vertex input declarations, and pipeline
     * layouts. Creates resources based on loaded data.
     * NOTE: does not re-create shader modules if the module with the same spv has already been done.
     * @param desc Paths to shader files
     */
    virtual void LoadShaderFiles(const ShaderFilePathDesc& desc) = 0;

    /** Load shader file and create resources.
     *  NOTE: beware that shader json files can refer to names (not uris) of other shader resources (like pipeline
     *  layout) and they need to be loaded before hand. Prefer using LoadShaderFiles() or automatic loading of shaders.
     *  @param uri A uri to a valid shader data json file.
     */
    virtual void LoadShaderFile(const BASE_NS::string_view uri) = 0;

    /** Unload shader files.
     * Looks for json files under paths for shaders, shader states, vertex input declarations, and pipeline
     * layouts. Unloads/Destroys the shaders.
     * @param desc Paths to shader files
     */
    virtual void UnloadShaderFiles(const ShaderFilePathDesc& desc) = 0;

    /** Get json string.
     * @param handle A handle to a valid shader.
     * @return Returns string view of loaded shader file.
     */
    virtual const BASE_NS::string_view GetShaderFile(const RenderHandleReference& handle) const = 0;

    /** Get material metadata for the shader.
     * @param handle A handle to a valid shader.
     * @return Returns available metadata as JSON.
     */
    virtual const CORE_NS::json::value* GetMaterialMetadata(const RenderHandleReference& handle) const = 0;

    /** Destroy shader data handle (shaders, pipeline layouts, shader states, vertex input declarations).
     * @param handle A handle to a valid shader (pl, state, vid).
     */
    virtual void Destroy(const RenderHandleReference& handle) = 0;

    /** Get shaders based on render handle (pipeline layout, vertex input declaration, graphics state).
     * Based on ShaderStageFlags searches for valid shaders.
     * Not fast operation and should not be queried every frame.
     * @param handle A handle to a valid render handle (pl, vid).
     * @param shaderStateFlags Flags for specific shaders.
     * @return Returns All shaders that use the specific handle.
     */
    virtual BASE_NS::vector<RenderHandleReference> GetShaders(
        const RenderHandleReference& handle, const ShaderStageFlags shaderStageFlags) const = 0;

    /** Get IdDesc for a RenderHandle.
     * @param handle A handle to a valid render handle.
     * @return Returns IdDesc for a given handle.
     */
    virtual IdDesc GetIdDesc(const RenderHandleReference& handle) const = 0;

    /** Get frame update index for a RenderHandle.
     * @param handle A handle to a valid render handle.
     * @return Returns frame index.
     */
    virtual uint64_t GetFrameIndex(const RenderHandleReference& handle) const = 0;

    /** Reload shader file and create resources.
     *  NOTE: Force re-creates shader modules from spv -files in shader json.
     *  NOTE: Do not call on perf-critical path.
     *  NOTE: beware that shader json files can refer to names (not uris) of other shader resources (like pipeline
     *  layout) and they need to be loaded before hand. Prefer using LoadShaderFiles() or automatic loading of shaders.
     *  @param uri A uri to a valid shader data json file.
     */
    virtual void ReloadShaderFile(const BASE_NS::string_view uri) = 0;

    /** Get all shaders.
     *  @return vector of all shaders.
     */
    virtual BASE_NS::vector<RenderHandleReference> GetShaders() const = 0;

    /** Get all graphics states.
     *  @return vector of all graphics states.
     */
    virtual BASE_NS::vector<RenderHandleReference> GetGraphicsStates() const = 0;

    /** Get all pipeline layouts.
     *  @return vector of all pipeline layouts.
     */
    virtual BASE_NS::vector<RenderHandleReference> GetPipelineLayouts() const = 0;

    /** Get all vertex input declarations.
     *  @return vector of all vertex input declarations.
     */
    virtual BASE_NS::vector<RenderHandleReference> GetVertexInputDeclarations() const = 0;

    /** Create shader pipeline binder.
     *  @param handle Shader handle.
     *  @return Returns shader pipeline binder.
     */
    virtual IShaderPipelineBinder::Ptr CreateShaderPipelineBinder(const RenderHandleReference& handle) const = 0;

    /** Create shader pipeline binder.
     *  @param handle Shader handle.
     *  @param plHandle Enforced pipeline layout handle.
     *  @return Returns shader pipeline binder.
     */
    virtual IShaderPipelineBinder::Ptr CreateShaderPipelineBinder(
        const RenderHandleReference& handle, const RenderHandleReference& plHandle) const = 0;

    /** Create shader pipeline binder.
     *  @param handle Shader handle.
     *  @param pipelineLayout Enforced pipeline layout.
     *  @return Returns shader pipeline binder.
     */
    virtual IShaderPipelineBinder::Ptr CreateShaderPipelineBinder(
        const RenderHandleReference& handle, const PipelineLayout& pipelineLayout) const = 0;

    /** Compatibility flags */
    enum CompatibilityFlagBits {
        /** Is compatible */
        COMPATIBLE_BIT = 0x00000001,
        /** Exact match */
        EXACT_BIT = 0x00000002,
    };
    /** Container for compatibility flag flag bits */
    using CompatibilityFlags = uint32_t;

    /** Check compatibility of render resources.
     *  @param lhs The base for compatibility check.
     *  @param rhs The one which will be compared against the base.
     *  @return Returns compatibility flags.
     */
    virtual CompatibilityFlags GetCompatibilityFlags(
        const RenderHandleReference& lhs, const RenderHandleReference& rhs) const = 0;

    /** Check forced graphics states. Can be set for render slot variants.
     *  @param handle Graphics state or shader handle. Faster to check with graphics state handle.
     *  @return Returns forced graphics state flags.
     */
    virtual GraphicsStateFlags GetForcedGraphicsStateFlags(const RenderHandleReference& handle) const = 0;

    /** Check forced graphics states. Can be set for render slot variants.
     *  @param renderSlotId Render slot id. Find the base graphics state and checks the forced flags
     *  @return Returns forced graphics state flags.
     */
    virtual GraphicsStateFlags GetForcedGraphicsStateFlags(const uint32_t renderSlotId) const = 0;

protected:
    IShaderManager() = default;
    virtual ~IShaderManager() = default;

    IShaderManager(const IShaderManager&) = delete;
    IShaderManager& operator=(const IShaderManager&) = delete;
    IShaderManager(IShaderManager&&) = delete;
    IShaderManager& operator=(IShaderManager&&) = delete;
};

/** IRenderNodeShaderManager.
 *  Shader manager interface to be used inside render nodes.
 *
 *  Some methods use faster path (no locking).
 *  Some methods are not available like creation methods
 */
class IRenderNodeShaderManager {
public:
    /** Get shader handle (any type).
     *  @param path Path/Name of the shader.
     *  @return Returns shader handle.
     */
    virtual RenderHandle GetShaderHandle(const BASE_NS::string_view path) const = 0;

    /** Get shader handle.
     *  @param path Path/Name of the shader.
     *  @param variantName Name of the variant.
     *  @return Returns shader gpu resource handle.
     */
    virtual RenderHandle GetShaderHandle(
        const BASE_NS::string_view path, const BASE_NS::string_view variantName) const = 0;

    /** Get shader handle. Tries to find variant of shader with given render slot id.
     *  @param handle Shader handle.
     *  @param renderSlotId Render slot id.
     *  @return Returns shader gpu resource handle for given slot.
     */
    virtual RenderHandle GetShaderHandle(const RenderHandle& handle, const uint32_t renderSlotId) const = 0;

    /** Get shaders
     *  @param renderSlotId Id of render slot
     */
    virtual BASE_NS::vector<RenderHandle> GetShaders(const uint32_t renderSlotId) const = 0;

    /** Get graphics state based on name
     *  @param path Path/Name of the graphics state
     */
    virtual RenderHandle GetGraphicsStateHandle(const BASE_NS::string_view path) const = 0;

    /** Get graphics state based on name
     *  @param path Path/Name of the graphics state
     *  @param variantName Variant name of the graphics state
     */
    virtual RenderHandle GetGraphicsStateHandle(
        const BASE_NS::string_view path, const BASE_NS::string_view variantName) const = 0;

    /** Get graphics state handle. Tries to find variant of graphics state with given render slot id.
     *  Returns the given handle if its render slot id matches.
     *  @param handle Graphics state handle.
     *  @param renderSlotId Render slot id.
     *  @return Returns shader graphics state handle for given slot.
     */
    virtual RenderHandle GetGraphicsStateHandle(const RenderHandle& handle, const uint32_t renderSlotId) const = 0;

    /** Get graphics state based on graphics state hash
     *  @param hash A hash created with HashGraphicsState()
     */
    virtual RenderHandle GetGraphicsStateHandleByHash(const uint64_t hash) const = 0;

    /** Get graphics state handle based on shader handle
     *  @param handle Valid shader handle.
     */
    virtual RenderHandle GetGraphicsStateHandleByShaderHandle(const RenderHandle& handle) const = 0;

    /** Get graphics state
     *  @param handle Graphics state render handle
     */
    virtual const GraphicsState& GetGraphicsState(const RenderHandle& handle) const = 0;

    /** Get render slot ID
     *  @param renderSlot Name of render slot
     *  @return render slot id if found (~0u invalid)
     */
    virtual uint32_t GetRenderSlotId(const BASE_NS::string_view renderSlot) const = 0;

    /** Get render slot ID
     *  @param handle Handle to resource
     *  @return render slot mask if found (0 invalid)
     */
    virtual uint32_t GetRenderSlotId(const RenderHandle& handle) const = 0;

    /** Get render slot data. Get default data of render slot.
     *  @param renderSlotId Render slot id.
     *  @return render slot data.
     */
    virtual IShaderManager::RenderSlotData GetRenderSlotData(const uint32_t renderSlotId) const = 0;

    /** Get vertex input declaration handle by shader handle.
     *  @param handle A handle of the shader which vertex input declaration data handle will be returned.
     *  @return Returns a data handle for vertex input declaration.
     */
    virtual RenderHandle GetVertexInputDeclarationHandleByShaderHandle(const RenderHandle& handle) const = 0;

    /** Get vertex input declaration handle by vertex input declaration name.
     *  @param path Path/Name of the vertex input declaration given in data.
     *  @return Returns a data handle for vertex input declaration.
     */
    virtual RenderHandle GetVertexInputDeclarationHandle(const BASE_NS::string_view path) const = 0;

    /** Get vertex input declaration view by vertex input declaration data handle.
     *  The return value should not be hold into as it contains array_views to data which my change.
     *  @param handle A handle to the vertex input declaration data.
     *  @return Returns a view to vertex input declaration data.
     */
    virtual VertexInputDeclarationView GetVertexInputDeclarationView(const RenderHandle& handle) const = 0;

    /** Get pipeline layout handle by shader handle.
     *  @param handle A handle of the shader which pipeline layout data handle will be returned.
     *  @return Returns a handle to pipeline layout.
     */
    virtual RenderHandle GetPipelineLayoutHandleByShaderHandle(const RenderHandle& handle) const = 0;

    /** Get pipeline layout by shader or compute shader pipeline layout handle.
     *  @param handle A handle to pipeline layout.
     *  @return Returns a const reference to pipeline layout.
     */
    virtual const PipelineLayout& GetPipelineLayout(const RenderHandle& handle) const = 0;

    /** Get pipeline layout handle by pipeline layout name.
     *  @param path Path/Name of the pipeline layout.
     *  @return Returns a handle to pipeline layout.
     */
    virtual RenderHandle GetPipelineLayoutHandle(const BASE_NS::string_view path) const = 0;

    /** Get pipeline layout handle for the shader reflection pipeline layout.
     *  @param handle A handle to a valid shader or compute shader.
     *  @return Returns a render handle to pipeline layout.
     */
    virtual RenderHandle GetReflectionPipelineLayoutHandle(const RenderHandle& handle) const = 0;

    /** Get pipeline layout reflected from the shader or compute shader pipeline layout handle.
     *  @param handle A handle to a valid shader or compute shader.
     *  @return Returns a const reference to pipeline layout.
     */
    virtual const PipelineLayout& GetReflectionPipelineLayout(const RenderHandle& handle) const = 0;

    /** Get specialization reflected from the given shader.
     *  @param handle A handle to a valid shader or compute shader.
     *  @return Returns a view to shader specialization. The struct should not be holded unto.
     */
    virtual ShaderSpecializationConstantView GetReflectionSpecialization(const RenderHandle& handle) const = 0;

    /** Get vertex input declaration reflected from the shader. (Zero values if shader does not have one)
     *  @param handle A handle to a valid shader.
     *  @return Returns a view to vertex input declaration view. The struct should not be holded unto.
     */
    virtual VertexInputDeclarationView GetReflectionVertexInputDeclaration(const RenderHandle& handle) const = 0;

    /** Get thread group size of a given shader. (Zero values if shader does not have thread group)
     *  @param handle A handle to a valid compute shader.
     *  @return Returns a shader thread group size.
     */
    virtual ShaderThreadGroup GetReflectionThreadGroupSize(const RenderHandle& handle) const = 0;

    /** Hash graphics state.
     *  @return Returns a hash for shader related graphics state.
     */
    virtual uint64_t HashGraphicsState(const GraphicsState& graphicsState) const = 0;

    /** Simple check if handle is valid or invalid */
    virtual bool IsValid(const RenderHandle& handle) const = 0;
    /** Checks if resource is a compute shader */
    virtual bool IsComputeShader(const RenderHandle& handle) const = 0;
    /** Checks if resource is a shader */
    virtual bool IsShader(const RenderHandle& handle) const = 0;

    /** Get shaders based on render handle (pipeline layout, vertex input declaration, graphics state).
     * Not fast operation and should not be queried every frame.
     * @param handle A handle to a valid render handle (pl, vid).
     * @param shaderStateFlags Flags for specific shaders.
     * @return Returns All shaders that use the specific handle.
     */
    virtual BASE_NS::vector<RenderHandle> GetShaders(
        const RenderHandle& handle, const ShaderStageFlags shaderStageFlags) const = 0;

    /** Check compatibility of render resources.
     *  @param lhs The base for compatibility check.
     *  @param rhs The one which will be compared against the base.
     *  @return Returns compatibility flags.
     */
    virtual IShaderManager::CompatibilityFlags GetCompatibilityFlags(
        const RenderHandle& lhs, const RenderHandle& rhs) const = 0;

    /** Check forced graphics states. Can be set for render slot variants.
     *  @param handle Graphics state or shader handle. Faster to check with graphics state handle.
     *  @return Returns forced graphics state flags.
     */
    virtual GraphicsStateFlags GetForcedGraphicsStateFlags(const RenderHandle& handle) const = 0;

    /** Check forced graphics states. Can be set for render slot variants.
     *  @param renderSlotId Render slot id. Find the base graphics state and checks the forced flags
     *  @return Returns forced graphics state flags.
     */
    virtual GraphicsStateFlags GetForcedGraphicsStateFlags(const uint32_t renderSlotId) const = 0;

protected:
    IRenderNodeShaderManager() = default;
    virtual ~IRenderNodeShaderManager() = default;

    IRenderNodeShaderManager(const IRenderNodeShaderManager&) = delete;
    IRenderNodeShaderManager& operator=(const IRenderNodeShaderManager&) = delete;
    IRenderNodeShaderManager(IRenderNodeShaderManager&&) = delete;
    IRenderNodeShaderManager& operator=(IRenderNodeShaderManager&&) = delete;
};
/** @} */
RENDER_END_NAMESPACE()

#endif // API_RENDER_DEVICE_ISHADER_MANAGER_H
