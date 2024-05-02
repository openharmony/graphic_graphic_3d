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
#include <scene_plugin/api/material_uid.h>
#include <scene_plugin/api/mesh_uid.h>
#include <scene_plugin/interface/intf_material.h>

#include <meta/ext/concrete_base_object.h>
#include <meta/ext/implementation_macros.h>

#include "bind_templates.inl"
#include "intf_submesh_bridge.h"
#include "node_impl.h"
#include "submesh_handler_uid.h"
#include "task_utils.h"

using SCENE_NS::MakeTask;
#include "intf_submesh_bridge.h"

namespace {
class MeshImpl
    : public META_NS::ConcreteBaseMetaObjectFwd<MeshImpl, NodeImpl, SCENE_NS::ClassId::Mesh, SCENE_NS::IMesh> {
    static constexpr BASE_NS::string_view MESH_COMPONENT_NAME = "MeshComponent";
    static constexpr size_t MESH_COMPONENT_NAME_LEN = MESH_COMPONENT_NAME.size() + 1;
    static constexpr BASE_NS::string_view MESH_AABBMIN = "MeshComponent.aabbMin";
    static constexpr BASE_NS::string_view MESH_AABBMAX = "MeshComponent.aabbMax";

    bool Build(const IMetadata::Ptr& data) override
    {
        bool ret = false;
        if (ret = NodeImpl::Build(data); ret) {
            PropertyNameMask()[MESH_COMPONENT_NAME] = { MESH_AABBMIN.substr(MESH_COMPONENT_NAME_LEN),
                MESH_AABBMAX.substr(MESH_COMPONENT_NAME_LEN) };

            // subscribe material changes
            MaterialOverride()->OnChanged()->AddHandler(
                META_NS::MakeCallback<META_NS::IOnChanged>([this]() { SyncMaterialOverrideToSubmeshes(); }));
        }
        return ret;
    }
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::IMesh, SCENE_NS::IMaterial::Ptr, MaterialOverride, {}, )
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(SCENE_NS::IMesh, BASE_NS::string, MaterialOverrideUri, {},
        META_NS::DEFAULT_PROPERTY_FLAGS | META_NS::ObjectFlagBits::INTERNAL)

