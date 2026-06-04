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
#include "render_resource_manager.h"

#include <mutex>
#include <scene/ext/util.h>
#include <vector>

#include <core/image/intf_image_loader_manager.h>
#include <core/implementation_uids.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_class_register.h>
#include <core/threading/intf_thread_pool.h>
#include <core/util/parallel_sort.h>
#include <render/device/intf_gpu_resource_manager.h>

#include <meta/interface/resource/intf_resource.h>

#include "../perf/cpu_perf_scope.h"

SCENE_BEGIN_NAMESPACE()

namespace {

struct DeferredLoad {
    IImage::Ptr image;
    BASE_NS::string path;
    ImageLoadInfo info;
    IRenderContext::WeakPtr context;
    CORE_NS::IImageLoaderManager::LoadResult loadResult;
    CORE_NS::IThreadPool::IResult::Ptr decodeTask;
};

std::mutex& SharedDeferredLoadPoolMutex()
{
    static std::mutex m;
    return m;
}

// Storage accessor: never creates the pool. Used by Shutdown to release
// without forcing lazy init when no resources were ever loaded.
CORE_NS::IThreadPool::Ptr& SharedDeferredLoadPoolStorage()
{
    static CORE_NS::IThreadPool::Ptr pool;
    return pool;
}

CORE_NS::IThreadPool::Ptr SharedDeferredLoadPool()
{
    std::lock_guard<std::mutex> lk(SharedDeferredLoadPoolMutex());
    auto& pool = SharedDeferredLoadPoolStorage();
    if (!pool) {
        if (auto factory = CORE_NS::GetInstance<CORE_NS::ITaskQueueFactory>(CORE_NS::UID_TASK_QUEUE_FACTORY)) {
            // Use core count / 2 threads for resource loading.
            auto size = BASE_NS::Math::max(1u, factory->GetNumberOfCores() / 2u);
            pool = factory->CreateThreadPool(size);
        }
    }
    return pool;
}

std::mutex& PendingMutex()
{
    static std::mutex m;
    return m;
}

std::vector<BASE_NS::shared_ptr<DeferredLoad>>& PendingLoads()
{
    static std::vector<BASE_NS::shared_ptr<DeferredLoad>> v;
    return v;
}
}  // namespace

bool RenderResourceManager::Build(const META_NS::IMetadata::Ptr& d)
{
    bool res = Super::Build(d);
    if (res) {
        context_ = GetInterfaceBuildArg<IRenderContext>(d, "RenderContext");
        if (!context_) {
            CORE_LOG_E("Invalid arguments to construct RenderResourceManager");
            return false;
        }
    }
    return res;
}

static RENDER_NS::GpuImageDesc SetImageInfoFlags(const ImageInfo& info, RENDER_NS::GpuImageDesc& desc)
{
    desc.usageFlags |= static_cast<uint32_t>(info.usageFlags);
    desc.memoryPropertyFlags |= static_cast<uint32_t>(info.memoryFlags);
    desc.engineCreationFlags |= static_cast<uint32_t>(info.creationFlags);
    return desc;
}

static RENDER_NS::GpuImageDesc ConvertToImageDesc(const ImageCreateInfo& info)
{
    RENDER_NS::GpuImageDesc desc;

    desc.width = info.size.x;
    desc.height = info.size.y;
    desc.format = info.format;

    return SetImageInfoFlags(info.info, desc);
}

static IImage::Ptr ConstructImage(const IRenderContext::Ptr& context, RENDER_NS::RenderHandleReference handle)
{
    auto image = META_NS::GetObjectRegistry().Create<IImage>(ClassId::Image, CreateRenderContextArg(context));
    if (auto i = interface_cast<IRenderResource>(image)) {
        i->SetRenderHandle(BASE_NS::move(handle));
    }
    return image;
}

