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

#ifndef SCENE_INTERFACE_ISCENE_MANAGER_H
#define SCENE_INTERFACE_ISCENE_MANAGER_H

#include <scene/base/types.h>
#include <scene/interface/intf_scene.h>

SCENE_BEGIN_NAMESPACE()

class ISceneManager : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISceneManager, "4ac48d37-5bd9-4237-87e1-09848e1e15c5")
public:
    virtual Future<IScene::Ptr> CreateScene() = 0;
    virtual Future<IScene::Ptr> CreateScene(BASE_NS::string_view uri) = 0;
};

META_REGISTER_CLASS(SceneManager, "e294e3d0-014f-4c93-a12b-e25c691277b4", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::ISceneManager)

#endif