    META_IMPLEMENT_INTERFACE_READONLY_ARRAY_PROPERTY(
        SCENE_NS::IMesh, SCENE_NS::ISubMesh::Ptr, SubMeshes, {}, )
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(SCENE_NS::IMesh, BASE_NS::Math::Vec3, AABBMin,
        BASE_NS::Math::Vec3(0.f, 0.f, 0.f), META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(SCENE_NS::IMesh, BASE_NS::Math::Vec3, AABBMax,
        BASE_NS::Math::Vec3(0.f, 0.f, 0.f), META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)

    void SyncMaterialOverrideToSubmeshes()
    {
        auto material = MaterialOverride()->GetValue();
        if (material) {

            for (auto& submesh : SubMeshes()->GetValue()) {
                auto submeshPrivate = interface_cast<SCENE_NS::ISubMeshPrivate>(submesh);
                if (submeshPrivate) {
                    submeshPrivate->SetOverrideMaterial(material);
                }
            }
        } else {
            META_ACCESS_PROPERTY(MaterialOverrideUri)->SetValue({});
            // Reset submeshes back to original value.
            for (auto& submesh : SubMeshes()->GetValue()) {
                auto submeshPrivate = interface_cast<SCENE_NS::ISubMeshPrivate>(submesh);
                if (submeshPrivate) {
                    submeshPrivate->SetOverrideMaterial({});
                }
            }
        }
    }

    bool CompleteInitialization(const BASE_NS::string& path) override
    {
        if (!NodeImpl::CompleteInitialization(path)) {
            return false;
        }

        // Ensure override material is connected to ecs.
        auto overrideMaterial = GetValue(MaterialOverride());
        if (overrideMaterial) {
            BindObject(interface_pointer_cast<INode>(overrideMaterial));
        }

        // Ensure submesh materials are connected to ecs.
        auto submeshes = META_ACCESS_PROPERTY(SubMeshes)->GetValue();
        for (auto i = 0; i < submeshes.size(); ++i) {
            auto submesh = submeshes.at(i);
            auto material = META_NS::GetValue(submesh->Material());
            if (material) {
                BindObject(interface_pointer_cast<INode>(material));
            }
        }

        auto meta = interface_pointer_cast<META_NS::IMetadata>(ecsObject_);
        BindChanges(propHandler_, META_ACCESS_PROPERTY(AABBMin), meta, MESH_AABBMIN);
        BindChanges(propHandler_, META_ACCESS_PROPERTY(AABBMax), meta, MESH_AABBMAX);

        if (auto ecs = EcsScene()->GetEcs()) {
            submeshHandler_ = GetObjectRegistry().Create<SCENE_NS::ISubMeshBridge>(SCENE_NS::ClassId::SubMeshHandler);
            submeshHandler_->Initialize(CORE_NS::GetManager<CORE3D_NS::IMeshComponentManager>(*ecs),
                EcsObject()->GetEntity(), META_ACCESS_PROPERTY(SubMeshes), GetSelf<INodeEcsInterfacePrivate>());
        }

        // Ok, the initialization has reached the point where we have a scene and ecs initialized
        // check if we have material URIs that should be progressed before we sync the status with ecs
        auto materialOverrideUri = MaterialOverrideUri()->GetValue();
        if (!materialOverrideUri.empty()) {
            SCENE_NS::IMaterial::Ptr material = GetScene()->GetMaterial(materialOverrideUri);
            if (material) {
                MaterialOverride()->SetValue(material);
            }
        }

        // Update material override.
        SyncMaterialOverrideToSubmeshes();

        // Also restore submesh materials.
        for (auto i = 0; i < submeshes.size(); ++i) {
            auto submesh = submeshes.at(i);
            auto materialUri = META_NS::GetValue(submesh->MaterialUri());
            if (!materialUri.empty()) {
                submesh->Material()->SetValue(GetScene()->GetMaterial(materialUri));
            }
        }
        return true;
    }

    void SetPath(const BASE_NS::string& path, const BASE_NS::string& name, CORE_NS::Entity entity) override
    {
        META_ACCESS_PROPERTY(Path)->SetValue(path);
        META_ACCESS_PROPERTY(Name)->SetValue(name);

        if (auto iscene = GetScene()) {
            iscene->UpdateCachedReference(GetSelf<SCENE_NS::INode>());
        }
        if (auto scene = EcsScene()) {
            SetStatus(SCENE_NS::INode::NODE_STATUS_CONNECTING);
            initializeTaskToken_ = scene->AddEngineTask(
                MakeTask(
                    [name, fullpath = path + name](auto selfObject) {
                        if (auto self = static_pointer_cast<NodeImpl>(selfObject)) {
                            if (auto sceneHolder = self->SceneHolder()) {
                                CORE_NS::Entity meshEntinty {};
                                if (sceneHolder->FindMesh(name, fullpath, meshEntinty)) {
                                    SCENE_PLUGIN_VERBOSE_LOG("binding mesh: %s", name.c_str());
                                    if (auto proxyIf =
                                            interface_pointer_cast<SCENE_NS::IEcsProxyObject>(self->EcsObject())) {
                                        proxyIf->SetCommonListener(sceneHolder->GetCommonEcsListener());
                                    }
                                    self->EcsObject()->DefineTargetProperties(self->PropertyNameMask());
                                    self->EcsObject()->SetEntity(sceneHolder->GetEcs(), meshEntinty);
                                    sceneHolder->QueueApplicationTask(
                                        MakeTask(
                                            [name](auto selfObject) {
                                                if (auto self = static_pointer_cast<NodeImpl>(selfObject)) {
                                                    self->CompleteInitialization(name);
                                                    self->SetStatus(SCENE_NS::INode::NODE_STATUS_CONNECTED);
                                                    if (auto node =
                                                            interface_pointer_cast<SCENE_NS::INode>(selfObject)) {
                                                        META_NS::Invoke<META_NS::IOnChanged>(node->OnLoaded());
                                                    }
                                                }
                                                return false;
                                            },
                                            selfObject),
                                        false);
                                } else {
                                    CORE_LOG_D("Could not find '%s' mesh", name.c_str());
                                    sceneHolder->QueueApplicationTask(
                                        MakeTask(
                                            [](auto selfObject) {
                                                if (auto self = static_pointer_cast<NodeImpl>(selfObject)) {
                                                    self->SetStatus(SCENE_NS::INode::NODE_STATUS_DISCONNECTED);
                                                }
                                                return false;
                                            },
                                            selfObject),
                                        false);
                                }
                            }
                        }
                        return false;
                    },
                    GetSelf()),
                false);
        }
    }

    bool BuildChildren(SCENE_NS::INode::BuildBehavior) override
    {
        // in typical cases we should not have children
        if (META_NS::GetValue(META_ACCESS_PROPERTY(Status)) == SCENE_NS::INode::NODE_STATUS_CONNECTED) {
            SetStatus(SCENE_NS::INode::NODE_STATUS_FULLY_CONNECTED);
            META_NS::Invoke<META_NS::IOnChanged>(OnBound());
            bound_ = true;
        }
        return true;
    }

    bool ShouldExport() const override
    {
        for (auto& submesh : SubMeshes()->GetValue()) {
            auto meta = interface_pointer_cast<META_NS::IMetadata>(submesh);
            if (HasChangedProperties(*meta)) {
                return true;
            }
        }
        return false;
    }
    //todo
    /*
    bool Export(
        META_NS::Serialization::IExportContext& context, META_NS::Serialization::ClassPrimitive& value) const override
    {
        BASE_NS::vector<SCENE_NS::IMaterial::Ptr> nonSerializedMaterials;

        auto hasOverrideMaterialUri = !GetValue(MaterialOverrideUri()).empty();
        auto overrideMaterial = GetValue(MaterialOverride());
        if (overrideMaterial && hasOverrideMaterialUri) {
            if (interface_pointer_cast<META_NS::IObjectFlags>(overrideMaterial)->GetObjectFlags() &
                META_NS::ObjectFlagBits::SERIALIZE) {
                nonSerializedMaterials.push_back(overrideMaterial);
            }
        }

        for (auto& submesh : SubMeshes()->ToVector()) {
            auto hasSubmeshMaterialUri = !GetValue(submesh->MaterialUri()).empty();
            auto submeshMaterial = GetValue(submesh->Material());
            if (submeshMaterial && hasSubmeshMaterialUri) {
                if (interface_pointer_cast<META_NS::IObjectFlags>(submeshMaterial)->GetObjectFlags() &
                    META_NS::ObjectFlagBits::SERIALIZE) {
                    nonSerializedMaterials.push_back(submeshMaterial);
                }
            }
        }

        // Do not export these materials.
        for (auto& material : nonSerializedMaterials) {
            META_NS::SetObjectFlags(material, META_NS::ObjectFlagBits::SERIALIZE, false);
        }

        bool result = Fwd::Export(context, value);

        // Put back material serialization info.
        for (auto& material : nonSerializedMaterials) {
            META_NS::SetObjectFlags(material, META_NS::ObjectFlagBits::SERIALIZE, true);
        }

        return result;
    }
    */
    // Set given material for all the submeshes.
    void SetMaterial(const SCENE_NS::IMaterial::Ptr material) override
    {
        MaterialOverride()->SetValue(material);
    }

    // Set material for the spesific submesh.
    void SetMaterial(size_t index, const SCENE_NS::IMaterial::Ptr& material) override
    {
        if (auto node = interface_pointer_cast<SCENE_NS::INode>(material)) {
            node->Status()->OnChanged()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>(
                [mat = BASE_NS::weak_ptr(material)](const auto& self, const auto& status, const auto& index) {
                    if (self && status && status->GetValue() == SCENE_NS::INode::NODE_STATUS_FULLY_CONNECTED) {
                        static_cast<MeshImpl*>(self.get())->SetMaterialToScene((int32_t)index, mat.lock());
                    }
                },
                GetSelf(), node->Status(), index));
            if (auto status = META_NS::GetValue(node->Status());
                status == SCENE_NS::INode::NODE_STATUS_FULLY_CONNECTED ||
                status == SCENE_NS::INode::NODE_STATUS_CONNECTED) {
                SetMaterialToScene(static_cast<int64_t>(index), material);
            }
        }
    }

