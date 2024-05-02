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

#ifndef API_3D_GLTF_GLTF_H
#define API_3D_GLTF_GLTF_H

#include <cstdint>

#include <3d/ecs/components/mesh_component.h>
#include <3d/namespace.h>
#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <core/ecs/entity_reference.h>
#include <render/device/pipeline_state_desc.h>
#include <render/resource_handle.h>

CORE_BEGIN_NAMESPACE()
class IEcs;
class IThreadPool;
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
/** \addtogroup group_gltf_gltf
 *  @{
 */
/** No scene explicitly defined in import options. */
const unsigned long CORE_GLTF_INVALID_INDEX = 0x7FFFFFFF;

/** Interface to GLTF data. */
class IGLTFData {
public:
    /** Populates buffers with binary data.
     *  @return true if buffers were successfully loaded, otherwise false.
     */
    virtual bool LoadBuffers() = 0;

    /** Releases allocated data from buffers. */
    virtual void ReleaseBuffers() = 0;

    /** Can be used to retrieve external file dependencies.
     *  @return Array of file uris that describe required files that need to be available for this glTF file to
     * successfully load.
     */
    virtual BASE_NS::vector<BASE_NS::string> GetExternalFileUris() = 0;

    /** Retrieve default scene index. */
    virtual size_t GetDefaultSceneIndex() const = 0;

    /** Retrieve number of scenes in this glTF file.
     *  @return Number of scenes available.
     */
    virtual size_t GetSceneCount() const = 0;

    /** Describes thumbnail image structure. */
    struct ThumbnailImage {
        /** Data extension, such as 'png'. */
        BASE_NS::string extension;

        /** Image data buffer. */
        BASE_NS::array_view<const uint8_t> data;
    };

    /** Retrieve number of thumbnails in this glTF file.
     *  @return Number of thumbnail images available.
     */
    virtual size_t GetThumbnailImageCount() const = 0;

    /** Retrieve thumbnail image data of given thumbnail.
     *  @param thumbnailIndex Index of the requested thumbnail image.
     *  @return A structure containing thumbnail image data.
     */
    virtual ThumbnailImage GetThumbnailImage(size_t thumbnailIndex) = 0;

    struct Deleter {
        constexpr Deleter() noexcept = default;
        void operator()(IGLTFData* ptr) const
        {
            ptr->Destroy();
        }
    };
    using Ptr = BASE_NS::unique_ptr<IGLTFData, Deleter>;

protected:
    IGLTFData() = default;
    virtual ~IGLTFData() = default;
    virtual void Destroy() = 0;
};

/** Describes result of the loading operation. */
struct GLTFLoadResult {
    GLTFLoadResult() = default;
    explicit GLTFLoadResult(BASE_NS::string&& error) : success(false), error(error) {}

    /** Indicates, whether the loading operation is successful. */
    bool success { true };

    /** In case of parsing error, contains the description of the error. */
    BASE_NS::string error;

    /** Loaded data. */
    IGLTFData::Ptr data;
};

/** Access to imported GLTF resources. */
struct GLTFResourceData {
    /** Imported samplers. */
    BASE_NS::vector<CORE_NS::EntityReference> samplers;

    /** Imported images. */
    BASE_NS::vector<CORE_NS::EntityReference> images;

    /** Imported textures. */
    BASE_NS::vector<CORE_NS::EntityReference> textures;

    /** Imported materials. */
    BASE_NS::vector<CORE_NS::EntityReference> materials;

    /** Imported geometry. */
    BASE_NS::vector<CORE_NS::EntityReference> meshes;

    /** Imported skins. */
    BASE_NS::vector<CORE_NS::EntityReference> skins;

    /** Imported animations. */
    BASE_NS::vector<CORE_NS::EntityReference> animations;

    /** Imported IBL cubemaps. */
    BASE_NS::vector<CORE_NS::EntityReference> specularRadianceCubemaps;
};

