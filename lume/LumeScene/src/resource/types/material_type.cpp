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

#include "material_type.h"

#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/intf_render_resource.h>

#include <meta/api/metadata_util.h>

#include "../../serialization/util.h"
#include "../util.h"

SCENE_BEGIN_NAMESPACE()

// MaterialResourceType
CORE_NS::IResource::Ptr MaterialResourceType::LoadResource(const StorageInfo& s) const
{
    auto scene = interface_pointer_cast<IScene>(s.context);
    if (!scene) {
        CORE_LOG_W("missing context, cannot load material resource");
        return nullptr;
    }
    auto is = scene->GetInternalScene();
    CORE_NS::IResource::Ptr res = is->RunDirectlyOrInTask(
        [&]() { return interface_pointer_cast<CORE_NS::IResource>(is->CreateObject(ClassId::Material, s.id)); });
    if (res && s.options) {
        ApplyObjectResourceOptions(s.id, res, *s.options, s.self, s.context);
    }
    return res;
}
bool MaterialResourceType::SaveResource(const CORE_NS::IResource::ConstPtr& p, const StorageInfo& s) const
{
    if (s.options) {
        CreateObjectResourceOptions(p, s.self, *s.options);
    }
    return true;
}
bool MaterialResourceType::ReloadResource(const StorageInfo&, const CORE_NS::IResource::Ptr&) const
{
    return false;
}

// OcclusionMaterialResourceType
CORE_NS::IResource::Ptr OcclusionMaterialResourceType::LoadResource(const StorageInfo& s) const
{
    auto scene = interface_pointer_cast<IScene>(s.context);
    if (!scene) {
        CORE_LOG_W("missing context, cannot load occlusion material resource");
        return nullptr;
    }
    auto is = scene->GetInternalScene();
    CORE_NS::IResource::Ptr res = is->RunDirectlyOrInTask([&]() {
        return interface_pointer_cast<CORE_NS::IResource>(is->CreateObject(ClassId::OcclusionMaterial, s.id));
    });
    if (res && s.options) {
        ApplyObjectResourceOptions(s.id, res, *s.options, s.self, s.context);
    }
    return res;
}
bool OcclusionMaterialResourceType::SaveResource(const CORE_NS::IResource::ConstPtr& p, const StorageInfo& s) const
{
    if (s.options) {
        CreateObjectResourceOptions(p, s.self, *s.options);
    }
    return true;
}
bool OcclusionMaterialResourceType::ReloadResource(const StorageInfo&, const CORE_NS::IResource::Ptr&) const
{
    return false;
}

SCENE_END_NAMESPACE()