    void SetMaterialToScene(int64_t index, const SCENE_NS::IMaterial::Ptr& material)
    {
        auto count = SubMeshes()->GetSize();
        if (index == -1) {
            for (auto i = 0; i < count; ++i) {
                SubMeshes()->GetValueAt(i)->SetMaterial(material);
            }
        } else if (index < count) {
            SubMeshes()->GetValueAt(index)->SetMaterial(material);
        }
    }

    SCENE_NS::IMaterial::Ptr GetMaterial(size_t index) override
    {
        if (index < META_ACCESS_PROPERTY(SubMeshes)->GetSize()) {
            return META_ACCESS_PROPERTY(SubMeshes)->GetValueAt(index)->Material()->GetValue();
        }

        SCENE_NS::IMaterial::Ptr ret {};
        if (auto iscene = GetScene()) {
            if (META_NS::GetValue(iscene->Asynchronous()) == false) {
                auto entityName = SceneHolder()->GetMaterialName(EcsObject()->GetEntity(), index);
                if (!entityName.empty()) {
                    ret = iscene->GetMaterial(entityName);
                }
            }
        }

        // This path is not needed if material exists, in async mode it could still save the day, though
        if (!ret) {
            ret = GetObjectRegistry().Create<SCENE_NS::IMaterial>(SCENE_NS::ClassId::Material);

            if (auto scene = EcsScene()) {
                scene->AddEngineTask(
                    MakeTask(
                        [ret, index](auto selfObject) {
                            if (auto self = interface_pointer_cast<SCENE_NS::IEcsObject>(selfObject)) {
                                if (auto node = static_pointer_cast<NodeImpl>(selfObject)) {
                                    if (auto sceneHolder = node->SceneHolder()) {
                                        auto entityName = sceneHolder->GetMaterialName(self->GetEntity(), index);
                                        if (!entityName.empty()) {
                                            BASE_NS::string name(entityName.data(), entityName.size());
                                            sceneHolder->QueueApplicationTask(
                                                MakeTask(
                                                    [name, ret](auto selfObject) {
                                                        if (auto node = static_pointer_cast<NodeImpl>(selfObject)) {
                                                            auto scene = node->EcsScene();
                                                            auto object = META_NS::GetObjectRegistry().Create(
                                                                SCENE_NS::ClassId::EcsObject);
                                                            if (auto ecsObject =
                                                                    interface_pointer_cast<SCENE_NS::IEcsObject>(
                                                                        object)) {
                                                                if (auto privateInit = interface_pointer_cast<
                                                                        INodeEcsInterfacePrivate>(ret)) {
                                                                    privateInit->Initialize(scene, ecsObject, {}, "",
                                                                        name, node->SceneHolder(), {});
                                                                }
                                                            }
                                                        }
                                                        return false;
                                                    },
                                                    selfObject),
                                                false);
                                        }
                                    }
                                }
                            }
                            return false;
                        },
                        GetSelf()),
                    false);
            }
        }
        return ret;
    }

