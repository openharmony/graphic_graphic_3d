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
#include <scene_plugin/interface/intf_mesh.h>

#include <meta/ext/concrete_base_object.h>

#include "bind_templates.inl"
#include "intf_multi_mesh_initialization.h"
#include "node_impl.h"
#include "task_utils.h"

using SCENE_NS::MakeTask;
namespace {

class MultiMeshImpl : public META_NS::ObjectFwd<MultiMeshImpl, SCENE_NS::ClassId::MultiMeshProxy,
                          META_NS::ClassId::Object, SCENE_NS::IMultiMeshProxy, IMultimeshInitilization> {
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::IMultiMeshProxy, SCENE_NS::IMaterial::Ptr, MaterialOverride, {}, META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    // META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(
    //     SCENE_NS::IMultiMeshProxy, BASE_NS::string, MaterialOverrideUri, {}, META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::IMultiMeshProxy, SCENE_NS::IMesh::Ptr, Mesh, {}, META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IMultiMeshProxy, size_t, VisibleInstanceCount, 0u)
    META_IMPLEMENT_INTERFACE_ARRAY_PROPERTY(SCENE_NS::IMultiMeshProxy, BASE_NS::Math::Vec4, CustomData)
    META_IMPLEMENT_INTERFACE_ARRAY_PROPERTY(SCENE_NS::IMultiMeshProxy, BASE_NS::Math::Mat4X4, Transforms)

    void SetInstanceCount(size_t count) override
    {
        // post to engine
        if (auto sh = sceneHolder_.lock()) {
            sh->QueueEngineTask(MakeTask(
                                    [count](auto selfObject) {
                                        if (auto self = static_pointer_cast<MultiMeshImpl>(selfObject)) {
                                            if (auto sceneHolder = self->sceneHolder_.lock()) {
                                                CORE_NS::Entity mm;
                                                mm.id = self->handle_;
                                                sceneHolder->SetInstanceCountMultimeshArray(mm, count);
                                            }
                                        }
                                        return false;
                                    },
                                    GetSelf()),
                false);
        }

        // Make sure that array properties are in sync
        auto size = META_ACCESS_PROPERTY(CustomData)->GetSize();
        assert(size == META_ACCESS_PROPERTY(Transforms)->GetSize());
        do {
            if (count > size) {
                META_ACCESS_PROPERTY(CustomData)->AddValue({ 1.f, 1.f, 1.f, 1.f });
                META_ACCESS_PROPERTY(Transforms)->AddValue(BASE_NS::Math::IDENTITY_4X4);
            } else if (count < size) {
                META_ACCESS_PROPERTY(CustomData)->RemoveAt(size - 1);
                META_ACCESS_PROPERTY(Transforms)->RemoveAt(size - 1);
            }
            size = META_ACCESS_PROPERTY(CustomData)->GetSize();
        } while (size != count);
    }

    SceneHolder::WeakPtr sceneHolder_;
    uint64_t handle_ {};
    BASE_NS::vector<CORE_NS::Entity> overridenMaterials_;

    void Initialize(SceneHolder::Ptr sceneHolder, size_t instanceCount, CORE_NS::Entity baseComponent) override
    {
        sceneHolder_ = sceneHolder;
        if (auto sh = sceneHolder) {
            sh->QueueEngineTask(MakeTask(
                                    [baseComponent](auto sceneHolder, auto self) {
                                        auto entity = sceneHolder->CreateMultiMeshInstance(baseComponent);
                                        static_cast<MultiMeshImpl*>(self.get())->handle_ = entity.id;

                                        return false;
                                    },
                                    sceneHolder_, GetSelf()),
                false);
        }
        SetInstanceCount(instanceCount);
    }

    bool Build(const IMetadata::Ptr& data) override
    {
        // subscribe mesh changes
        Mesh()->OnChanged()->AddHandler(
            META_NS::MakeCallback<META_NS::IOnChanged>([weak = BASE_NS::weak_ptr(GetSelf())]() {
                if (auto self = static_pointer_cast<MultiMeshImpl>(weak.lock())) {
                    if (auto sh = self->sceneHolder_.lock()) {
                        sh->QueueEngineTask(MakeTask([weak]() {
                            if (auto self = static_pointer_cast<MultiMeshImpl>(weak.lock())) {
                                auto mesh = interface_pointer_cast<INodeEcsInterfacePrivate>(self->Mesh()->GetValue());
                                if (!mesh || !mesh->EcsObject()) {
                                    return false; // Not sure if user should be able to reset ecs mesh by
                                                  // setting the property as null
                                }
                                if (auto sceneHolder = self->sceneHolder_.lock()) {
                                    CORE_NS::Entity mm;
                                    mm.id = self->handle_;
                                    sceneHolder->SetMeshMultimeshArray(mm, mesh->EcsObject()->GetEntity());
                                }
                            }
                            return false;
                        }),
                            false);
                    }
                }
            }),
            reinterpret_cast<uint64_t>(this));

        // subscribe material changes
        MaterialOverride()->OnChanged()->AddHandler(
            META_NS::MakeCallback<META_NS::IOnChanged>([weak = BASE_NS::weak_ptr(GetSelf())]() {
                if (auto self = static_pointer_cast<MultiMeshImpl>(weak.lock())) {
                    if (auto sh = self->sceneHolder_.lock()) {
                        sh->QueueEngineTask(
                            MakeTask(
                                [](auto selfObject) {
                                    if (auto self = static_pointer_cast<MultiMeshImpl>(selfObject)) {
                                        if (auto sceneHolder = self->sceneHolder_.lock()) {
                                            CORE_NS::Entity mm;
                                            mm.id = self->handle_;
                                            auto material = interface_pointer_cast<INodeEcsInterfacePrivate>(
                                                self->MaterialOverride()->GetValue());
                                            if (!material || !material->EcsObject()) {
                                                if (!self->overridenMaterials_.empty()) {
                                                    sceneHolder->ResetOverrideMaterialMultimeshArray(
                                                        mm, self->overridenMaterials_);
                                                }
                                            } else {
                                                auto backup = sceneHolder->SetOverrideMaterialMultimeshArray(
                                                    mm, material->EcsObject()->GetEntity());
                                                if (self->overridenMaterials_.empty()) {
                                                    self->overridenMaterials_ =
                                                        backup; // store backup only for the first change
                                                }
                                            }
                                        }
                                    }
                                    return false;
                                },
                                weak),
                            false);
                    }
                }
            }),
            reinterpret_cast<uint64_t>(this));

        // subscribe visible count changes
        VisibleInstanceCount()->OnChanged()->AddHandler(
            META_NS::MakeCallback<META_NS::IOnChanged>([weak = BASE_NS::weak_ptr(GetSelf())]() {
                if (auto self = static_pointer_cast<MultiMeshImpl>(weak.lock())) {
                    auto count = self->VisibleInstanceCount()->GetValue();
                    if (auto sh = self->sceneHolder_.lock()) {
                        sh->QueueEngineTask(MakeTask(
                                                [count](auto selfObject) {
                                                    if (auto self = static_pointer_cast<MultiMeshImpl>(selfObject)) {
                                                        if (auto sceneHolder = self->sceneHolder_.lock()) {
                                                            CORE_NS::Entity mm;
                                                            mm.id = self->handle_;
                                                            sceneHolder->SetVisibleCountMultimeshArray(mm, count);
                                                        }
                                                    }
                                                    return false;
                                                },
                                                weak),
                            false);
                    }
                }
            }),
            reinterpret_cast<uint64_t>(this));

        //todo
        // subscribe CustomData changes, write only
        /*CustomData()->OnChanged()->AddHandler(META_NS::MakeCallback<META_NS::IMetaArrayProperty::
                                                      OnChangedType>([weak = BASE_NS::weak_ptr(GetSelf())](
                                                                         META::ArrayChangeNotification::ConstPtr ptr) {
            if (auto self = static_pointer_cast<MultiMeshImpl>(weak.lock())) {
                for (auto& ref : ptr->Changes) {
                    if (ref.Type == META::ArrayChangeNotification::MODIFIED ||
                        ref.Type == META::ArrayChangeNotification::ADDED) {
                        auto ix = ref.Begin;
                        auto data = self->CustomData()->Get(ix);
                        if (auto sh = self->sceneHolder_.lock()) {
                            sh->QueueEngineTask(
                                MakeTask(
                                    [data, ix](auto selfObject) {
                                        if (auto self = static_pointer_cast<MultiMeshImpl>(selfObject)) {
                                            if (auto sceneHolder = self->sceneHolder_.lock()) {
                                                CORE_NS::Entity mm;
                                                mm.id = self->handle_;
                                                sceneHolder->SetCustomData(mm, ix, { data.x, data.y, data.z, data.w });
                                            }
                                        }
                                        return false;
                                    },
                                    weak),
                                false);
                        }
                    }
                }
            }
        }),
            reinterpret_cast<uint64_t>(this));*/

        //todo
        // subscribe Transformation changes, write only
        /*
        Transforms()->OnChanged()->AddHandler(
            META_NS::MakeCallback<META_NS::IMetaArrayProperty::OnChangedType>(
                [weak = BASE_NS::weak_ptr(GetSelf())](META::ArrayChangeNotification::ConstPtr ptr) {
                    if (auto self = static_pointer_cast<MultiMeshImpl>(weak.lock())) {
                        for (auto& ref : ptr->Changes) {
                            if (ref.Type == META::ArrayChangeNotification::MODIFIED ||
                                ref.Type == META::ArrayChangeNotification::ADDED) {
                                auto ix = ref.Begin;
                                auto data = self->Transforms()->Get(ix);
                                if (auto sh = self->sceneHolder_.lock()) {
                                    sh->QueueEngineTask(
                                        MakeTask(
                                            [data, ix](auto selfObject) {
                                                if (auto self = static_pointer_cast<MultiMeshImpl>(selfObject)) {
                                                    if (auto sceneHolder = self->sceneHolder_.lock()) {
                                                        CORE_NS::Entity mm;
                                                        mm.id = self->handle_;
                                                        sceneHolder->SetTransformation(mm, ix, data);
                                                    }
                                                }
                                                return false;
                                            },
                                            weak),
                                        false);
                                }
                            }
                        }
                    }
                }),
            reinterpret_cast<uint64_t>(this));
        */
        return true;
    }
};
} // namespace
SCENE_BEGIN_NAMESPACE()
void RegisterMultiMeshImpl()
{
    META_NS::GetObjectRegistry().RegisterObjectType<MultiMeshImpl>();
}
void UnregisterMultiMeshImpl()
{
    META_NS::GetObjectRegistry().UnregisterObjectType<MultiMeshImpl>();
}

SCENE_END_NAMESPACE()