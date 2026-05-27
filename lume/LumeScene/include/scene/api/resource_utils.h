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

#ifndef SCENE_API_RESOURCE_UTILS_H
#define SCENE_API_RESOURCE_UTILS_H

#include <scene/api/resource.h>
#include <scene/api/template/animation_template.h>
#include <scene/api/template/environment_template.h>
#include <scene/api/template/material_template.h>
#include <scene/api/template/postprocess_template.h>
#include <scene/ext/scene_utils.h>
#include <scene/interface/resource/types.h>
#include <scene/interface/template/intf_resource_template.h>

#include <meta/api/animation.h>
#include <meta/base/interface_utils.h>
#include <meta/interface/intf_object_registry.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief A helper function for retrieving all resources of given type that were imported as part of a node in a scene.
 * @param node The node whose resources to fetch.
 * @param resourceId Type of resource. If {}, return all resources, regardless of type.
 * @return A list of resources that match the input criteria.
 */
inline BASE_NS::vector<CORE_NS::IResource::Ptr> GetImportedResources(
    const INode::ConstPtr& node, const BASE_NS::Uid resourceId)
{
    BASE_NS::vector<CORE_NS::IResource::Ptr> ret;
    if (node) {
        if (auto scene = node->GetScene()) {
            auto resources = GetResourceManager(scene);
            auto ext = GetExternalNodeAttachment(node);
            if (resources && ext) {
                BASE_NS::vector<CORE_NS::MatchingResourceId> ids;
                for (auto&& id : ext->GetAssociatedResources()) {
                    auto i = resources->GetResourceInfo(id);
                    if (i.type == resourceId || resourceId == BASE_NS::Uid{}) {
                        if (auto res = resources->GetResource(id)) {
                            ret.push_back(BASE_NS::move(res));
                        }
                    }
                }
            }
        }
    }
    return ret;
}

/**
 * @brief A helper function for retrieving a list of animations that were imported as part of a node.
 * @param node The node whose animations to fetch.
 * @return A list of animations.
 */
inline BASE_NS::vector<META_NS::Animation> GetImportedAnimations(const INode::ConstPtr& node)
{
    return META_NS::Internal::ArrayCast<META_NS::Animation>(
        GetImportedResources(node, ClassId::AnimationResource.Id().ToUid()));
}

/**
 * @brief A helper function for retrieving a list of materials that were imported as part of a node.
 * @param node The node whose materials to fetch.
 * @return A list of materials.
 */
inline BASE_NS::vector<Material> GetImportedMaterials(const INode::ConstPtr& node)
{
    BASE_NS::vector<Material> materials;
    auto mat = GetImportedResources(node, ClassId::MaterialResource.Id().ToUid());
    auto occ = GetImportedResources(node, ClassId::OcclusionMaterialResource.Id().ToUid());
    materials.reserve(mat.size() + occ.size());
    for (auto&& m : mat) {
        materials.emplace_back(m);
    }
    for (auto&& m : occ) {
        materials.emplace_back(m);
    }
    return materials;
}

/**
 * @brief Retrieve all scene-context-bound resources of the given type from the
 *        scene's resource manager.
 * @param scene The scene whose resource manager to query.
 * @param resourceId Type of resource. If {}, return all resources, regardless of type.
 * @return Resources of the requested type that are registered with the scene.
 */
inline BASE_NS::vector<CORE_NS::IResource::Ptr> GetResourcesOfType(
    const IScene::Ptr& scene, const BASE_NS::Uid resourceId)
{
    BASE_NS::vector<CORE_NS::IResource::Ptr> ret;
    auto resources = GetResourceManager(scene);
    if (!resources || !scene) {
        return ret;
    }
    auto context = interface_pointer_cast<CORE_NS::IInterface>(scene);
    for (auto&& info : resources->GetResourceInfos(context)) {
        if (info.type == resourceId || resourceId == BASE_NS::Uid{}) {
            if (auto res = resources->GetResource({info.id, context})) {
                ret.push_back(BASE_NS::move(res));
            }
        }
    }
    return ret;
}

/**
 * @brief Retrieve all context-free resources of the given type from a resource manager.
 *        Use this overload to find entries (e.g. resource templates) that were
 *        registered with no resource context.
 * @param resources The resource manager to query.
 * @param resourceId Type of resource. If {}, return all resources, regardless of type.
 * @return Resources of the requested type that are not bound to any context.
 */
