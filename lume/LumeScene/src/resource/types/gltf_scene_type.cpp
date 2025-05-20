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
#include <scene/interface/serialization/intf_scene_exporter.h>
#include <scene/interface/serialization/intf_scene_importer.h>

SCENE_BEGIN_NAMESPACE()

bool GltfSceneResourceType::Build(const META_NS::IMetadata::Ptr& d)
{
    bool res = Super::Build(d);
    if (res) {
        auto context = GetInterfaceBuildArg<IRenderContext>(d, "RenderContext");
        if (!context) {
            CORE_LOG_E("Invalid arguments to construct GltfSceneResourceType");
            return false;
        }
        context_ = context;
    }
    return res;
}
BASE_NS::string GltfSceneResourceType::GetResourceName() const
{
    return "GltfSceneResource";
}
BASE_NS::Uid GltfSceneResourceType::GetResourceType() const
{
    return ClassId::GltfSceneResource.Id().ToUid();
}
CORE_NS::IResource::Ptr GltfSceneResourceType::LoadResource(const StorageInfo& s) const
{
    CORE_NS::IResource::Ptr res;
    if (s.payload) {
        if (auto m = META_NS::GetObjectRegistry().Create<ISceneManager>(
                ClassId::SceneManager, CreateRenderContextArg(context_.lock()))) {
            if (auto scene = m->CreateScene(s.path).GetResult()) {
                res = interface_pointer_cast<CORE_NS::IResource>(scene);
            }
        }
    }
    return res;
}
bool GltfSceneResourceType::SaveResource(const CORE_NS::IResource::ConstPtr& p, const StorageInfo& s) const
{
    return true;
}
bool GltfSceneResourceType::ReloadResource(const StorageInfo&, const CORE_NS::IResource::Ptr&) const
{
    // todo
    return false;
}

SCENE_END_NAMESPACE()
