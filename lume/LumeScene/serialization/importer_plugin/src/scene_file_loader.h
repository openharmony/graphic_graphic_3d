/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SCENE_IMP_SRC_SCENE_FILE_LOADER_H
#define SCENE_IMP_SRC_SCENE_FILE_LOADER_H

#include <scene/interface/serialization/intf_scene_file_loader.h>
#include <scene_importer/base/namespace.h>

#include <meta/ext/object.h>

SCENE_IMP_BEGIN_NAMESPACE()

class SceneFileLoader : public META_NS::IntroduceInterfaces<META_NS::BaseObject, SCENE_NS::ISceneFileLoader> {
    META_OBJECT(SceneFileLoader, SCENE_NS::ClassId::SceneFileLoader, IntroduceInterfaces)
public:
    SCENE_NS::IScene::Ptr LoadScene(const SCENE_NS::IRenderContext::Ptr& context, BASE_NS::string_view uri,
        BASE_NS::array_view<const BASE_NS::string> resourceIndices) override;
};

SCENE_IMP_END_NAMESPACE()

#endif
