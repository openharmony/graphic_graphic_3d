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

#include "gltf_scene_type.h"

#include <scene/ext/util.h>
#include <scene/interface/intf_scene_manager.h>

#include "../config.h"

SCENE_IMP_BEGIN_NAMESPACE()

CORE_NS::IResource::Ptr GltfSceneResourceType::LoadResource(const StorageInfo& s) const
{
    if (!s.payload) {
        return nullptr;
    }
    auto imp = interface_pointer_cast<IImporterInternal>(importer_.lock());
    if (!imp) {
        CORE_LOG_E("Importer not available when loading gltf scene resource: %s", s.id.ToString().c_str());
        return nullptr;
    }
    auto config = imp->GetConfig();
    auto m = META_NS::GetObjectRegistry().Create<SCENE_NS::ISceneManager>(
        SCENE_NS::ClassId::SceneManager, CreateRenderContextArg(config.context));
    if (!m) {
        CORE_LOG_E("Failed to create SceneManager for gltf resource: %s", s.id.ToString().c_str());
        return nullptr;
    }
    SCENE_NS::SceneOptions opts = config.staticConfig.options.scene;
    opts.resourceId = s.id;
    CORE_NS::IResource::Ptr res;
    if (auto scene = m->CreateScene(s.path, opts).GetResult()) {
        res = interface_pointer_cast<CORE_NS::IResource>(scene);
    }
    return res;
}

SCENE_IMP_END_NAMESPACE()
