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
#include "asset_object.h"

#include <algorithm>
#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_ecs_object.h>
#include <scene/ext/intf_ecs_object_access.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/intf_render_resource.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/resource/types.h>

#include <3d/ecs/components/animation_component.h>
#include <3d/ecs/components/animation_track_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <base/containers/unordered_map.h>

#include "../ecs_component/resource_component.h"
#include "../resource/util.h"

SCENE_BEGIN_NAMESPACE()
namespace {

struct ResourcesCreator {
    IScene::Ptr scene;
    IInternalScene::Ptr iScene;
    BASE_NS::string group;

    IResourceComponentManager* resuMan {};
    CORE3D_NS::IRenderHandleComponentManager* renderMan {};
    CORE3D_NS::INameComponentManager* nameMan {};
    CORE_NS::IResourceManager::Ptr resources;

    ResourcesCreator(const IScene::Ptr& sc, BASE_NS::string_view uri, const CORE3D_NS::IGLTF2Importer::Ptr& importer,
        const CORE_NS::ResourceId& rid)
        : scene(sc)
    {
        if (auto i = scene->GetInternalScene()) {
            iScene = i;
            if (auto c = i->GetContext()) {
                resources = c->GetResources();
            }
        }
        if (!resources) {
            return;
        }
        resuMan = static_cast<IResourceComponentManager*>(iScene->GetEcsContext().FindComponent<ResourceComponent>());
        renderMan = static_cast<CORE3D_NS::IRenderHandleComponentManager*>(
            iScene->GetEcsContext().FindComponent<CORE3D_NS::RenderHandleComponent>());
        nameMan = static_cast<CORE3D_NS::INameComponentManager*>(
            iScene->GetEcsContext().FindComponent<CORE3D_NS::NameComponent>());

        if (!resuMan || !renderMan || !nameMan) {
            return;
        }
        if (rid.IsValid()) {
            group = rid.ToString();
        } else {
            // try to find group name that is not in use
            group = UniqueGroupName(resources, uri);
        }
        CORE_LOG_D("Loading gltf resources to group: %s", group.c_str());

        CreateImageResources(importer->GetResult().data.images);
        CreateMaterialResources(importer->GetResult().data.materials);
        CreateAnimationResources();

        iScene->SetResourceGroups(StringToResourceGroupBundle(iScene->GetContext(), { group }));
    }

    void AddResource(
        CORE_NS::Entity ent, const CORE_NS::ResourceType& type, BASE_NS::string_view nameprefix, size_t index)
    {
        BASE_NS::string name;
        if (auto h = nameMan->Read(ent)) {
            name = h->name;
        }
        if (name.empty()) {
            name = BASE_NS::string(nameprefix + BASE_NS::to_string(index));
        }
        CORE_NS::ResourceId rid { name, group };
        resuMan->Create(ent);
        if (auto h = resuMan->Write(ent)) {
            h->resourceId = rid;
        }
        if (!resources->GetResourceInfo(rid).id.IsValid()) {
            resources->AddResource(rid, type, "", nullptr);
        }
    }

    void CreateImageResources(const BASE_NS::vector<CORE_NS::EntityReference>& entities)
    {
        size_t index = 0;
        for (auto&& ent : entities) {
            if (auto r = renderMan->Read(ent)) {
                if (r->reference) {
                    AddResource(ent, ClassId::ImageResource.Id().ToUid(), "image_", index++);
                }
            }
        }
    }

    void CreateMaterialResources(const BASE_NS::vector<CORE_NS::EntityReference>& entities)
    {
        size_t index = 0;
        for (auto&& ent : entities) {
            AddResource(ent, ClassId::MaterialResource.Id().ToUid(), "material_", index++);
        }
    }

