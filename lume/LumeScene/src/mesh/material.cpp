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

#include "material.h"

#include <scene/ext/util.h>

#include <core/log.h>

#include <meta/api/engine/util.h>
#include <meta/api/make_callback.h>

#include "entity_converting_value.h"
#include "meta/interface/engine/intf_engine_value_manager.h"
#include "texture.h"

SCENE_BEGIN_NAMESPACE()
namespace {
struct ShaderConverter {
    ShaderConverter(const IInternalScene::Ptr& scene, META_NS::Property<CORE3D_NS::MaterialComponent::Shader> p)
        : scene_(scene), p_(p)
    {}

    using SourceType = IShader::Ptr;
    using TargetType = CORE3D_NS::MaterialComponent::Shader;

    SourceType ConvertToSource(META_NS::IAny& any, const TargetType& v) const
    {
        auto p = META_NS::GetPointer<IShader>(any);
        if (auto scene = scene_.lock()) {
            scene
                ->AddTask([&] {
                    if (!p) {
                        p = interface_pointer_cast<IShader>(scene->CreateObject(ClassId::Shader));
                    }
                    if (auto i = interface_cast<IShaderState>(p)) {
                        if (auto rhman = static_cast<CORE3D_NS::IRenderHandleComponentManager*>(
                                scene->GetEcsContext().FindComponent<CORE3D_NS::RenderHandleComponent>())) {
                            i->SetShaderState(scene, rhman->GetRenderHandleReference(v.shader), v.shader,
                                rhman->GetRenderHandleReference(v.graphicsState), v.graphicsState);

                            if (auto ig = interface_cast<IGraphicsState>(p)) {
                                auto s = ig->GetGraphicsState();
                                if (s != v.graphicsState) {
                                    auto copy = v;
                                    copy.graphicsState = s;

                                    p_.GetUnlockedAccess().SetValue(copy);
                                }
                            }
                        }
                    }
                })
                .Wait();
        }
        return p;
    }
    TargetType ConvertToTarget(const SourceType& v) const
    {
        TargetType res;
        if (auto scene = scene_.lock()) {
            if (auto i = interface_cast<IEcsResource>(v)) {
                res.shader = scene->GetEcsContext().GetEntityReference(i->GetEntity());
            }
            if (auto i = interface_cast<IGraphicsState>(v)) {
                res.graphicsState = i->GetGraphicsState();
            }
        }
        return res;
    }

private:
    IInternalScene::WeakPtr scene_;
    META_NS::Property<CORE3D_NS::MaterialComponent::Shader> p_;
};
struct RenderSortConverter {
    using SourceType = SCENE_NS::RenderSort;
    using TargetType = CORE3D_NS::MaterialComponent::RenderSort;

    SourceType ConvertToSource(META_NS::IAny&, const TargetType& v) const
    {
        return SourceType { v.renderSortLayer, v.renderSortLayerOrder };
    }
    TargetType ConvertToTarget(const SourceType& v) const
    {
        return TargetType { v.renderSortLayer, v.renderSortLayerOrder };
    }
};
struct LightingFlagsConverter {
    using SourceType = SCENE_NS::LightingFlags;
    using TargetType = uint32_t;

    SourceType ConvertToSource(META_NS::IAny&, const TargetType& v) const
    {
        return static_cast<SourceType>(v);
    }
    TargetType ConvertToTarget(const SourceType& v) const
    {
        return static_cast<TargetType>(v);
    }
};
} // namespace

CORE_NS::Entity Material::CreateEntity(const IInternalScene::Ptr& scene)
{
    const auto& ecs = scene->GetEcsContext().GetNativeEcs();
    const auto materialManager = CORE_NS::GetManager<CORE3D_NS::IMaterialComponentManager>(*ecs);
    if (!materialManager) {
        return {};
    }
    auto ent = ecs->GetEntityManager().Create();
    materialManager->Create(ent);
    return ent;
}

bool Material::SetEcsObject(const IEcsObject::Ptr& obj)
{
    if (Super::SetEcsObject(obj)) {
        if (auto att = GetSelf<META_NS::IAttach>()) {
            if (auto attc = att->GetAttachmentContainer(false)) {
                if (auto c = attc->FindByName<IInternalMaterial>("MaterialComponent")) {
                    material_ = c;
                }
            }
        }
        if (!material_) {
            auto p = META_NS::GetObjectRegistry().Create<IInternalMaterial>(ClassId::MaterialComponent);
            if (auto acc = interface_cast<IEcsObjectAccess>(p)) {
                if (acc->SetEcsObject(obj)) {
                    material_ = p;
                }
            }
        }
    }
    return material_ != nullptr;
}