inline BASE_NS::vector<CORE_NS::IResource::Ptr> GetResourcesOfType(
    const CORE_NS::IResourceManager::Ptr& resources, const BASE_NS::Uid resourceId)
{
    BASE_NS::vector<CORE_NS::IResource::Ptr> ret;
    if (!resources) {
        return ret;
    }
    for (auto&& info : resources->GetResourceInfos(CORE_NS::ResourceContextPtr{})) {
        if (info.type == resourceId || resourceId == BASE_NS::Uid{}) {
            if (auto res = resources->GetResource({info.id, CORE_NS::ResourceContextWeakPtr{}})) {
                ret.push_back(BASE_NS::move(res));
            }
        }
    }
    return ret;
}

namespace Internal {
inline BASE_NS::vector<Material> CombineMaterials(
    BASE_NS::vector<CORE_NS::IResource::Ptr> mat, BASE_NS::vector<CORE_NS::IResource::Ptr> occ)
{
    BASE_NS::vector<Material> materials;
    materials.reserve(mat.size() + occ.size());
    for (auto&& m : mat) {
        materials.emplace_back(BASE_NS::move(m));
    }
    for (auto&& m : occ) {
        materials.emplace_back(BASE_NS::move(m));
    }
    return materials;
}
}  // namespace Internal

/**
 * @brief Retrieve all materials (Material + OcclusionMaterial) registered with
 *        the scene as their resource context.
 */
inline BASE_NS::vector<Material> GetSceneMaterials(const IScene::Ptr& scene)
{
    return Internal::CombineMaterials(GetResourcesOfType(scene, ClassId::MaterialResource.Id().ToUid()),
        GetResourcesOfType(scene, ClassId::OcclusionMaterialResource.Id().ToUid()));
}

/**
 * @brief Retrieve all context-free materials (Material + OcclusionMaterial) from
 *        the resource manager.
 */
inline BASE_NS::vector<Material> GetSceneMaterials(const CORE_NS::IResourceManager::Ptr& resources)
{
    return Internal::CombineMaterials(GetResourcesOfType(resources, ClassId::MaterialResource.Id().ToUid()),
        GetResourcesOfType(resources, ClassId::OcclusionMaterialResource.Id().ToUid()));
}

/**
 * @brief Retrieve all environments registered with the scene as their resource context.
 */
inline BASE_NS::vector<Environment> GetSceneEnvironments(const IScene::Ptr& scene)
{
    return META_NS::Internal::ArrayCast<Environment>(
        GetResourcesOfType(scene, ClassId::EnvironmentResource.Id().ToUid()));
}

/**
 * @brief Retrieve all context-free environments from the resource manager.
 */
inline BASE_NS::vector<Environment> GetSceneEnvironments(const CORE_NS::IResourceManager::Ptr& resources)
{
    return META_NS::Internal::ArrayCast<Environment>(
        GetResourcesOfType(resources, ClassId::EnvironmentResource.Id().ToUid()));
}

/**
 * @brief Retrieve all shaders registered with the scene as their resource context.
 */
inline BASE_NS::vector<Shader> GetSceneShaders(const IScene::Ptr& scene)
{
    return META_NS::Internal::ArrayCast<Shader>(GetResourcesOfType(scene, ClassId::ShaderResource.Id().ToUid()));
}

/**
 * @brief Retrieve all context-free shaders from the resource manager.
 */
inline BASE_NS::vector<Shader> GetSceneShaders(const CORE_NS::IResourceManager::Ptr& resources)
{
    return META_NS::Internal::ArrayCast<Shader>(GetResourcesOfType(resources, ClassId::ShaderResource.Id().ToUid()));
}

/**
 * @brief Retrieve all animations registered with the scene as their resource context.
 */
inline BASE_NS::vector<META_NS::Animation> GetSceneAnimations(const IScene::Ptr& scene)
{
    return META_NS::Internal::ArrayCast<META_NS::Animation>(
        GetResourcesOfType(scene, ClassId::AnimationResource.Id().ToUid()));
}

/**
 * @brief Retrieve all context-free animations from the resource manager.
 */
inline BASE_NS::vector<META_NS::Animation> GetSceneAnimations(const CORE_NS::IResourceManager::Ptr& resources)
{
    return META_NS::Internal::ArrayCast<META_NS::Animation>(
        GetResourcesOfType(resources, ClassId::AnimationResource.Id().ToUid()));
}

/**
 * @brief Retrieve all material templates registered with the scene as their resource context.
 */
inline BASE_NS::vector<MaterialTemplate> GetSceneMaterialTemplates(const IScene::Ptr& scene)
{
    return META_NS::Internal::ArrayCast<MaterialTemplate>(
        GetResourcesOfType(scene, ClassId::MaterialResourceTemplate.Id().ToUid()));
}

/**
 * @brief Retrieve all context-free material templates from the resource manager.
 */
inline BASE_NS::vector<MaterialTemplate> GetSceneMaterialTemplates(const CORE_NS::IResourceManager::Ptr& resources)
{
    return META_NS::Internal::ArrayCast<MaterialTemplate>(
        GetResourcesOfType(resources, ClassId::MaterialResourceTemplate.Id().ToUid()));
}