    void SetRenderSortLayerOrder(size_t index, uint8_t value) override
    {
        if (submeshHandler_) {
            submeshHandler_->SetRenderSortLayerOrder(index, value);
        } else {
            CORE_LOG_W("%s: mesh is not ready yet, call has no effect", __func__);
        }
    }

    uint8_t GetRenderSortLayerOrder(size_t index) const override
    {
        if (index < SubMeshes()->GetSize()) {
            return META_NS::GetValue(SubMeshes()->GetValueAt(index)->RenderSortLayerOrder());
        }
        return 0u; // default in mesh component
    }

    template<typename INDEX_TYPE>
    void UpdateMeshFromArrays(
        SCENE_NS::MeshGeometryArrayPtr<INDEX_TYPE> arrays, const RENDER_NS::IndexType& indexType, bool append = false)
    {
        if (auto sh = SceneHolder()) {
            if (!append) {
                // Need to invalidate materials up front
                SetMaterial(SCENE_NS::IMaterial::Ptr {});
            }
            SetStatus(SCENE_NS::INode::NODE_STATUS_CONNECTING);
            sh->QueueEngineTask(MakeTask(
                                    [arrays, indexType, append](auto privateIntf) {
                                        // create new mesh entity
                                        if (auto sh = privateIntf->SceneHolder()) {
                                            auto existingEntity = privateIntf->EcsObject()->GetEntity();
                                            auto newEntity = sh->template CreateMeshFromArrays<INDEX_TYPE>(
                                                "TempMesh", arrays, indexType, existingEntity, append);
                                            sh->QueueApplicationTask(
                                                MakeTask(
                                                    [](auto strong) {
                                                        if (auto self = static_cast<MeshImpl*>(strong.get())) {
                                                            self->SetStatus(SCENE_NS::INode::NODE_STATUS_CONNECTED);
                                                            self->SetStatus(
                                                                SCENE_NS::INode::NODE_STATUS_FULLY_CONNECTED);
                                                            if (META_NS::GetValue(self->MaterialOverride())) {
                                                                self->SyncMaterialOverrideToSubmeshes();
                                                            }
                                                        }
                                                        return false;
                                                    },
                                                    privateIntf),
                                                false);
                                        }
                                        return false;
                                    },
                                    GetSelf<INodeEcsInterfacePrivate>()),
                false);
        }
    }

