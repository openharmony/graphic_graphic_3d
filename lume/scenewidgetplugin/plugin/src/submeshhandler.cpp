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
#include <scene_plugin/api/mesh_uid.h>
#include <scene_plugin/interface/intf_material.h>

#include <3d/ecs/components/mesh_component.h>

#include <meta/ext/object.h>

#include "intf_node_private.h"
#include "intf_submesh_bridge.h"
#include "submesh_handler_uid.h"
#include "task_utils.h"

using SCENE_NS::MakeTask;

namespace {
class SubMeshHandler : public META_NS::ObjectFwd<SubMeshHandler, SCENE_NS::ClassId::SubMeshHandler,
                           META_NS::ClassId::Object, SCENE_NS::IEcsProxyObject, SCENE_NS::ISubMeshBridge> {
public:
    void Initialize(CORE3D_NS::IMeshComponentManager* componentManager, CORE_NS::Entity entity,
        META_NS::IProperty::Ptr submeshes, INodeEcsInterfacePrivate::Ptr node) override
    {
        node_ = node;
        if (node) {
            componentManager_ = componentManager;
            entity_ = entity;
            submeshes_ = META_NS::ArrayProperty<SCENE_NS::ISubMesh::Ptr>(submeshes);
            SetCommonListener(node->SceneHolder()->GetCommonEcsListener());
            if (auto scene = node->EcsScene()) {
                scene->AddEngineTask(MakeTask(
                                         [](auto obj) {
                                             if (auto self = static_cast<SubMeshHandler*>(obj.get())) {
                                                 self->DoComponentEvent(
                                                     CORE_NS::IEcs::ComponentListener::EventType::MODIFIED,
                                                     *self->componentManager_, self->entity_);
                                             }
                                             return false;
                                         },
                                         GetSelf()),
                    false);
            }
        }
    }

    void Destroy() override
    {
        if (listener_) {
            listener_->RemoveEntity(entity_, GetSelf<SCENE_NS::IEcsProxyObject>(),
                *static_cast<CORE_NS::IComponentManager*>(componentManager_));
        }
    }

    // IEcsProxyObject

    void SetCommonListener(BASE_NS::shared_ptr<SCENE_NS::EcsListener> listener) override
    {
        if (listener_) {
            listener_->RemoveEntity(entity_, GetSelf<SCENE_NS::IEcsProxyObject>(),
                *static_cast<CORE_NS::IComponentManager*>(componentManager_));
        }
        listener_ = listener;
        if (listener_) {
            listener_->AddEntity(entity_, GetSelf<SCENE_NS::IEcsProxyObject>(),
                *static_cast<CORE_NS::IComponentManager*>(componentManager_));
        }
    }

    void DoEntityEvent(CORE_NS::IEcs::EntityListener::EventType, const CORE_NS::Entity&) override {}

    SCENE_NS::IMaterial::Ptr GetMaterialFromEntity(
        SCENE_NS::IScene::Ptr scene, SceneHolder::Ptr sceneHolder, CORE_NS::Entity entity)
    {
        // First see if we have this material already.
        auto materials = scene->GetMaterials();
        for (auto material : materials) {
            auto ecsObject = interface_pointer_cast<SCENE_NS::IEcsObject>(material);
            if (ecsObject && ecsObject->GetEntity() == entity) {
                return material;
            }
        }

        // Then try to resolve from uri.
        BASE_NS::string uri;
        if (sceneHolder->GetEntityUri(entity, uri)) {
            return scene->GetMaterial(uri);
        }

        BASE_NS::string name;
        if (sceneHolder->GetEntityName(entity, name)) {
            return scene->GetMaterial(name);
        }

        return {};
    }

    // ToDo: This runs on "engine queue", someday the property operations could be decoupled
    // Another todo, this should presumably be closer to SceneHolder instead interleaved into the node impl
    void DoComponentEvent(CORE_NS::IEcs::ComponentListener::EventType type,
        const CORE_NS::IComponentManager& componentManager, const CORE_NS::Entity& entity) override
    {
        if (entity != entity_) {
            CORE_LOG_W("DoComponentEvent called for wrong entity.");
            return;
        }

        if (type == CORE_NS::IEcs::ComponentListener::EventType::MODIFIED) {
            // Read data here, we don't want to keep read lock when we update materials below
            // as it can cause changes to material components further down the pipeline.
            auto componentData = componentManager_->Get(entity);

            // This may be nasty from the application point of view, but lessens a risk
            // that index changes would be propagated incorrectly to client app
            if (submeshes_->GetSize() > componentData.submeshes.size()) {
                submeshes_->Reset();
            }

            while (submeshes_->GetSize() < componentData.submeshes.size()) {
                auto submesh = GetObjectRegistry().Create<SCENE_NS::ISubMesh>(SCENE_NS::ClassId::SubMesh);
                submeshes_->AddValue(submesh);
            }

            for (auto i = 0; i < componentData.submeshes.size(); ++i) {
                const auto& submesh = componentData.submeshes.at(i);
                auto ptr = submeshes_->GetValueAt(i);
                // TODO: These could be initialized only once.
                ptr->Name()->SetValue(BASE_NS::string("Submesh ") + BASE_NS::to_string(i));

                META_NS::SetValue(ptr->AABBMin(), submesh.aabbMin);
                META_NS::SetValue(ptr->AABBMax(), submesh.aabbMax);
                META_NS::SetValue(ptr->RenderSortLayerOrder(), submesh.renderSortLayerOrder);
                META_NS::SetValue(ptr->Handle(), i);

                auto priv = interface_pointer_cast<SCENE_NS::ISubMeshPrivate>(ptr);
                if (CORE_NS::EntityUtil::IsValid(submesh.material)) {
                    if (auto node = node_.lock()) {
                        // This would appreciate more async approach
                        auto sceneHolder = node->SceneHolder();
                        auto scene = interface_pointer_cast<SCENE_NS::IScene>(node->EcsScene());

                        if (scene && sceneHolder) {
                            auto material = GetMaterialFromEntity(scene, sceneHolder, submesh.material);
                            priv->SetDefaultMaterial(material);
                        }
                    }
                } else {
                    priv->SetDefaultMaterial({});
                }
                priv->AddSubmeshBrigde(GetSelf<ISubMeshBridge>());
            }
        }
    }

public: // ISubMeshBridge
    void SetRenderSortLayerOrder(size_t index, uint8_t value) override
    {
#ifdef SYNC_ACCESS_ECS
        if (auto handle = componentManager_->Write(entity_)) {
            if (index >= 0 && index < handle->submeshes.size()) {
                handle->submeshes[index].renderSortOrder = value;
            }
        }
#else
        if (auto node = node_.lock()) {
            if (auto scene = node->EcsScene()) {
                scene->AddEngineTask(MakeTask(
                                         [index, value](auto selfNode) {
                                             if (auto self = interface_pointer_cast<SCENE_NS::IEcsObject>(selfNode)) {
                                                 if (auto node = interface_cast<INodeEcsInterfacePrivate>(selfNode)) {
                                                     if (auto sceneHolder = node->SceneHolder()) {
                                                         sceneHolder->SetSubmeshRenderSortOrder(
                                                             self->GetEntity(), index, value);
                                                     }
                                                 }
                                             }
                                             return false;
                                         },
                                         node),
                    false);
            }
        }
#endif
        if (index < submeshes_->GetSize()) {
            META_NS::SetValue(submeshes_->GetValueAt(index)->RenderSortLayerOrder(), value);
        }
    }

    void SetMaterialToEcs(size_t index, SCENE_NS::IMaterial::Ptr& material) override
    {
        if (auto node = node_.lock()) {
            if (auto scene = node->EcsScene()) {
                scene->AddEngineTask(
                    MakeTask(
                        [mat = SCENE_NS::IMaterial::WeakPtr(material), index](auto node) {
                            if (auto self = interface_pointer_cast<SCENE_NS::IEcsObject>(node)) {
                                CORE_NS::Entity entity {};
                                if (auto ecsObject = interface_pointer_cast<SCENE_NS::IEcsObject>(mat.lock())) {
                                    entity = ecsObject->GetEntity();
                                }
                                auto sceneHolder = node->SceneHolder();
                                if (sceneHolder) {
                                    sceneHolder->SetMaterial(self->GetEntity(), entity, index);
                                }
                            }
                            return false;
                        },
                        node_),
                    false);
            }
        }
    }

    void SetAABBMin(size_t index, BASE_NS::Math::Vec3 vec) override
    {
#ifdef SYNC_ACCESS_ECS
        if (auto handle = componentManager_->Write(entity_)) {
            if (index >= 0 && index < handle->submeshes.size()) {
                handle->submeshes[index].aabbMin = vec;
                handle->aabbMin = BASE_NS::Math::min(handle->aabbMin, vec);
            }
        }
#else
        if (auto node = node_.lock()) {
            if (auto scene = node->EcsScene()) {
                scene->AddEngineTask(MakeTask(
                                         [index, vec](auto selfNode) {
                                             if (auto self = interface_pointer_cast<SCENE_NS::IEcsObject>(selfNode)) {
                                                 if (auto node = interface_cast<INodeEcsInterfacePrivate>(selfNode)) {
                                                     if (auto sceneHolder = node->SceneHolder()) {
                                                         sceneHolder->SetSubmeshAABBMin(self->GetEntity(), index, vec);
                                                     }
                                                 }
                                             }
                                             return false;
                                         },
                                         node),
                    false);
            }
        }
#endif
        if (index < submeshes_->GetSize()) {
            META_NS::SetValue(submeshes_->GetValueAt(index)->AABBMin(), vec);
        }
    }

    void SetAABBMax(size_t index, BASE_NS::Math::Vec3 vec) override
    {
#ifdef SYNC_ACCESS_ECS
        if (auto handle = componentManager_->Write(entity_)) {
            if (index >= 0 && index < handle->submeshes.size()) {
                handle->submeshes[index].aabbMax = vec;
                handle->aabbMax = BASE_NS::Math::max(handle->aabbMax, vec);
            }
        }
#else
        if (auto node = node_.lock()) {
            if (auto scene = node->EcsScene()) {
                scene->AddEngineTask(MakeTask(
                                         [index, vec](auto selfNode) {
                                             if (auto self = interface_pointer_cast<SCENE_NS::IEcsObject>(selfNode)) {
                                                 if (auto node = interface_cast<INodeEcsInterfacePrivate>(selfNode)) {
                                                     if (auto sceneHolder = node->SceneHolder()) {
                                                         sceneHolder->SetSubmeshAABBMax(self->GetEntity(), index, vec);
                                                     }
                                                 }
                                             }
                                             return false;
                                         },
                                         node),
                    false);
            }
        }
#endif
        if (index < submeshes_->GetSize()) {
            META_NS::SetValue(submeshes_->GetValueAt(index)->AABBMax(), vec);
        }
    }

    void RemoveSubmesh(int32_t index) override
    {
#ifdef SYNC_ACCESS_ECS
        if (auto handle = componentManager_->Write(entity_)) {
            if (index < 0) {
                handle->submeshes.clear();
                handle->aabbMin = { 0.f, 0.f, 0.f };
                handle->aabbMax = { 0.f, 0.f, 0.f };
            } else if (index < handle->submeshes.size()) {
                handle->submeshes.erase(handle->submeshes.begin() + index);
                for (const auto& submesh : handle->submeshes) {
                    handle->aabbMin = BASE_NS::Math::min(handle->aabbMin, submesh.aabbMin);
                    handle->aabbMax = BASE_NS::Math::max(handle->aabbMax, submesh.aabbMax);
                }
            }
        }
#else
        if (auto node = node_.lock()) {
            if (auto scene = node->EcsScene()) {
                scene->AddEngineTask(MakeTask(
                                         [index](auto selfNode) {
                                             if (auto self = interface_pointer_cast<SCENE_NS::IEcsObject>(selfNode)) {
                                                 if (auto node = interface_cast<INodeEcsInterfacePrivate>(selfNode)) {
                                                     if (auto sceneHolder = node->SceneHolder()) {
                                                         sceneHolder->RemoveSubmesh(self->GetEntity(), index);
                                                     }
                                                 }
                                             }
                                             return false;
                                         },
                                         node_),
                    false);
            }
        }
#endif
    }

    CORE_NS::Entity GetEntity() const override
    {
        return entity_;
    }

    BASE_NS::shared_ptr<SCENE_NS::EcsListener> listener_ {};
    CORE3D_NS::IMeshComponentManager* componentManager_ {};
    CORE_NS::Entity entity_ {};
    META_NS::ArrayProperty<SCENE_NS::ISubMesh::Ptr> submeshes_ {};
    INodeEcsInterfacePrivate::WeakPtr node_ {};
};
} // namespace
SCENE_BEGIN_NAMESPACE()

void RegisterSubMeshHandler()
{
    META_NS::GetObjectRegistry().RegisterObjectType<SubMeshHandler>();
}
void UnregisterSubMeshHandler()
{
    META_NS::GetObjectRegistry().UnregisterObjectType<SubMeshHandler>();
}
SCENE_END_NAMESPACE()