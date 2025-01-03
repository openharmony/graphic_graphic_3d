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

#ifndef SCENE_SRC_SCENE_MANAGER_H
#define SCENE_SRC_SCENE_MANAGER_H

#include <scene/interface/intf_scene_manager.h>

#include <meta/ext/object_container.h>
#include <meta/interface/intf_task_queue.h>

SCENE_BEGIN_NAMESPACE()

class SceneManager : public META_NS::IntroduceInterfaces<META_NS::CommonObjectContainerFwd, ISceneManager> {
    META_OBJECT(SceneManager, ClassId::SceneManager, IntroduceInterfaces, META_NS::ClassId::ObjectContainer)
public:
    bool Build(const META_NS::IMetadata::Ptr&) override;

    Future<IScene::Ptr> CreateScene() override;
    Future<IScene::Ptr> CreateScene(BASE_NS::string_view uri) override;

private:
    META_NS::IMetadata::Ptr context_;
    META_NS::ITaskQueue::Ptr engineQueue_;
};

SCENE_END_NAMESPACE()

#endif