bool Material::InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    BASE_NS::string name = p->GetName();
    if (name == "MaterialShader" || name == "DepthShader") {
        auto ep = object_->CreateEngineProperty(path).GetResult();
        auto i = interface_cast<META_NS::IStackProperty>(p);
        return ep && i &&
               i->PushValue(
                   META_NS::IValue::Ptr(new ConvertingValue<ShaderConverter>(ep, { object_->GetScene(), ep })));
    }
    if (name == "Textures") {
        return ConstructTextures(p);
    }
    if (name == "RenderSort") {
        auto ep = material_->RenderSort();
        auto i = interface_cast<META_NS::IStackProperty>(p);
        return ep && i && i->PushValue(META_NS::IValue::Ptr(new ConvertingValue<RenderSortConverter>(ep)));
    }
    if (name == "LightingFlags") {
        auto ep = material_->LightingFlags();
        auto i = interface_cast<META_NS::IStackProperty>(p);
        return ep && i && i->PushValue(META_NS::IValue::Ptr(new ConvertingValue<LightingFlagsConverter>(ep)));
    }
    return false;
}

// clang-format off
const char* const TEXTURE_NAMES[] = {
    "BASE_COLOR",
    "NORMAL",
    "MATERIAL",
    "EMISSIVE",
    "AO",
    "CLEARCOAT",
    "CLEARCOAT_ROUGHNESS",
    "CLEARCOAT_NORMAL",
    "SHEEN",
    "TRANSMISSION",
    "SPECULAR"
};
constexpr size_t TEXTURE_NAMES_SIZE = sizeof(TEXTURE_NAMES) / sizeof(*TEXTURE_NAMES);
// clang-format on

bool Material::ConstructTextures(const META_NS::IProperty::Ptr& p)
{
    auto& r = META_NS::GetObjectRegistry();
    BASE_NS::vector<ITexture::Ptr> texs;
    auto mtex = material_->Textures()->GetValue();
    auto mdata = r.Create<META_NS::IMetadata>(META_NS::ClassId::Object);
    auto index = META_NS::ConstructProperty<size_t>("Index");
    mdata->AddProperty(index);
    for (size_t i = 0; i != mtex.size(); ++i) {
        index->SetValue(i);
        auto t = r.Create<ITexture>(ClassId::Texture, mdata);
        if (auto access = interface_cast<IEcsObjectAccess>(t)) {
            access->SetEcsObject(GetEcsObject());
            if (auto named = interface_cast<META_NS::INamed>(t); named && i < TEXTURE_NAMES_SIZE) {
                named->Name()->SetValue(TEXTURE_NAMES[i]);
            }
            texs.push_back(t);
        } else {
            CORE_LOG_E("Failed to create texture for material");
            return false;
        }
    }
    META_NS::ArrayProperty<ITexture::Ptr> prop { p };
    if (prop) {
        prop->SetValue(texs);
    }
    return static_cast<bool>(prop);
}

META_NS::IMetadata::Ptr Material::GetCustomProperties() const
{
    META_NS::IMetadata::Ptr customs;
    if (const auto obj = GetEcsObject()) {
        BASE_NS::vector<META_NS::IEngineValue::Ptr> values;
        if (SyncCustomProperties(&values)) {
            customs = META_NS::GetObjectRegistry().Create<META_NS::IMetadata>(META_NS::ClassId::Object);
            for (auto&& v : values) {
                BASE_NS::string pname(SCENE_NS::PropertyName(v->GetName()));
                if (auto prop = customs->GetProperty(pname)) {
                    SetEngineValueToProperty(prop, v);
                } else {
                    customs->AddProperty(META_NS::PropertyFromEngineValue(pname, v));
                }
            }
        }
    }
    return customs;
}

META_NS::IProperty::Ptr Material::GetCustomProperty(BASE_NS::string_view name) const
{
    const auto obj = GetEcsObject();
    if (!obj) {
        return nullptr;
    }
    const auto manager = obj->GetEngineValueManager();
    if (!manager) {
        return nullptr;
    }

    if (!SyncCustomProperties({})) {
        CORE_LOG_W("Syncing properties to engine values failed. Using old values if available.");
    }
    BASE_NS::string fullName { name };
    if (!name.starts_with("MaterialComponent.customProperties.")) {
        fullName = "MaterialComponent.customProperties." + fullName;
    }
    return META_NS::PropertyFromEngineValue(fullName, manager->GetEngineValue(fullName));
}

bool Material::SyncCustomProperties(BASE_NS::vector<META_NS::IEngineValue::Ptr>* synced_values) const
{
    const auto obj = GetEcsObject();
    if (!material_ || !obj) {
        return false;
    }
    const auto manager = obj->GetEngineValueManager();
    const auto allCustomProps = META_NS::GetEngineValueFromProperty(material_->CustomProperties());
    if (!manager || !allCustomProps) {
        return false;
    }

    auto doSync = [&]() -> bool {
        // syncing material shader updates the custom properties
        obj->GetScene()->SyncProperty(MaterialShader(), META_NS::EngineSyncDirection::AUTO);
        obj->GetScene()->SyncProperty(material_->CustomProperties(), META_NS::EngineSyncDirection::FROM_ENGINE);
        return manager->ConstructValues(allCustomProps, { "", synced_values });
    };
    return obj->GetScene()->AddTask(doSync).Wait();
}

SCENE_END_NAMESPACE()
