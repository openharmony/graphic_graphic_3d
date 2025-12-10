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

#ifndef SCENE_SRC_RENDER_CONTEXT_H
#define SCENE_SRC_RENDER_CONTEXT_H

#include <scene/interface/intf_application_context.h>
#include <scene/interface/intf_render_context.h>

#include <meta/ext/object.h>
#include <meta/interface/interface_helpers.h>

SCENE_BEGIN_NAMESPACE()

class RenderContext : public META_NS::IntroduceInterfaces<META_NS::BaseObject, IRenderContext,
                          IApplicationContextProvider, IApplicationContextSetter> {
    META_OBJECT(RenderContext, ClassId::RenderContext, IntroduceInterfaces)
public:
    bool Initialize(BASE_NS::shared_ptr<RENDER_NS::IRenderContext> rc, META_NS::ITaskQueue::Ptr rq,
        META_NS::ITaskQueue::Ptr aq, CORE_NS::IResourceManager::Ptr r) override;
    BASE_NS::shared_ptr<RENDER_NS::IRenderContext> GetRenderer() const override;
    META_NS::ITaskQueue::Ptr GetRenderQueue() const override;
    META_NS::ITaskQueue::Ptr GetApplicationQueue() const override;
    CORE_NS::IResourceManager::Ptr GetResources() const override;
    Counters GetCounters() const override;

protected: // IApplicationContextProvider
    IApplicationContext::ConstPtr GetApplicationContext() const override;

protected: // IApplicationContextSetter
    void SetApplicationContext(const IApplicationContext::ConstPtr& context) override;

private:
    BASE_NS::shared_ptr<RENDER_NS::IRenderContext> rcontext;
    META_NS::ITaskQueue::Ptr renderQ;
    META_NS::ITaskQueue::Ptr appQ;
    CORE_NS::IResourceManager::Ptr res;
    IApplicationContext::ConstWeakPtr context_;
};

SCENE_END_NAMESPACE()

#endif
