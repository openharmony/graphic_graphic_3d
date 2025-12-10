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

SCENE_BEGIN_NAMESPACE()

void RegisterResourceTypes(const CORE_NS::IResourceManager::Ptr& res, const META_NS::IMetadata::Ptr& params)
{
    if (!res || !res->GetResourceTypes().empty()) {
        return;
    }

    res->AddResourceType(META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::SceneResource, params));
    res->AddResourceType(
        META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::GltfSceneResource, params));
    res->AddResourceType(META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::ImageResource, params));
    res->AddResourceType(META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::ShaderResource, params));
    res->AddResourceType(
        META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::EnvironmentResource, params));
    res->AddResourceType(
        META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::MaterialResource, params));
    res->AddResourceType(
        META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::OcclusionMaterialResource, params));
    res->AddResourceType(
        META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::PostProcessResource, params));
    res->AddResourceType(
        META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::AnimationResource, params));

    res->AddResourceType(
        META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(META_NS::ClassId::ObjectTemplateType));

    auto metaAnim =
        META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(META_NS::ClassId::AnimationResourceType, params);
    if (auto i = interface_cast<META_NS::IRefUriBuilderAnchorType>(metaAnim)) {
        i->AddAnchorType(ClassId::Scene.Id());
    }
    res->AddResourceType(metaAnim);

    if (auto env = META_NS::GetObjectRegistry().Create<META_NS::IObjectResource>(
            META_NS::ClassId::ObjectResourceType, params)) {
        env->SetResourceType(ClassId::EnvironmentResourceTemplate);
        res->AddResourceType(interface_pointer_cast<CORE_NS::IResourceType>(env));
    }
    if (auto env = META_NS::GetObjectRegistry().Create<META_NS::IObjectResource>(
            META_NS::ClassId::ObjectResourceType, params)) {
        env->SetResourceType(ClassId::MaterialResourceTemplate);
        res->AddResourceType(interface_pointer_cast<CORE_NS::IResourceType>(env));
    }
    if (auto env = META_NS::GetObjectRegistry().Create<META_NS::IObjectResource>(
            META_NS::ClassId::ObjectResourceType, params)) {
        env->SetResourceType(ClassId::PostProcessResourceTemplate);
        res->AddResourceType(interface_pointer_cast<CORE_NS::IResourceType>(env));
    }
}

void RegisterResourceTypes(const IRenderContext::Ptr& context, const SceneOptions& opts)
{
    if (context) {
        auto params = CreateRenderContextArg(context);
        if (auto op = META_NS::ConstructProperty<SceneOptions>("Options", opts)) {
            params->AddProperty(op);
        }
        if (auto res = context->GetResources()) {
            RegisterResourceTypes(res, params);
        }
    }
}

SCENE_END_NAMESPACE()