/** Flags for resource import. */
enum GltfResourceImportFlagBits {
    /** Sampler */
    CORE_GLTF_IMPORT_RESOURCE_SAMPLER = 0x00000001,
    /** Image */
    CORE_GLTF_IMPORT_RESOURCE_IMAGE = 0x00000002,
    /** Texture */
    CORE_GLTF_IMPORT_RESOURCE_TEXTURE = 0x00000004,
    /** Material */
    CORE_GLTF_IMPORT_RESOURCE_MATERIAL = 0x00000008,
    /** Mesh */
    CORE_GLTF_IMPORT_RESOURCE_MESH = 0x00000010,
    /** Skin */
    CORE_GLTF_IMPORT_RESOURCE_SKIN = 0x00000020,
    /** Animation */
    CORE_GLTF_IMPORT_RESOURCE_ANIMATION = 0x00000040,
    /** Skip resources that are not referenced and do not import them. */
    CORE_GLTF_IMPORT_RESOURCE_SKIP_UNUSED = 0x00000080,
    /** Keep mesh data for CPU access. Allowing CPU access increases memory usage. */
    CORE_GLTF_IMPORT_RESOURCE_MESH_CPU_ACCESS = 0x00000100,
    /** All flags bits */
    CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL = 0x7FFFFEFF
};

/** Container for flags for resource import. */
using GltfResourceImportFlags = uint32_t;

/** Flags for scene / ecs component import. */
enum GltfSceneImportFlagBits {
    /** Scene, Deprecated value used for environment */
    CORE_GLTF_IMPORT_COMPONENT_SCENE = 0x00000001,
    /** Environment */
    CORE_GLTF_IMPORT_COMPONENT_ENVIRONMENT = 0x00000001,
    /** Mesh */
    CORE_GLTF_IMPORT_COMPONENT_MESH = 0x00000002,
    /** Camera */
    CORE_GLTF_IMPORT_COMPONENT_CAMERA = 0x00000004,
    /** Skin */
    CORE_GLTF_IMPORT_COMPONENT_SKIN = 0x00000008,
    /** Light */
    CORE_GLTF_IMPORT_COMPONENT_LIGHT = 0x00000010,
    /** Morph */
    CORE_GLTF_IMPORT_COMPONENT_MORPH = 0x00000020,
    /** All flag bits */
    CORE_GLTF_IMPORT_COMPONENT_FLAG_BITS_ALL = 0x7FFFFFFF
};

/** Container for Gltf scene import flag bits */
using GltfSceneImportFlags = uint32_t;

/** Describes result of the import operation. */
struct GLTFImportResult {
    /** Indicates, whether the import operation is successful. */
    bool success { true };

    /** In case of import error, contains the description of the error. */
    BASE_NS::string error;

    /** Imported data. */
    GLTFResourceData data;
};

struct GltfMeshData {
    struct SubMesh {
        uint32_t indices;
        uint32_t vertices;
        BASE_NS::array_view<const uint8_t> indexBuffer;
        BASE_NS::array_view<const uint8_t> attributeBuffers[MeshComponent::Submesh::BUFFER_COUNT];
    };
    struct Mesh {
        BASE_NS::vector<SubMesh> subMeshes;
    };
    /** Vertex input declaration used for formatting the data. */
    RENDER_NS::VertexInputDeclarationData vertexInputDeclaration;
    /** Data of each imported mesh. They are in the same order as in GLTFResourceData::meshes. */
    BASE_NS::vector<Mesh> meshes;
};

/** GLTF2 importer interface */
class IGLTF2Importer {
public:
    /** Listener for import events. */
    class Listener {
    public:
        virtual ~Listener() = default;

        /** On import started */
        virtual void OnImportStarted() = 0;
        /** On import finished */
        virtual void OnImportFinished() = 0;
        /** On import progressed */
        virtual void OnImportProgressed(size_t taskIndex, size_t taskCount) = 0;
    };

    /** Import GLTF2 data synchronously. The previous imported data will be discarded. */
    virtual void ImportGLTF(const IGLTFData& data, GltfResourceImportFlags flags) = 0;

    /** Import GLTF2 data asynchronously, user is required to call Execute() from main thread until it returns true.
     * The previous imported data will be discarded. */
    virtual void ImportGLTFAsync(const IGLTFData& data, GltfResourceImportFlags flags, Listener* listener) = 0;

    /** Advances the import process, needs to be called from the main thread when performing asynchronous import.
     *  @param timeBudget Time budget for resource import in microseconds, if 0 all available work will be executed
     *  during this frame.
     */
    virtual bool Execute(uint32_t timeBudget) = 0;

    /** Cancel import operation, this does not discard imported data. */
    virtual void Cancel() = 0;

