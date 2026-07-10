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

#include "resource_types.h"

#include <scene/interface/intf_scene.h>
#include <scene/interface/resource/types.h>

#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/resource/intf_object_resource.h>
#include <meta/interface/serialization/intf_refuri_builder.h>

#include "../config.h"

SCENE_IMP_BEGIN_NAMESPACE()

void RegisterResourceTypes(const IImporter::Ptr& importer, const CORE_NS::IResourceManager::Ptr& res)
{
    auto params = META_NS::GetObjectRegistry().Create<META_NS::IMetadata>(META_NS::ClassId::Object);
    if (!res || !params) {
        return;
    }
    params->AddProperty(META_NS::ConstructProperty<IImporter::Ptr>("Importer", importer));

    res->AddResourceType(META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::SceneLoader, params));
    res->AddResourceType(META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::GltfSceneLoader, params));
    res->AddResourceType(META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::ImageLoader, params));
    res->AddResourceType(META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::ShaderLoader, params));
    res->AddResourceType(
        META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::EnvironmentLoader, params));
    res->AddResourceType(META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::MeshLoader, params));
    res->AddResourceType(
        META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::MeshTemplateLoader, params));
    res->AddResourceType(META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::MaterialLoader, params));
    res->AddResourceType(
        META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::OcclusionMaterialLoader, params));
    res->AddResourceType(
        META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::PostProcessLoader, params));

    res->AddResourceType(
        META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(META_NS::ClassId::ObjectTemplateType));

    res->AddResourceType(
        META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::NodeTemplateLoader, params));

    res->AddResourceType(
        META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::AnimationTemplateLoader, params));
    res->AddResourceType(
        META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::MaterialTemplateLoader, params));
    res->AddResourceType(
        META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::OcclusionMaterialTemplateLoader, params));
    res->AddResourceType(
        META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::PostProcessTemplateLoader, params));
    res->AddResourceType(
        META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::EnvironmentTemplateLoader, params));

    res->AddResourceType(
        META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::GltfAnimationLoader, params));
    res->AddResourceType(
        META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::MetaAnimationLoader, params));
}

void RegisterResourceTypes(const IImporter::Ptr& importer)
{
    if (auto imp = interface_cast<IImporterInternal>(importer)) {
        auto conf = imp->GetConfig();
        if (conf.context) {
            RegisterResourceTypes(importer, conf.context->GetResources());
        }
    }
}

SCENE_IMP_END_NAMESPACE()
