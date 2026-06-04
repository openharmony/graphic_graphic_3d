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

#include "render_context.h"

#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/scene_debug_info.h>
#include <scene/ext/scene_utils.h>

#include <3d/ecs/systems/intf_node_system.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_renderer.h>

#include <meta/interface/resource/intf_resource_query.h>

SCENE_BEGIN_NAMESPACE()

bool RenderContext::Initialize(BASE_NS::shared_ptr<RENDER_NS::IRenderContext> rc, META_NS::ITaskQueue::Ptr rq,
    META_NS::ITaskQueue::Ptr aq, CORE_NS::IResourceManager::Ptr r)
{
    bool result = !rcontext;
    if (result) {
        rcontext = BASE_NS::move(rc);
        renderQ = BASE_NS::move(rq);
        appQ = BASE_NS::move(aq);
        res = BASE_NS::move(r);
    }
    return result;
}
BASE_NS::shared_ptr<RENDER_NS::IRenderContext> RenderContext::GetRenderer() const
{
    return rcontext;
}
META_NS::ITaskQueue::Ptr RenderContext::GetRenderQueue() const
{
    return renderQ;
}
META_NS::ITaskQueue::Ptr RenderContext::GetApplicationQueue() const
{
    return appQ;
}
CORE_NS::IResourceManager::Ptr RenderContext::GetResources() const
{
    return res;
}
IApplicationContext::ConstPtr RenderContext::GetApplicationContext() const
{
    return context_.lock();
}
void RenderContext::SetApplicationContext(const IApplicationContext::ConstPtr& context)
{
    context_ = context;
}

/// Return entity count of ecs
uint32_t GetEntityCount(CORE_NS::IEcs& ecs)
{
    auto& em = ecs.GetEntityManager();
    auto it = em.Begin();
    if (!it || it->Compare(em.End())) {
        return 0;
    }
    uint32_t count{};
    do {
        count++;
    } while (it->Next());
    return count;
}

/// Get count of node+its children
uint32_t GetChildCount(CORE3D_NS::ISceneNode* root)
{
    if (!root) {
        return 0;
    }
    uint32_t count{1};  // the node itself
    for (auto child : root->GetChildren()) {
        count += GetChildCount(child);
    }
    return count;
}

/// Return entity count of ecs
uint32_t GetNodeCount(CORE_NS::IEcs& ecs)
{
    uint32_t count{};
    auto ns = CORE_NS::GetSystem<CORE3D_NS::INodeSystem>(ecs);
    if (ns) {
        count = GetChildCount(&ns->GetRootNode());
    }
    return count;
}

/// Return SceneInfo from based on scene instance
IRenderContext::Counters::SceneInfo GetSceneInfo(IInternalScene& scene)
{
    IRenderContext::Counters::SceneInfo info;
    const auto ecs = scene.GetEcsContext().GetNativeEcs();
    if (!ecs) {
        return info;
    }
    info.entityCount = GetEntityCount(*ecs);
    info.nodeCount = GetNodeCount(*ecs);
    auto di = scene.GetDebugInfo();
    info.nodeObjectCount = di.nodeObjectsAlive;
    return info;
}

BASE_NS::vector<CORE_NS::ResourceInfo> GetResourceInfos(const IScene::Ptr& is)
{
    auto resourceManager = GetResourceManager(is);
    return resourceManager ? resourceManager->GetResourceInfos(is) : BASE_NS::vector<CORE_NS::ResourceInfo>{};
}

namespace {
// ImporterDeferred scope is thread-local: only the call stack that pushed
// it should see it. Stored as a file-static thread_local accessed via the
// IRenderContext methods. Keeping the storage outside of any per-context
// member is correct here — the deferred-load pending queue
// (RenderResourceManager::WaitAllPendingLoads) is also process-wide, so
// having the routing flag share that scope is consistent.
int32_t& ImporterDeferredDepthTLS()
{
    static thread_local int32_t depth = 0;
    return depth;
}
}  // namespace

void RenderContext::AdjustScope(ScopeType types, int32_t delta)
{
    if (Any(types & ScopeType::SceneLoad)) {
        sceneLoadDepth_.fetch_add(delta, std::memory_order_acq_rel);
    }
    if (Any(types & ScopeType::ImporterDeferred)) {
        ImporterDeferredDepthTLS() += delta;
    }
}

bool RenderContext::IsInScope(ScopeType type) const
{
    if (type == ScopeType::SceneLoad) {
        return sceneLoadDepth_.load(std::memory_order_acquire) > 0;
    }
    if (type == ScopeType::ImporterDeferred) {
        return ImporterDeferredDepthTLS() > 0;
    }
    return false;
}

IRenderContext::Counters RenderContext::GetCounters() const
{
    IRenderContext::Counters counters;
    RunDirectlyOrInTask([&]() {
        // Find all ClassId::Scene instances
        for (const auto& instance : META_NS::GetObjectRegistry().GetAllObjectInstances()) {
            // Only require that the object implements IScene
            const auto scene = interface_pointer_cast<IScene>(instance);
            if (scene) {
                counters.totalSceneCount++;
                const auto internal = scene->GetInternalScene();
                if (internal) {
                    if (rcontext.get() == &internal->GetRenderContext()) {
                        auto& si = counters.scenes.emplace_back(BASE_NS::move(GetSceneInfo(*internal)));
                        si.resources = BASE_NS::move(GetResourceInfos(scene));
                    }
                }
            }
        }
        // Get gpu resource counters
        if (rcontext) {
            auto& man = rcontext->GetDevice().GetGpuResourceManager();
            counters.handles.bufferHandleCount = man.GetBufferHandles().size();
            counters.handles.imageHandleCount = man.GetImageHandles().size();
            counters.handles.samplerHandleCount = man.GetSamplerHandles().size();
        }
        // Get resource count
        if (auto qi = interface_cast<META_NS::IResourceQuery>(res)) {
            counters.resourceCount = qi->GetResourceCount();
        }
    });
    return counters;
}

SCENE_END_NAMESPACE()
