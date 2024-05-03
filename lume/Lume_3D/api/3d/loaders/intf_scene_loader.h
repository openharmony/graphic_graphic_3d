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

#ifndef API_3D_LOADERS_ISCENE_LOADER_H
#define API_3D_LOADERS_ISCENE_LOADER_H

#include <3d/ecs/components/mesh_component.h>
#include <3d/namespace.h>
#include <base/containers/array_view.h>
#include <base/containers/refcnt_ptr.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <core/ecs/entity.h>
#include <core/ecs/entity_reference.h>
#include <core/plugin/intf_interface.h>

CORE_BEGIN_NAMESPACE()
class IEcs;
class IThreadPool;
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
/** No scene explicitly defined in import options. */
const unsigned long CORE_SCENE_INVALID_INDEX = 0x7FFFFFFF;

class ISceneData : public CORE_NS::IInterface {
public:
    static constexpr auto UID = BASE_NS::Uid { "eb6381c1-36a3-4709-8031-c37c1b9cd76e" };
    using Ptr = BASE_NS::refcnt_ptr<ISceneData>;

    /** Retrieve default scene index. */
    virtual size_t GetDefaultSceneIndex() const = 0;

    /** Retrieve number of scenes in this scene file.
     *  @return Number of scenes available.
     */
    virtual size_t GetSceneCount() const = 0;

protected:
    ISceneData() = default;
    virtual ~ISceneData() = default;
};

/** Access to imported resources. */
struct ResourceData {
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
enum ResourceImportFlagBits {
    /** Sampler */
    CORE_IMPORT_RESOURCE_SAMPLER = 0x00000001,
    /** Image */
    CORE_IMPORT_RESOURCE_IMAGE = 0x00000002,
    /** Texture */
    CORE_IMPORT_RESOURCE_TEXTURE = 0x00000004,
    /** Material */
    CORE_IMPORT_RESOURCE_MATERIAL = 0x00000008,
    /** Mesh */
    CORE_IMPORT_RESOURCE_MESH = 0x00000010,
    /** Skin */
    CORE_IMPORT_RESOURCE_SKIN = 0x00000020,
    /** Animation */
    CORE_IMPORT_RESOURCE_ANIMATION = 0x00000040,
    /** Skip resources that are not referenced and do not import them. */
    CORE_IMPORT_RESOURCE_SKIP_UNUSED = 0x00000080,
    /** Keep mesh data for CPU access. Allowing CPU access increases memory usage. */
    CORE_IMPORT_RESOURCE_MESH_CPU_ACCESS = 0x00000100,
    /** All flags bits */
    CORE_IMPORT_RESOURCE_FLAG_BITS_ALL = 0x7FFFFEFF
};

/** Container for flags for resource import. */
using ResourceImportFlags = uint32_t;

/** Flags for scene / ecs component import. */
enum SceneImportFlagBits {
    /** Environment */
    CORE_IMPORT_COMPONENT_ENVIRONMENT = 0x00000001,
    /** Mesh */
    CORE_IMPORT_COMPONENT_MESH = 0x00000002,
    /** Camera */
    CORE_IMPORT_COMPONENT_CAMERA = 0x00000004,
    /** Skin */
    CORE_IMPORT_COMPONENT_SKIN = 0x00000008,
    /** Light */
    CORE_IMPORT_COMPONENT_LIGHT = 0x00000010,
    /** Morph */
    CORE_IMPORT_COMPONENT_MORPH = 0x00000020,
    /** All flag bits */
    CORE_IMPORT_COMPONENT_FLAG_BITS_ALL = 0x7FFFFFFF
};

/** Container for  scene import flag bits */
using SceneImportFlags = uint32_t;

struct MeshData {
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
    /** Data of each imported mesh. They are in the same order as in ResourceData::meshes. */
    BASE_NS::vector<Mesh> meshes;
};

class ISceneImporter : public CORE_NS::IInterface {
public:
    static constexpr auto UID = BASE_NS::Uid { "6dd26fca-9ef1-40f1-ba67-1bbcc1740885" };

    using Ptr = BASE_NS::refcnt_ptr<ISceneImporter>;

    struct Result {
        /** Indicates, whether the import operation is successful. */
        int32_t error { 0 };

        /** In case of import error, contains the description of the error. */
        BASE_NS::string message;

        /** Imported resources. */
        ResourceData data;
    };

    /** Listener for import events. */
    class Listener {
    public:
        virtual ~Listener() = default;

        /** On import started */
        virtual void OnImportStarted() = 0;
        /** On import progressed */
        virtual void OnImportProgressed(size_t taskIndex, size_t taskCount) = 0;
        /** On import finished */
        virtual void OnImportFinished() = 0;
    };

    /** Import resources synchronously. The previous imported data will be discarded.
     * @param data Scene data returned by the loader.
     * @param flags Flags for scene / ecs component import.
     */
    virtual void ImportResources(const ISceneData::Ptr& data, ResourceImportFlags flags) = 0;

