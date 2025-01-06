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

#include "scene_manager.h"

#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/util.h>

#include <render/intf_render_context.h>

#include <meta/api/task_queue.h>

#include "asset/asset_object.h"
#include "util.h"

SCENE_BEGIN_NAMESPACE()

bool SceneManager::Build(const META_NS::IMetadata::Ptr& d)
{
    bool res = Super::Build(d);
    if (res) {
        if (d) {
            using META_NS::SharedPtrIInterface;
            BASE_NS::shared_ptr<RENDER_NS::IRenderContext> rc =
                GetInterfaceBuildArg<RENDER_NS::IRenderContext>(d, "RenderContext");
            engineQueue_ = GetInterfaceBuildArg<META_NS::ITaskQueue>(d, "EngineQueue");
            META_NS::ITaskQueue::Ptr app = GetInterfaceBuildArg<META_NS::ITaskQueue>(d, "AppQueue");
            if (!engineQueue_ || !app || !rc) {
                CORE_LOG_E("Invalid arguments to construct SceneManager");
                return false;
            }
        } else {
            CORE_LOG_E("SceneManager requires arguments when constructed");
            return false;
        }
        context_ = d;
    }
    return res;
}

Future<IScene::Ptr> SceneManager::CreateScene()
{
    return META_NS::AddFutureTaskOrRunDirectly(engineQueue_, [context = context_] {
        if (auto scene = META_NS::GetObjectRegistry().Create<IScene>(SCENE_NS::ClassId::Scene, context)) {
            auto& ecs = scene->GetInternalScene()->GetEcsContext();
            if (ecs.CreateUnnamedRootNode()) {
                return scene;
            }
            CORE_LOG_E("Failed to create root node");
        }
        return SCENE_NS::IScene::Ptr {};
    });
}

static IScene::Ptr Load(const IScene::Ptr& scene, BASE_NS::string_view uri)
{
    CORE_LOG_I("Loading scene: '%s'", BASE_NS::string(uri).c_str());
    if (uri == "" || uri == "scene://empty") {
        return scene;
    }

    if (auto assets = META_NS::GetObjectRegistry().Create<IAssetObject>(ClassId::AssetObject)) {
        if (assets->Load(scene, uri)) {
            if (auto att = interface_cast<META_NS::IAttach>(scene)) {
                att->Attach(assets);
            }

            scene->GetInternalScene()->Update();

            return scene;
        }
    }
    return {};
}

Future<IScene::Ptr> SceneManager::CreateScene(BASE_NS::string_view uri)
{
    return META_NS::AddFutureTaskOrRunDirectly(engineQueue_, [path = BASE_NS::string(uri), context = context_] {
        if (auto s = META_NS::GetObjectRegistry().Create<IScene>(SCENE_NS::ClassId::Scene, context)) {
            return Load(s, path);
        }
        return IScene::Ptr {};
    });
}

SCENE_END_NAMESPACE()