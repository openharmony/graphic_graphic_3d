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
#include "submesh.h"

#include <3d/ecs/components/material_component.h>

#include "../entity_converting_value.h"

SCENE_BEGIN_NAMESPACE()

static constexpr BASE_NS::string_view MATERIAL_PROPERTY_NAME = "Material";

class MaterialConverter : public DynamicInterfacePtrEntityConverter<IMaterial> {
public:
    MaterialConverter(IInternalScene::Ptr scene) : DynamicInterfacePtrEntityConverter<IMaterial>(scene) {}

protected:
    META_NS::ObjectId GetObjectId(const CORE_NS::IEcs& ecs, const CORE_NS::Entity& e) const override
    {
        // What ClassId to create for material depends on the material's type
        auto id = ClassId::Material;
        auto m = CORE_NS::GetManager<CORE3D_NS::IMaterialComponentManager>(ecs);
        if (m) {
            if (auto rh = m->Read(e)) {
                if (rh->type == CORE3D_NS::MaterialComponent::Type::OCCLUSION) {
                    id = ClassId::OcclusionMaterial;
                }
            }
        }
        return id;
    }
};

bool SubMesh::SetEcsObject(const IEcsObject::Ptr& ecso)
{
    return Super::SetEcsObject(ecso);
}

bool SubMesh::InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    if (index_ == -1) {
        CORE_LOG_E("Submesh index is uninitialised");
        return false;
    }
    BASE_NS::string cpath = "MeshComponent.submeshes[" + BASE_NS::string(BASE_NS::to_string(index_)) + "]." + path;

    if (p->GetName() == MATERIAL_PROPERTY_NAME) {
        auto ep = object_->CreateProperty(cpath).GetResult();
        auto i = interface_cast<META_NS::IStackProperty>(p);
        return ep && i &&
               i->PushValue(META_NS::IValue::Ptr(new ConvertingValue<MaterialConverter>(ep, { GetInternalScene() })));
    }
    return AttachEngineProperty(p, cpath);
}

void SubMesh::OnPropertyChanged(const META_NS::IProperty& p)
{
    const auto name = p.GetName();
    if (name != "Name") {
        META_NS::Invoke<META_NS::IOnChanged>(event_);
    }
}

IInternalScene::Ptr SubMesh::GetScene() const
{
    return object_ ? object_->GetScene() : nullptr;
}

void SubMesh::SetMaterialOverride(const IMaterial::Ptr& material)
{
    if (overrideMaterial_.lock() == material) {
        return; // No change, ignore the call
    }
    if (auto scene = GetScene()) {
        // Need to initialize the material property to handle original value
        auto raw = GetProperty(MATERIAL_PROPERTY_NAME, META_NS::MetadataQuery::CONSTRUCT_ON_REQUEST);
        META_NS::PropertyLock materialProperty { raw };
        if (materialProperty) {
            auto v = materialProperty->GetValueAny().Clone();
            if (material) { // Setting override
                if (overrideAny_) {
                    // Already override set, update value
                    if (auto i = META_NS::GetFirstValueFromProperty<IConvertingValue>(raw)) {
                        if (auto val = interface_cast<META_NS::IValue>(i)) {
                            val->SetValue(META_NS::Any<IMaterial::Ptr>(material));
                        }
                    }
                } else {
                    // Didn't have override earlier, add one to stack
                    overrideAny_ = v;
                    materialProperty->SetValueAny(META_NS::Any<IMaterial::Ptr>(material));
                    materialProperty->PushValue(v);
                }
            } else { // Reseting override
                if (overrideAny_) {
                    // Remove override from stack
                    materialProperty->RemoveValue(overrideAny_);
                    overrideAny_.reset();
                }
                materialProperty->SetValueAny(*v);
            }
            // Need ECS sync
            scene->SyncProperty(raw, META_NS::EngineSyncDirection::AUTO);
        }
        overrideMaterial_ = material;
    }
}

IMaterial::Ptr SubMesh::GetMaterialOverride() const
{
    return overrideMaterial_.lock();
}

SCENE_END_NAMESPACE()