    /** Import resource data asynchronously. The previous imported data will be discarded. Import will mostly happen in
     * a threadpool, but any access to the ECS must be done synchronized. Typically the user calls Execute() from the
     * rendering thread each frame.
     * @param data Scene data returned by the loader.
     * @param flags Flags for scene / ecs component import.
     * @param listener Listener which receives updates of the imports progress.
     */
    virtual void ImportResources(const ISceneData::Ptr& data, ResourceImportFlags flags, Listener* listener) = 0;

    /** Advances the import process when performing asynchronous import. Needs to be called synchronized with other ECS
     * usage.
     *  @param timeBudget Time budget for resource import in microseconds, if 0 all available work will be executed
     *  during this frame.
     * @return True if the import was compled.
     */
    virtual bool Execute(uint32_t timeBudget) = 0;

    /** Cancel import operation, this does not discard imported data. */
    virtual void Cancel() = 0;

    /** Returns true when import process is completed. */
    virtual bool IsCompleted() const = 0;

    /** Returns imported data and success of the whole operation. The imported resources are reference counted and the
     * importer holds references until a new import is started or the imported is destroyed. Therefore a copy of
     * ImportResult::ResourceData (or selected EntityReferences) should be stored. */
    virtual const Result& GetResult() const = 0;

    /** Returns CPU accessible mesh data. Data is available when CORE_IMPORT_RESOURCE_MESH_CPU_ACCESS was included
     * in import flags. Unless copied, the data is valid until a new import is started or the imported is destroyed. */
    virtual const MeshData& GetMeshData() const = 0;

    /** Import scene to ECS using pre-imported resources.
     *  @param sceneIndex Index of scene to import.
     *  @return Scene root entity.
     */
    virtual CORE_NS::Entity ImportScene(size_t sceneIndex) = 0;

    /** Import scene to ECS using pre-imported resources.
     *  @param sceneIndex Index of scene to import.
     *  @param flags Import flags to filter out which components are imported.
     *  @return Scene root entity.
     */
    virtual CORE_NS::Entity ImportScene(size_t sceneIndex, SceneImportFlags flags) = 0;

    /** Import scene to ECS using pre-imported resources.
     *  @param sceneIndex Index of scene to import.
     *  @param parentEntity Scene will be added as a child of the parent entity.
     *  @return Scene root entity.
     */
    virtual CORE_NS::Entity ImportScene(size_t sceneIndex, CORE_NS::Entity parentEntity) = 0;

    /** Import scene to ECS using pre-imported resources.
     *  @param sceneIndex Index of scene to import.
     *  @param parentEntity Scene will be added as a child of the parent entity.
     *  @param flags Import flags to filter out which components are imported.
     *  @return Scene root entity.
     */
    virtual CORE_NS::Entity ImportScene(size_t sceneIndex, CORE_NS::Entity parentEntity, SceneImportFlags flags) = 0;

protected:
};

class ISceneLoader : public CORE_NS::IInterface {
public:
    static constexpr auto UID = BASE_NS::Uid { "61997694-aa64-4753-9d01-17d18aad4822" };

    using Ptr = BASE_NS::refcnt_ptr<ISceneLoader>;

    struct Result {
        /** Indicates, whether the loading was successful. */
        int32_t error { 0 };

        /** In case of an error, contains the description of the error. */
        BASE_NS::string message;

        /** Loaded data. */
        ISceneData::Ptr data;
    };

    /** Load scene data from URI.
     * @param uri URI pointing to the scene data.
     * @return If the scene data could be parsed LoadResult::result will be zero and LoadResult::data is valid. If
     * parsing fails LoadResult::result will be a non-zero, loader specific error code and LoadResult::error will give
     * more details on the failure.
     */
    virtual Result Load(BASE_NS::string_view uri) = 0;

    /** Create an importer that builds 3D resources and scenes from loaded scene data.
     *  The importer will create a thread pool where import task are executed.
     * @param ecs ECS that contains all required subsystems that are needed for resource creation.
     * @return Scene importer instance.
     */
    virtual ISceneImporter::Ptr CreateSceneImporter(CORE_NS::IEcs& ecs) = 0;

    /** Create an importer that builds 3D resources and scenes from loaded scene data.
     * @param ecs ECS that contains all required subsystems that are needed for resource creation.
     * @param pool Importer will use the given thread pool instead of creating its own.
     * @return Scene importer instance.
     */
    virtual ISceneImporter::Ptr CreateSceneImporter(CORE_NS::IEcs& ecs, CORE_NS::IThreadPool& pool) = 0;

    /** Returns a list of extensions the loader supports.
     * @return List of supported file extensions.
     */
    virtual BASE_NS::array_view<const BASE_NS::string_view> GetSupportedExtensions() const = 0;

protected:
    ISceneLoader() = default;
    virtual ~ISceneLoader() = default;
};
CORE3D_END_NAMESPACE()
#endif // API_3D_LOADERS_ISCENE_LOADER_H