    void UpdateMeshFromArraysI16(SCENE_NS::MeshGeometryArrayPtr<uint16_t> arrays) override
    {
        UpdateMeshFromArrays<uint16_t>(arrays, RENDER_NS::IndexType::CORE_INDEX_TYPE_UINT16);
    }

    void UpdateMeshFromArraysI32(SCENE_NS::MeshGeometryArrayPtr<uint32_t> arrays) override
    {
        UpdateMeshFromArrays<uint32_t>(arrays, RENDER_NS::IndexType::CORE_INDEX_TYPE_UINT32);
    }

    void AddSubmeshesFromArrayI16(SCENE_NS::MeshGeometryArrayPtr<uint16_t> arrays) override
    {
        UpdateMeshFromArrays<uint16_t>(arrays, RENDER_NS::IndexType::CORE_INDEX_TYPE_UINT16, true);
    }

    void AddSubmeshesFromArraysI32(SCENE_NS::MeshGeometryArrayPtr<uint32_t> arrays) override
    {
        UpdateMeshFromArrays<uint32_t>(arrays, RENDER_NS::IndexType::CORE_INDEX_TYPE_UINT32, true);
    }

    void CloneSubmesh(SCENE_NS::ISubMesh::Ptr submesh) override
    {
        if (auto sh = SceneHolder()) {
            sh->QueueEngineTask(MakeTask(
                                    [source = interface_pointer_cast<SCENE_NS::ISubMeshPrivate>(submesh)->GetEntity(),
                                        index = META_NS::GetValue(submesh->Handle())](auto privateIntf) {
                                        privateIntf->SceneHolder()->CopySubMesh(
                                            privateIntf->EcsObject()->GetEntity(), source, index);

                                        return false;
                                    },
                                    GetSelf<INodeEcsInterfacePrivate>()),
                false);
        }
        // Submesh bridge should automatically reflect the updates
    }

    void RemoveSubMesh(size_t index) override
    {
        if (submeshHandler_) {
            submeshHandler_->RemoveSubmesh(index);
        } else {
            CORE_LOG_W("%s: mesh is not ready yet, call has no effect", __func__);
        }
    }
    virtual void RemoveAllSubmeshes() override
    {
        if (submeshHandler_) {
            submeshHandler_->RemoveSubmesh(-1);
        } else {
            CORE_LOG_W("%s: mesh is not ready yet, call has no effect", __func__);
        }
    }

private:
    SCENE_NS::ISubMeshBridge::Ptr submeshHandler_;
};
} // namespace
SCENE_BEGIN_NAMESPACE()
void RegisterMeshImpl()
{
    META_NS::GetObjectRegistry().RegisterObjectType<MeshImpl>();
}
void UnregisterMeshImpl()
{
    META_NS::GetObjectRegistry().UnregisterObjectType<MeshImpl>();
}

SCENE_END_NAMESPACE()
