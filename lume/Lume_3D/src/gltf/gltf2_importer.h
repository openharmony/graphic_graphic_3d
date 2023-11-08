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

#ifndef CORE__GLTF__GLTF2_IMPORTER_H
#define CORE__GLTF__GLTF2_IMPORTER_H

#include <atomic>
#include <mutex>

#include <3d/gltf/gltf.h>
#include <base/containers/string.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_ecs.h>
#include <core/namespace.h>
#include <core/threading/intf_thread_pool.h>

CORE_BEGIN_NAMESPACE()
class IEngine;
class IImageLoaderManager;
class IPerformanceDataManager;
CORE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
class IDevice;
class IGpuResourceManager;
class IRenderContext;
class IShaderManager;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class IRenderHandleComponentManager;
class IMaterialComponentManager;
class IMeshComponentManager;
class INameComponentManager;
class IUriComponentManager;
class IAnimationInputComponentManager;
class IAnimationOutputComponentManager;

namespace GLTF2 {
class Data;
struct Accessor;
struct AnimationTrack;
enum class ImportPhase {
    BUFFERS = 0,
    SAMPLERS,
    IMAGES,
    TEXTURES,
    MATERIALS,
    ANIMATION_SAMPLERS,
    ANIMATIONS,
    SKINS,
    MESHES,
    FINISHED
};

// A class that executes GLTF load and import operation over multiple frames.
class GLTF2Importer final : public IGLTF2Importer {
public:
    GLTF2Importer(CORE_NS::IEngine& engine, RENDER_NS::IRenderContext& renderContext, CORE_NS::IEcs& ecs);
    GLTF2Importer(CORE_NS::IEngine& engine, RENDER_NS::IRenderContext& renderContext, CORE_NS::IEcs& ecs,
        CORE_NS::IThreadPool& pool);
    ~GLTF2Importer() override;

    void ImportGLTF(const IGLTFData& data, GltfResourceImportFlags flags) override;
    void ImportGLTFAsync(const IGLTFData& data, GltfResourceImportFlags flags, Listener* listener) override;
    bool Execute(uint32_t timeBudget) override;

    void Cancel() override;
    bool IsCompleted() const override;

    const GLTFImportResult& GetResult() const override;

    struct DefaultMaterialShaderData {
        struct SingleShaderData {
            CORE_NS::EntityReference shader;
            CORE_NS::EntityReference gfxState;
            CORE_NS::EntityReference gfxStateDoubleSided;
        };
        SingleShaderData opaque;
        SingleShaderData blend;
        SingleShaderData depth;
    };

protected:
    void Destroy() override;

private:
    struct ImporterTask;
    template<typename T>
    struct GatheredDataTask;
    template<typename Component>
    struct ComponentTaskData;
    struct AnimationTaskData;

    class ImportThreadTask;
    class GatherThreadTask;

    void Prepare();
    void PrepareBufferTasks();
    void PrepareSamplerTasks();
    void PrepareImageTasks();
    void PrepareImageBasedLightTasks();
    void PrepareMaterialTasks();
    void PrepareAnimationTasks();
    void PrepareSkinTasks();
    void PrepareMeshTasks();
    template<typename T>
    GatheredDataTask<T>* PrepareAnimationInputTask(BASE_NS::unordered_map<GLTF2::Accessor*, GatheredDataTask<T>*>&,
        const GLTF2::AnimationTrack&, IAnimationInputComponentManager*);
    template<typename T>
    GatheredDataTask<T>* PrepareAnimationOutputTask(BASE_NS::unordered_map<GLTF2::Accessor*, GatheredDataTask<T>*>&,
        const GLTF2::AnimationTrack&, IAnimationOutputComponentManager*);
    void QueueImage(size_t i, BASE_NS::string&& uri, BASE_NS::string&& name);
    void ResolveReferencedImages(BASE_NS::vector<bool>& imageLoadingRequred);

    void QueueTask(BASE_NS::unique_ptr<ImporterTask>&& task);
    bool ProgressTask(ImporterTask& task);
    void Gather(ImporterTask& task);
    void Import(ImporterTask& task);
    void CompleteTask(ImporterTask& task);

    void StartPhase(ImportPhase phase);
    void HandleGatherTasks();
    void HandleImportTasks();

    ImporterTask* FindTaskById(uint64_t id);

    ImportPhase phase_ { ImportPhase::BUFFERS };
    BASE_NS::vector<BASE_NS::unique_ptr<ImporterTask>> tasks_;

    CORE_NS::IEngine& engine_;
    RENDER_NS::IRenderContext& renderContext_;
    RENDER_NS::IDevice& device_;
    RENDER_NS::IGpuResourceManager& gpuResourceManager_;
    CORE_NS::IEcs::Ptr ecs_;
    IRenderHandleComponentManager& gpuHandleManager_;
    IMaterialComponentManager& materialManager_;
    IMeshComponentManager& meshManager_;
    INameComponentManager& nameManager_;
    IUriComponentManager& uriManager_;

    // assigned to material in import
    DefaultMaterialShaderData dmShaderData_;

    const Data* data_ { nullptr };

    CORE_NS::IThreadPool::Ptr threadPool_;
    CORE_NS::IDispatcherTaskQueue::Ptr mainThreadQueue_;
    Listener* listener_ { nullptr };

    GltfResourceImportFlags flags_ { CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL };
    GLTFImportResult result_;

    std::mutex gatherTasksLock_;
    BASE_NS::vector<uint64_t> finishedGatherTasks_;
    BASE_NS::vector<CORE_NS::IThreadPool::IResult::Ptr> gatherTaskResults_;

    size_t pendingGatherTasks_ { 0 };
    size_t pendingImportTasks_ { 0 };
    size_t completedTasks_ { 0 };

    std::atomic_bool cancelled_ { false };
};
} // namespace GLTF2

CORE_NS::Entity ImportScene(RENDER_NS::IDevice& device, size_t sceneIndex, const GLTF2::Data& data,
    const GLTFResourceData& gltfResourceData, CORE_NS::IEcs& ecs, CORE_NS::Entity rootEntity,
    GltfSceneImportFlags flags);
CORE3D_END_NAMESPACE()

#endif // CORE__GLTF__GLTF2_IMPORTER_H
