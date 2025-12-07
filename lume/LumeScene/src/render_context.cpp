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

#include <3d/ecs/systems/intf_node_system.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_renderer.h>

#include "scene/ext/intf_ecs_context.h"
#include "scene/ext/intf_internal_scene.h"
#include "scene/ext/scene_debug_info.h"

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
    uint32_t count {};
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
    uint32_t count { 1 }; // the node itself
    for (auto child : root->GetChildren()) {
        count += GetChildCount(child);
    }
    return count;
}

/// Return entity count of ecs
uint32_t GetNodeCount(CORE_NS::IEcs& ecs)
{
    uint32_t count {};
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

IRenderContext::Counters RenderContext::GetCounters() const
{
    IRenderContext::Counters counters;
    RunDirectlyOrInTask([&]() {
        // Find all ClassId::Scene instances
        for (const auto& instance : META_NS::GetObjectRegistry().GetAllObjectInstances()) {
            // Only require that the object implements IScene
            const auto scene = interface_cast<IScene>(instance);
            if (scene) {
                counters.totalSceneCount++;
                const auto internal = scene->GetInternalScene();
                if (internal) {
                    if (rcontext.get() == &internal->GetRenderContext()) {
                        counters.scenes.emplace_back(BASE_NS::move(GetSceneInfo(*internal)));
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
        counters.resourceCount = res ? res->GetResourceInfos().size() : 0;
    });
    return counters;
}

SCENE_END_NAMESPACE()