Future<IImage::Ptr> RenderResourceManager::CreateImage(const ImageCreateInfo& info, BASE_NS::vector<uint8_t> d)
{
    return context_->AddTaskOrRunDirectly(
        [ctxWeak = BASE_NS::weak_ptr(context_), desc = ConvertToImageDesc(info), data = BASE_NS::move(d)] {
            IImage::Ptr res;

            auto ctx = ctxWeak.lock();
            if (!ctx) {
                return res;
            }

            auto& gpuResMan = ctx->GetRenderer()->GetDevice().GetGpuResourceManager();
            RENDER_NS::RenderHandleReference handle;
            if (data.empty()) {
                handle = gpuResMan.Create(desc);
            } else {
                handle = gpuResMan.Create(desc, data);
            }
            return ConstructImage(ctx, BASE_NS::move(handle));
        });
}
Future<IImage::Ptr> RenderResourceManager::LoadImage(BASE_NS::string_view uri, const ImageLoadInfo& info)
{
    return context_->AddTaskOrRunDirectly(
        [ctxWeak = BASE_NS::weak_ptr(context_), info = info, path = BASE_NS::string(uri)] {
            SCENE_CPU_PERF_SCOPE("LoadImage", path);
            IImage::Ptr res;

            auto ctx = ctxWeak.lock();
            if (!ctx) {
                return res;
            }

            auto& gpuResMan = ctx->GetRenderer()->GetDevice().GetGpuResourceManager();
            auto& loader = ctx->GetRenderer()->GetEngine().GetImageLoaderManager();
            auto loadResult = loader.LoadImage(path, static_cast<uint32_t>(info.loadFlags));
            if (!loadResult.success) {
                CORE_LOG_E("Failed to load image (%s): %s", path.c_str(), loadResult.error);
                return res;
            }

            auto gpuDesc = gpuResMan.CreateGpuImageDesc(loadResult.image->GetImageDesc());
            SetImageInfoFlags(info.info, gpuDesc);
            auto handle = gpuResMan.Create(path, gpuDesc, std::move(loadResult.image));
            return ConstructImage(ctx, BASE_NS::move(handle));
        });
}

static IShader::Ptr ConstructShader(const IRenderContext::Ptr& context, RENDER_NS::RenderHandleReference handle)
{
    auto shader = META_NS::GetObjectRegistry().Create<IShader>(ClassId::Shader, CreateRenderContextArg(context));
    if (auto i = interface_cast<IShaderState>(shader)) {
        auto& man = context->GetRenderer()->GetDevice().GetShaderManager();
        i->SetShaderState(handle, man.GetGraphicsStateHandleByShaderHandle(handle));
    }
    return shader;
}

IImage::Ptr RenderResourceManager::LoadImageDeferred(BASE_NS::string_view uri, const ImageLoadInfo& info)
{
    auto pool = SharedDeferredLoadPool();
    if (!pool) {
        // Fallback if the engine has no task-queue factory: synchronous load.
        return LoadImage(uri, info).GetResult();
    }

    // Construct the shell synchronously. Its render handle is filled in by
    // WaitAllPendingLoads once the CPU decode completes and GPU create runs.
    auto image = META_NS::GetObjectRegistry().Create<IImage>(ClassId::Image, CreateRenderContextArg(context_));
    if (!image) {
        return nullptr;
    }

    auto entry = BASE_NS::make_shared<DeferredLoad>();
    entry->image = image;
    entry->path = BASE_NS::string(uri);
    entry->info = info;
    entry->context = context_;

    auto task = CORE_NS::CreateFunctionTask([entry]() {
        SCENE_CPU_PERF_SCOPE("LoadImage", entry->path);
        auto ctx = entry->context.lock();
        if (!ctx) {
            return;
        }
        auto& loader = ctx->GetRenderer()->GetEngine().GetImageLoaderManager();
        entry->loadResult = loader.LoadImage(entry->path, static_cast<uint32_t>(entry->info.loadFlags));
    });

    entry->decodeTask = pool->Push(BASE_NS::move(task));
    {
        std::lock_guard<std::mutex> lk(PendingMutex());
        PendingLoads().push_back(entry);
    }
    return image;
}

