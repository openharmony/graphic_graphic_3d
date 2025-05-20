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

#ifndef SCENE_SRC_APPLICATION_CONTEXT_H
#define SCENE_SRC_APPLICATION_CONTEXT_H

#include <scene/interface/intf_application_context.h>

#include <meta/ext/object.h>

#include "resource/resource_types.h"

SCENE_BEGIN_NAMESPACE()

class ApplicationContext : public META_NS::IntroduceInterfaces<META_NS::MetaObject, IApplicationContext> {
    META_OBJECT(ApplicationContext, ClassId::ApplicationContext, IntroduceInterfaces)
public:
    bool Initialize(const ApplicationContextInfo& info) override
    {
        bool res = !context_;
        if (res) {
            context_ = META_NS::GetObjectRegistry().Create<IRenderContext>(ClassId::RenderContext);
            if (context_) {
                context_->Initialize(
                    info.renderContext, info.engineTaskQueue, info.applicationTaskQueue, info.resourceManager);
                if (auto setter = interface_cast<IApplicationContextSetter>(context_)) {
                    setter->SetApplicationContext(GetSelf<IApplicationContext>());
                }
            }
            defaultSceneOptions_ = info.defaultSceneOptions;
            auto args = CreateRenderContextArg(context_);
            if (args) {
                if (auto op = META_NS::ConstructProperty<SceneOptions>("Options", defaultSceneOptions_)) {
                    args->AddProperty(op);
                }
            }
            RegisterResourceTypes(context_->GetResources(), args);
            sceneManager_ = META_NS::GetObjectRegistry().Create<ISceneManager>(ClassId::SceneManager, args);
        }
        return res;
    }

    IRenderContext::Ptr GetRenderContext() const override
    {
        return context_;
    }
    ISceneManager::Ptr GetSceneManager() const override
    {
        return sceneManager_;
    }
    SceneOptions GetDefaultSceneOptions() const override
    {
        return defaultSceneOptions_;
    }

private:
    IRenderContext::Ptr context_;
    ISceneManager::Ptr sceneManager_;
    SceneOptions defaultSceneOptions_;
};

SCENE_END_NAMESPACE()

#endif