/**
 * @brief Retrieve all environment templates registered with the scene as their resource context.
 */
inline BASE_NS::vector<EnvironmentTemplate> GetSceneEnvironmentTemplates(const IScene::Ptr& scene)
{
    return META_NS::Internal::ArrayCast<EnvironmentTemplate>(
        GetResourcesOfType(scene, ClassId::EnvironmentResourceTemplate.Id().ToUid()));
}

/**
 * @brief Retrieve all context-free environment templates from the resource manager.
 */
inline BASE_NS::vector<EnvironmentTemplate> GetSceneEnvironmentTemplates(
    const CORE_NS::IResourceManager::Ptr& resources)
{
    return META_NS::Internal::ArrayCast<EnvironmentTemplate>(
        GetResourcesOfType(resources, ClassId::EnvironmentResourceTemplate.Id().ToUid()));
}

/**
 * @brief Retrieve all postprocess templates registered with the scene as their resource context.
 */
inline BASE_NS::vector<PostProcessTemplate> GetScenePostProcessTemplates(const IScene::Ptr& scene)
{
    return META_NS::Internal::ArrayCast<PostProcessTemplate>(
        GetResourcesOfType(scene, ClassId::PostProcessResourceTemplate.Id().ToUid()));
}

/**
 * @brief Retrieve all context-free postprocess templates from the resource manager.
 */
inline BASE_NS::vector<PostProcessTemplate> GetScenePostProcessTemplates(
    const CORE_NS::IResourceManager::Ptr& resources)
{
    return META_NS::Internal::ArrayCast<PostProcessTemplate>(
        GetResourcesOfType(resources, ClassId::PostProcessResourceTemplate.Id().ToUid()));
}

/**
 * @brief Retrieve all animation templates registered with the scene as their resource context.
 */
inline BASE_NS::vector<AnimationTemplate> GetSceneAnimationTemplates(const IScene::Ptr& scene)
{
    return META_NS::Internal::ArrayCast<AnimationTemplate>(
        GetResourcesOfType(scene, ClassId::AnimationResourceTemplate.Id().ToUid()));
}

/**
 * @brief Retrieve all context-free animation templates from the resource manager.
 */
inline BASE_NS::vector<AnimationTemplate> GetSceneAnimationTemplates(const CORE_NS::IResourceManager::Ptr& resources)
{
    return META_NS::Internal::ArrayCast<AnimationTemplate>(
        GetResourcesOfType(resources, ClassId::AnimationResourceTemplate.Id().ToUid()));
}

/**
 * @brief Map a resource type UID to the class id of its corresponding template.
 * @return A valid ObjectId if a template class exists for this resource type,
 *         otherwise an empty ObjectId.
 */
inline META_NS::ObjectId TemplateClassIdForResourceType(const CORE_NS::ResourceType& type)
{
    if (type == ClassId::MaterialResource.Id().ToUid()) {
        return ClassId::MaterialTemplate;
    }
    if (type == ClassId::OcclusionMaterialResource.Id().ToUid()) {
        return ClassId::MaterialTemplate;
    }
    if (type == ClassId::EnvironmentResource.Id().ToUid()) {
        return ClassId::EnvironmentTemplate;
    }
    if (type == ClassId::PostProcessResource.Id().ToUid()) {
        return ClassId::PostProcessTemplate;
    }
    if (type == ClassId::AnimationResource.Id().ToUid()) {
        return ClassId::AnimationTemplate;
    }
    return {};
}

/**
 * @brief Create a typed template populated from the current state of a resource.
 *
 * Constructs the template whose class matches resource->GetResourceType() and
 * fills it by calling IResourceTemplate::CopyFrom on the resource. The returned
 * template is a snapshot of the resource at the time of the call; it does not
 * stay in sync with later mutations.
 *
 * @return A populated template, or nullptr if the resource type has no mapped
 *         template class or CopyFrom fails.
 */
inline IResourceTemplate::Ptr CreateTemplateForResource(const CORE_NS::IResource::Ptr& resource)
{
    if (!resource) {
        return nullptr;
    }
    auto classId = TemplateClassIdForResourceType(resource->GetResourceType());
    if (!classId.IsValid()) {
        return nullptr;
    }
    auto templ = META_NS::GetObjectRegistry().Create<IResourceTemplate>(classId);
    if (!templ) {
        return nullptr;
    }
    if (auto obj = interface_cast<META_NS::IObject>(resource.get())) {
        if (!templ->CopyFrom(*obj)) {
            return nullptr;
        }
    }
    return templ;
}

SCENE_END_NAMESPACE()

#endif  // SCENE_API_RESOURCE_H
