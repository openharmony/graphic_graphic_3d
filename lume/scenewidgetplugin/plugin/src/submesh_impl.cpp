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

#include <meta/ext/concrete_base_object.h>

#include "bind_templates.inl"
#include "intf_submesh_bridge.h"
#include "node_impl.h"
#include "submesh_handler_uid.h"
#include "task_utils.h"

using SCENE_NS::MakeTask;

namespace {
class SubMeshImpl : public META_NS::ObjectFwd<SubMeshImpl, SCENE_NS::ClassId::SubMesh, META_NS::ClassId::Object,
                        SCENE_NS::ISubMesh, SCENE_NS::ISubMeshPrivate> {
    META_IMPLEMENT_INTERFACE_PROPERTY(META_NS::INamed, BASE_NS::string, Name, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::ISubMesh, SCENE_NS::IMaterial::Ptr, Material, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ISubMesh, uint64_t, Handle, {}, META_NS::ObjectFlagBits::INTERNAL)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ISubMesh, BASE_NS::Math::Vec3, AABBMin,
        BASE_NS::Math::Vec3(0.f, 0.f, 0.f), META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ISubMesh, BASE_NS::Math::Vec3, AABBMax,
        BASE_NS::Math::Vec3(0.f, 0.f, 0.f), META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::ISubMesh, uint8_t, RenderSortLayerOrder, 0u, META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(SCENE_NS::ISubMesh, BASE_NS::string, MaterialUri, "",
        META_NS::DEFAULT_PROPERTY_FLAGS | META_NS::ObjectFlagBits::INTERNAL)

public:
    bool Build(const META_NS::IMetadata::Ptr& data) override
    {
        // subscribe to material changes.
        Material()->OnChanged()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([this]() {
            // Material has changed.
            auto material = Material()->GetValue();

            // If this change was not triggered by us, we will store the value.
            if (!isSettingDefaults_) {
                SaveCurrentMaterial(material);
            }

            // Override material always overrides everything.
            // This could use validator as well, but MetaObject Validate() function was not invoked properly.
            if (overrideMaterial_) {
                material = overrideMaterial_;
            } else if (!material) {
                // No material / case reset.
                material = defaultMaterial_;
            }

            UpdateMaterialToScene(material);
        }));

        return true;
    }

    void SetRenderSortLayerOrder(uint8_t order) override
    {
        for (auto& ite : submeshHandlers_) {
            if (auto bridge = ite.lock()) {
                bridge->SetRenderSortLayerOrder(META_NS::GetValue(Handle()), order);
            }
        }
    }

    void SetAABBMin(BASE_NS::Math::Vec3 vec) override
    {
        for (auto& ite : submeshHandlers_) {
            if (auto bridge = ite.lock()) {
                bridge->SetAABBMin(META_NS::GetValue(Handle()), vec);
            }
        }
    }

    void SetAABBMax(BASE_NS::Math::Vec3 vec) override
    {
        for (auto& ite : submeshHandlers_) {
            if (auto bridge = ite.lock()) {
                bridge->SetAABBMax(META_NS::GetValue(Handle()), vec);
            }
        }
    }

    void SetMaterial(SCENE_NS::IMaterial::Ptr material) override
    {
        Material()->SetValue(material);
    }

    void UpdateMaterialToScene(SCENE_NS::IMaterial::Ptr material)
    {
        // Activate material to scene.
        for (auto& ite : submeshHandlers_) {
            if (auto bridge = ite.lock()) {
                bridge->SetMaterialToEcs(META_NS::GetValue(Handle()), material);
            }
        }
    }

public: // ISubMeshPrivate
    void AddSubmeshBrigde(SCENE_NS::ISubMeshBridge::Ptr bridge) override
    {
        for (auto& existingBridge : submeshHandlers_) {
            if (bridge == existingBridge.lock()) {
                return;
            }
        }

        submeshHandlers_.push_back(bridge);

        // Store initial material that we have assigned right now.
        currentMaterial_ = GetValue(Material());
        if (currentMaterial_) {
            bridge->SetMaterialToEcs(META_NS::GetValue(Handle()), currentMaterial_);
        }
    }

    virtual void RemoveSubmeshBrigde(SCENE_NS::ISubMeshBridge::Ptr bridge) override
    {
        for (auto ite = submeshHandlers_.cbegin(); ite != submeshHandlers_.cend();) {
            auto strong = ite->lock();
            if (!strong || bridge == strong) {
                ite = submeshHandlers_.erase(ite);
            } else {
                ite++;
            }
        }
    }

    CORE_NS::Entity GetEntity() const override
    {
        if (!submeshHandlers_.empty()) {
            if (auto first = submeshHandlers_.begin()->lock()) {
                return first->GetEntity();
            }
        }

        return {};
    }

    void RestoreDefaultMaterial() override
    {
        Material()->SetValue(defaultMaterial_);
    }

    void SetDefaultMaterial(SCENE_NS::IMaterial::Ptr material) override
    {
        isSettingDefaults_ = true;

        if (!defaultMaterialIsSet_) {
            // Store default material.
            defaultMaterial_ = material;
            defaultMaterialIsSet_ = true;

            // also make it default to this property, so that Reset() will work.
            // Currently this causes material (and its changes) not to be serialized to .ui
            // auto* directAccess =
            // interface_cast<META_NS::IPropertyDirectAccess<SCENE_NS::IMaterial::Ptr>>(Material()); if (directAccess) {
            //}

            // Activate default material if there is no current material.
            if (!META_NS::GetValue(Material())) {
                Material()->SetValue(material);
            }
        }

        isSettingDefaults_ = false;
    }

    void SetOverrideMaterial(SCENE_NS::IMaterial::Ptr material) override
    {
        isSettingDefaults_ = true;

        // Store override material.
        overrideMaterial_ = material;

        // If override material, make it active.
        if (overrideMaterial_) {
            UpdateMaterialToScene(overrideMaterial_);
        } else {
            // Otherwise restore the current material / default material.
            UpdateMaterialToScene(currentMaterial_ ? currentMaterial_ : defaultMaterial_);
        }
        isSettingDefaults_ = false;
    }

    void SaveCurrentMaterial(SCENE_NS::IMaterial::Ptr material)
    {
        // This material also becomes the "current" value.
        currentMaterial_ = material;

        if (!currentMaterial_) {
            Material()->SetValue(defaultMaterial_);
        }
    }

    bool defaultMaterialIsSet_ { false };
    SCENE_NS::IMaterial::Ptr defaultMaterial_;
    SCENE_NS::IMaterial::Ptr currentMaterial_;
    SCENE_NS::IMaterial::Ptr overrideMaterial_;

    bool isSettingDefaults_ { false };
    BASE_NS::vector<SCENE_NS::ISubMeshBridge::WeakPtr> submeshHandlers_;
};
} // namespace
SCENE_BEGIN_NAMESPACE()

void RegisterSubMeshImpl()
{
    META_NS::GetObjectRegistry().RegisterObjectType<SubMeshImpl>();
}
void UnregisterSubMeshImpl()
{
    META_NS::GetObjectRegistry().UnregisterObjectType<SubMeshImpl>();
}
SCENE_END_NAMESPACE()