    /** Returns true when import process is completed. */
    virtual bool IsCompleted() const = 0;

    /** Returns imported data and success of the whole operation. The imported resources are reference counted and the
     * importer holds references until a new import is started or the imported is destroyed. Therefore a copy of
     * GLTFImportResult::GLTFResourceData (or selected EntityReferences) should be stored. */
    virtual const GLTFImportResult& GetResult() const = 0;

    /** Returns CPU accessible mesh data. Data is available when CORE_GLTF_IMPORT_RESOURCE_MESH_CPU_ACCESS was included
     * in import flags. Unless copied, the data is valid until a new import is started or the imported is destroyed. */
    virtual const GltfMeshData& GetMeshData() const = 0;

    struct Deleter {
        constexpr Deleter() noexcept = default;
        void operator()(IGLTF2Importer* ptr) const
        {
            ptr->Destroy();
        }
    };
    using Ptr = BASE_NS::unique_ptr<IGLTF2Importer, Deleter>;

protected:
    IGLTF2Importer() = default;
    virtual ~IGLTF2Importer() = default;
    virtual void Destroy() = 0;
};

class IGltf2 {
public:
    /** Load GLTF data from URI.
     *  @param uri URI pointing to the glTF data.
     *  @return If the glTF data could be parsed GLTFLoadResult::success will be true and GLTFLoadResult::data is valid.
     *  If parsing fails GLTFLoadResult::success will be false and GLTFLoadResult::error will give more details on the
     *  failure.
     */
    virtual GLTFLoadResult LoadGLTF(BASE_NS::string_view uri) = 0;

    /** Load GLTF data from memory.
     *  @param data Contents of a GLB or a glTF file with embedded data.
     *  The memory should not be released until loading and importing have been completed.
     *  @return If the glTF data could be parsed GLTFLoadResult::success will be true and GLTFLoadResult::data is valid.
     * If parsing fails GLTFLoadResult::success will be false and GLTFLoadResult::error will give more details on the
     * failure.
     */
    virtual GLTFLoadResult LoadGLTF(BASE_NS::array_view<uint8_t const> data) = 0;

    /** Save GLTF data to file.
     *  @param ecs ECS instance from which data is exported as a glTF file.
     *  @param uri URI pointing to the glTF file.
     *  @return True if the data could be saved, false otherwise.
     */
    virtual bool SaveGLTF(CORE_NS::IEcs& ecs, BASE_NS::string_view uri) = 0;

    /** Create glTF file importer that builds 3D resources from glTF data.
     *  The importer will create a thread pool where import task are executed.
     *  @param ecs ECS that contains all required subsystems that are needed for resource creation.
     *  @return glTF importer instance.
     */
    virtual IGLTF2Importer::Ptr CreateGLTF2Importer(CORE_NS::IEcs& ecs) = 0;

    /** Create glTF file importer that builds 3D resources from glTF data.
     *  @param ecs ECS that contains all required subsystems that are needed for resource creation.
     *  @param pool Importer will use the given thread pool instead of creating its own.
     *  @return glTF importer instance.
     */
    virtual IGLTF2Importer::Ptr CreateGLTF2Importer(CORE_NS::IEcs& ecs, CORE_NS::IThreadPool& pool) = 0;

    /** Import glTF scene to Ecs using pre-imported glTF resources.
     *  @param sceneIndex Index of scene to import.
     *  @param gltfData Pre-loaded glTF data.
     *  @param gltfImportData Structure that contains glTF resource handles related to imported glTF.
     *  @param ecs Ecs structure that receives the imported entities and components.
     *  @param rootEntity Root entity for imported data.
     *  @param flags Import flags to filter out which components are imported.
     *  @return Scene root entity.
     */
    virtual CORE_NS::Entity ImportGltfScene(size_t sceneIndex, const IGLTFData& gltfData,
        const GLTFResourceData& gltfImportData, CORE_NS::IEcs& ecs, CORE_NS::Entity rootEntity = {},
        GltfSceneImportFlags flags = CORE_GLTF_IMPORT_COMPONENT_FLAG_BITS_ALL) = 0;

protected:
    IGltf2() = default;
    virtual ~IGltf2() = default;
};
/** @} */
CORE3D_END_NAMESPACE()

#endif // API_3D_GLTF_GLTF_H