    void CreateAnimationResources()
    {
        size_t index = 0;
        for (auto&& anim : iScene->GetAnimations()) {
            if (auto i = interface_cast<IEcsObjectAccess>(anim)) {
                if (auto obj = i->GetEcsObject()) {
                    AddResource(obj->GetEntity(), ClassId::AnimationResource.Id().ToUid(), "animation_", index++);
                }
            }
        }
    }
};

void UpdateAnimationTrackTargets(
    CORE_NS::IEcs& ecs, const CORE_NS::Entity& animationEntity, const CORE_NS::Entity& rootNode)
{
    auto& nameManager_ = *CORE_NS::GetManager<CORE3D_NS::INameComponentManager>(ecs);
    auto animationManager = CORE_NS::GetManager<CORE3D_NS::IAnimationComponentManager>(ecs);
    auto animationTrackManager = CORE_NS::GetManager<CORE3D_NS::IAnimationTrackComponentManager>(ecs);
    auto& entityManager = ecs.GetEntityManager();

    auto* nodeSystem = CORE_NS::GetSystem<CORE3D_NS::INodeSystem>(ecs);
    CORE_ASSERT(nodeSystem);
    if (!nodeSystem) {
        return;
    }
    auto* node = nodeSystem->GetNode(rootNode);
    CORE_ASSERT(node);
    if (!node) {
        return;
    }

    if (const CORE_NS::ScopedHandle<const CORE3D_NS::AnimationComponent> animationData =
            animationManager->Read(animationEntity);
        animationData) {
        BASE_NS::vector<CORE_NS::Entity> targetEntities;
        targetEntities.reserve(animationData->tracks.size());
        std::transform(animationData->tracks.begin(), animationData->tracks.end(), std::back_inserter(targetEntities),
            [&nameManager = nameManager_, &node](const CORE_NS::Entity& trackEntity) {
                if (auto nameHandle = nameManager.Read(trackEntity); nameHandle) {
                    if (nameHandle->name.empty()) {
                        return node->GetEntity();
                    } else {
                        if (auto targetNode = node->LookupNodeByPath(nameHandle->name); targetNode) {
                            return targetNode->GetEntity();
                        }
                    }
                }
                return CORE_NS::Entity {};
            });
        if (animationData->tracks.size() == targetEntities.size()) {
            auto targetIt = targetEntities.begin();
            for (const auto& trackEntity : animationData->tracks) {
                if (auto track = animationTrackManager->Write(trackEntity); track) {
                    if (track->target) {
                        CORE_LOG_D("AnimationTrack %s already targetted",
                            BASE_NS::to_hex(static_cast<const CORE_NS::Entity&>(track->target).id).data());
                    }
                    track->target = entityManager.GetReferenceCounted(*targetIt);
                }
                ++targetIt;
            }
        }
    }
}
} // namespace

bool AssetObject::Load(
    const IScene::Ptr& sc, BASE_NS::string_view uri, bool createResources, const CORE_NS::ResourceId& rid)
{
    auto scene = sc->GetInternalScene();
    auto& ecs = scene->GetEcsContext();
    graphicsContext_ = &scene->GetGraphicsContext();

    auto& gltf = graphicsContext_->GetGltf();
    auto gltfLoadResult = gltf.LoadGLTF(uri);
    if (!gltfLoadResult.success) {
        CORE_LOG_E("Loaded '%s' with errors:\n%s", BASE_NS::string(uri).c_str(), gltfLoadResult.error.c_str());
    }
    if (!gltfLoadResult.data) {
        return false;
    }

    ecs_ = ecs.GetNativeEcs();
    importer_ = gltf.CreateGLTF2Importer(*ecs_);
    if (!importer_) {
        CORE_LOG_E("Creating glTF importer failed");
        return false;
    }
    importer_->ImportGLTF(*gltfLoadResult.data, CORE3D_NS::CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL);
    const auto& gltfImportResult = importer_->GetResult();
    if (!gltfImportResult.success) {
        CORE_LOG_E("Importing of '%s' failed: %s", BASE_NS::string(uri).c_str(), gltfImportResult.error.c_str());
        return false;
    }

    // Loading and importing of glTF was done successfully. Fill the collection with all the gltf entities.
    const auto rootEntity = ImportSceneFromGltf(*gltfLoadResult.data, {});
    if (rootEntity == CORE_NS::Entity {}) {
        return false;
    }

    if (createResources) {
        ResourcesCreator c(sc, uri, importer_, rid);
    }

    if (!ecs.CreateUnnamedRootNode()) {
        CORE_LOG_E("Failed to create root node");
        return false;
    }

    return true;
}

CORE_NS::Entity AssetObject::ImportSceneFromGltf(const CORE3D_NS::IGLTFData& gltfData, CORE_NS::EntityReference root)
{
    auto& gltf = graphicsContext_->GetGltf();

    // Import the default scene, or first scene if there is no default scene set.
    size_t sceneIndex = gltfData.GetDefaultSceneIndex();
    if (sceneIndex == CORE3D_NS::CORE_GLTF_INVALID_INDEX && gltfData.GetSceneCount() > 0) {
        // Use first scene.
        sceneIndex = 0;
    }

    const CORE3D_NS::GLTFResourceData& resourceData = importer_->GetResult().data;
    CORE_NS::Entity importedSceneEntity {};
    if (sceneIndex != CORE3D_NS::CORE_GLTF_INVALID_INDEX) {
        CORE3D_NS::GltfSceneImportFlags importFlags = CORE3D_NS::CORE_GLTF_IMPORT_COMPONENT_FLAG_BITS_ALL;
        importedSceneEntity = gltf.ImportGltfScene(sceneIndex, gltfData, resourceData, *ecs_, root, importFlags);
    }
    if (!CORE_NS::EntityUtil::IsValid(importedSceneEntity)) {
        return {};
    }

    // Link animation tracks to target nodes.
    if (!resourceData.animations.empty()) {
        CORE3D_NS::INodeSystem* nodeSystem = CORE_NS::GetSystem<CORE3D_NS::INodeSystem>(*ecs_);
        if (auto animationRootNode = nodeSystem->GetNode(importedSceneEntity); animationRootNode) {
            for (const auto& animationEntity : resourceData.animations) {
                UpdateAnimationTrackTargets(*ecs_, animationEntity, animationRootNode->GetEntity());
            }
        }
    }

    return importedSceneEntity;
}

SCENE_END_NAMESPACE()