void RenderResourceManager::WaitAllPendingLoads()
{
    // Move the queue to a local so concurrent LoadImageDeferred calls from
    // other threads can keep enqueueing without being drained twice.
    std::vector<BASE_NS::shared_ptr<DeferredLoad>> local;
    {
        std::lock_guard<std::mutex> lk(PendingMutex());
        local.swap(PendingLoads());
    }

    // Phase 1: wait for all parallel CPU decodes (this is the ~90% work).
    for (auto& e : local) {
        if (e && e->decodeTask) {
            e->decodeTask->Wait();
        }
    }

    // Phase 2: do GPU-side create on the caller's thread. Scene-load drains
    // are called from the render queue, so gpuResMan.Create runs where it
    // always used to. No thread-safety assumption needed.
    for (auto& e : local) {
        if (!e) {
            continue;
        }
        if (!e->loadResult.success) {
            CORE_LOG_E("Failed to load image (%s): %s", e->path.c_str(), e->loadResult.error);
            continue;
        }
        auto ctx = e->context.lock();
        if (!ctx) {
            continue;
        }
        auto& gpuResMan = ctx->GetRenderer()->GetDevice().GetGpuResourceManager();
        auto gpuDesc = gpuResMan.CreateGpuImageDesc(e->loadResult.image->GetImageDesc());
        SetImageInfoFlags(e->info.info, gpuDesc);
        auto handle = gpuResMan.Create(e->path, gpuDesc, std::move(e->loadResult.image));
        if (auto i = interface_cast<IRenderResource>(e->image)) {
            i->SetRenderHandle(BASE_NS::move(handle));
        }
    }
}

void RenderResourceManager::Shutdown()
{
    // Drop any still-pending decode entries first. Their decode tasks are
    // running on the pool we're about to release, so wait for each to
    // finish; we deliberately skip the GPU-create phase (the engine is
    // tearing down — there is nothing left to render with).
    std::vector<BASE_NS::shared_ptr<DeferredLoad>> local;
    {
        std::lock_guard<std::mutex> lk(PendingMutex());
        local.swap(PendingLoads());
    }
    for (auto& e : local) {
        if (e && e->decodeTask) {
            e->decodeTask->Wait();
        }
    }
    local.clear();

    // Now release the pool. Its destructor signals workers to exit and
    // joins them; doing this while LumeEngine is still loaded means the
    // join runs through code that's still mapped. Use the storage accessor
    // so we don't lazily create a pool only to immediately destroy it when
    // no resources were ever loaded.
    std::lock_guard<std::mutex> lk(SharedDeferredLoadPoolMutex());
    SharedDeferredLoadPoolStorage().reset();
}

Future<IShader::Ptr> RenderResourceManager::LoadShader(BASE_NS::string_view uri)
{
    if (uri.empty()) {
        CORE_LOG_E("Cannot load shader from empty URI");
        return {};
    }
    return context_->AddTaskOrRunDirectly([ctxWeak = BASE_NS::weak_ptr(context_), path = BASE_NS::string(uri)] {
        SCENE_CPU_PERF_SCOPE("LoadShader", path);
        IShader::Ptr res;

        auto ctx = ctxWeak.lock();
        if (!ctx) {
            return res;
        }

        auto& man = ctx->GetRenderer()->GetDevice().GetShaderManager();
        // Skip LoadShaderFile if the shader is already registered: re-parsing a built-in
        // shader recreates its pipeline layout and overwrites the engine's render-slot
        // defaults, corrupting state for any material already bound to that slot.
        auto handle = man.GetShaderHandle(path);
        if (!handle) {
            man.LoadShaderFile(path);
            handle = man.GetShaderHandle(path);
        }
        if (handle) {
            res = ConstructShader(ctx, BASE_NS::move(handle));
        }

        return res;
    });
}

SCENE_END_NAMESPACE()
