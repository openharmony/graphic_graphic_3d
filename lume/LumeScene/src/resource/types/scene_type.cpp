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

#include "scene_type.h"

#include <scene/ext/util.h>
#include <scene/interface/serialization/intf_scene_exporter.h>
#include <scene/interface/serialization/intf_scene_importer.h>

SCENE_BEGIN_NAMESPACE()

bool SceneResourceType::Build(const META_NS::IMetadata::Ptr& d)
{
    bool res = Super::Build(d);
    if (res) {
        auto context = GetInterfaceBuildArg<IRenderContext>(d, "RenderContext");
        if (!context) {
            CORE_LOG_E("Invalid arguments to construct SceneResourceType");
            return false;
        }
        opts_ = GetBuildArg<SceneOptions>(d, "Options");
        context_ = context;
    }
    return res;
}
BASE_NS::string SceneResourceType::GetResourceName() const
{
    return "SceneResource";
}
BASE_NS::Uid SceneResourceType::GetResourceType() const
{
    return ClassId::SceneResource.Id().ToUid();
}
CORE_NS::IResource::Ptr SceneResourceType::LoadResource(const StorageInfo& s) const
{
    CORE_NS::IResource::Ptr res;
    if (s.payload) {
        auto md = CreateRenderContextArg(context_.lock());
        if (md) {
            md->AddProperty(META_NS::ConstructProperty<SceneOptions>("Options", opts_));
        }
        auto importer = META_NS::GetObjectRegistry().Create<ISceneImporter>(ClassId::SceneImporter, md);
        res = interface_pointer_cast<CORE_NS::IResource>(importer->ImportScene(*s.payload));
    }
    return res;
}
bool SceneResourceType::SaveResource(const CORE_NS::IResource::ConstPtr& p, const StorageInfo& s) const
{
    bool res = true;
    if (s.payload) {
        if (auto scene = interface_pointer_cast<IScene>(p)) {
            auto exporter = META_NS::GetObjectRegistry().Create<ISceneExporter>(
                ClassId::SceneExporter, CreateRenderContextArg(context_.lock()));
            res = exporter->ExportScene(*s.payload, scene);
        }
    }
    return res;
}
bool SceneResourceType::ReloadResource(const StorageInfo&, const CORE_NS::IResource::Ptr&) const
{
    // todo
    return false;
}

SCENE_END_NAMESPACE()
