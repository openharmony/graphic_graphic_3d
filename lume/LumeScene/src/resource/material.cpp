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

#include <mutex>
#include <scene/ext/util.h>
#include <set>

#include <3d/ecs/components/name_component.h>
#include <core/log.h>

#include <meta/api/engine/util.h>
#include <meta/api/make_callback.h>
#include <meta/interface/engine/intf_engine_value_manager.h>

#include "entity_converting_value.h"
#include "mesh/texture.h"
#include "util_interfaces.h"

SCENE_BEGIN_NAMESPACE()
namespace {
struct ShaderConverter {
    ShaderConverter(const IInternalScene::Ptr& scene) : scene_(scene) {}

    using SourceType = IShader::Ptr;
    using TargetType = CORE3D_NS::MaterialComponent::Shader;

    SourceType ConvertToSource(META_NS::IAny& any, const TargetType& v) const
    {
        auto p = META_NS::GetPointer<IShader>(any);
        if (auto scene = scene_.lock()) {
            scene
                ->AddTask([&] {
                    if (auto rhman = static_cast<CORE3D_NS::IRenderHandleComponentManager*>(
                            scene->GetEcsContext().FindComponent<CORE3D_NS::RenderHandleComponent>())) {
                        bool changed = false;
                        if (!p) {
                            p = interface_pointer_cast<IShader>(scene->CreateObject(ClassId::Shader));
                            changed = true;
                        }
                        auto sha = rhman->GetRenderHandleReference(v.shader);
                        auto gs = rhman->GetRenderHandleReference(v.graphicsState);

                        if (auto i = interface_cast<IRenderResource>(p)) {
                            changed |= i->GetRenderHandle().GetHandle() != sha.GetHandle();
                        }
                        if (auto i = interface_cast<IGraphicsState>(p)) {
                            changed |= i->GetGraphicsState().GetHandle() != gs.GetHandle();
                        }
                        if (auto i = interface_cast<IShaderState>(p); changed && i) {
                            i->SetShaderState(sha, gs);
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
            if (!v) {
                res.shader = {};
                res.graphicsState = {};
            } else {
                if (auto i = interface_cast<IRenderResource>(v)) {
                    res.shader = HandleFromRenderResource(scene, i->GetRenderHandle());
                }
                if (auto i = interface_cast<IGraphicsState>(v)) {
                    res.graphicsState = HandleFromRenderResource(scene, i->GetGraphicsState());
                }
            }
        }
        return res;
    }

private:
    IInternalScene::WeakPtr scene_;
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
    const auto nameManager = CORE_NS::GetManager<CORE3D_NS::INameComponentManager>(*ecs);
    if (!nameManager) {
        return {};
    }
    auto ent = ecs->GetEntityManager().Create();
    materialManager->Create(ent);
    nameManager->Create(ent);
    return ent;
}

bool Material::SetEcsObject(const IEcsObject::Ptr& obj)
{
    shaderChanged_.Unsubscribe();
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
                    IAttach::Attach(material_);
                }
            }
        }
    }
    return material_ != nullptr;
}

bool Material::InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    BASE_NS::string name = p->GetName();
    if (name == "MaterialShader") {
        auto ep = material_->MaterialShader();
        shaderChanged_.Subscribe(ep, [this]() {
            UpdateTextures(nullptr);
            UpdateCustoms(nullptr);
        });

        auto i = interface_cast<META_NS::IStackProperty>(p);
        return ep && i &&
               i->PushValue(META_NS::IValue::Ptr(new ConvertingValue<ShaderConverter>(ep, { object_->GetScene() })));
    }
    if (name == "DepthShader") {
        auto ep = material_->DepthShader();
        auto i = interface_cast<META_NS::IStackProperty>(p);
        return ep && i &&
               i->PushValue(META_NS::IValue::Ptr(new ConvertingValue<ShaderConverter>(ep, { object_->GetScene() })));
    }
    if (name == "Textures") {
        UpdateTextures(p).Wait();
        return true;
    }
    if (name == "CustomProperties") {
        UpdateCustoms(p).Wait();
        return true;
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

Future<bool> Material::UpdateTextures(const META_NS::IProperty::Ptr& p)
{
    auto property = p;
    if (!property) {
        property = GetProperty("Textures", META_NS::MetadataQuery::EXISTING);
    }
    if (const auto obj = GetEcsObject(); obj && property) {
        if (!textureSyncScheduled_.exchange(true)) {
            return obj->GetScene()->AddTask([this, property]() {
                if (textureSyncScheduled_.load()) {
                    bool success =
                        ConstructTextures(property); // Avoid callback due to ConstructTextures changing target
                    textureSyncScheduled_ = false;
                }
                return false;
            });
        }
    }
    return {};
}

Future<bool> Material::UpdateCustoms(const META_NS::IProperty::Ptr& p)
{
    auto property = p;
    if (!property) {
        property = GetProperty("CustomProperties", META_NS::MetadataQuery::EXISTING);
    }
    if (const auto obj = GetEcsObject(); obj && property) {
        if (!customsSyncScheduled_.exchange(true)) {
            return obj->GetScene()->AddTask([this, property]() {
                if (customsSyncScheduled_.load()) {
                    bool success =
                        UpdateCustomProperties(property); // Avoid callback due to ConstructTextures changing target
                    customsSyncScheduled_ = false;
                }
                return false;
            });
        }
    }
    return {};
}

// clang-format off
const char* const METALLIC_ROUGHNESS_TEXTURE_NAMES[] = {
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
const char* const METALLIC_ROUGHNESS_ECS_TEXTURE_NAMES[] = {
    "baseColor",
    "normal",
    "material",
    "emissive",
    "ambientOcclusion",
    "clearcoat",
    "clearcoatRoughness",
    "clearcoatNormal",
    "sheen",
    "transmission",
    "specular"
};
constexpr size_t TEXTURE_NAMES_SIZE = sizeof(METALLIC_ROUGHNESS_TEXTURE_NAMES) / sizeof(*METALLIC_ROUGHNESS_TEXTURE_NAMES);
// clang-format on

bool Material::UpdateTextureNames(
    const BASE_NS::vector<ITexture::Ptr>& textures, const IInternalMaterial::ActiveTextureSlotInfo& tsi)
{
    bool updated = false;
    for (size_t i = 0; i < textures.size(); ++i) {
        if (auto& t = textures[i]) {
            if (auto named = interface_cast<META_NS::INamed>(t)) {
                auto nameProp = named->Name();
                const auto& name = tsi.slots[i].name;
                if (i < TEXTURE_NAMES_SIZE && name == METALLIC_ROUGHNESS_ECS_TEXTURE_NAMES[i]) {
                    updated |=
                        nameProp->SetDefaultValue(METALLIC_ROUGHNESS_TEXTURE_NAMES[i]) == META_NS::AnyReturn::SUCCESS;
                } else {
                    // Different name to the default ecs texture slot name
                    updated |= nameProp->SetDefaultValue(name) == META_NS::AnyReturn::SUCCESS;
                }
            }
        }
    }
    return updated;
}

bool Material::ConstructTextures(const META_NS::IProperty::Ptr& p)
{
    auto textures = META_NS::ArrayProperty<ITexture::Ptr>(p);
    if (!textures || !material_) {
        return false;
    }
    BASE_NS::vector<ITexture::Ptr> texs = textures->GetValue();
    auto tsi = material_->GetActiveTextureSlotInfo();
    if (texs.size() == tsi.count) {
        if (UpdateTextureNames(texs, tsi)) {
            textures->NotifyChange();
        }
        return true;
    }
    auto mtex = material_->Textures()->GetValue();
    texs.resize(tsi.count);
    auto& r = META_NS::GetObjectRegistry();
    for (size_t i = 0; i < texs.size(); ++i) {
        auto& t = texs[i];
        if (!t) {
            t = r.Create<ITexture>(ClassId::Texture);
            if (auto si = interface_cast<IArrayElementIndex>(t)) {
                si->SetIndex(i);
            }
            if (auto access = interface_cast<IEcsObjectAccess>(t)) {
                access->SetEcsObject(GetEcsObject());
            }
            texs[i] = t;
        }
        if (!t) {
            CORE_LOG_E("Failed to create texture for material");
            return false;
        }
    }
    UpdateTextureNames(texs, tsi);
    if (!texs.empty()) {
        textures->SetValue(texs);
    } else {
        textures->ResetValue();
    }
    return true;
}

bool Material::UpdateCustomProperties(const META_NS::IProperty::Ptr& p)
{
    auto ecsobj = GetEcsObject();
    auto ccs = META_NS::Property<META_NS::IMetadata::Ptr>(p);
    if (!ccs || !material_ || !ecsobj) {
        return false;
    }
    auto meta = ccs->GetValue();
    if (!meta) {
        meta = META_NS::GetObjectRegistry().Create<META_NS::IMetadata>(META_NS::ClassId::Object);
    }
    std::set<BASE_NS::string> props;
    BASE_NS::vector<META_NS::IEngineValue::Ptr> values;
    if (SyncCustomProperties(&values)) {
        for (auto&& v : values) {
            BASE_NS::string pname(SCENE_NS::PropertyName(v->GetName()));
            auto p = meta->GetProperty(pname);
            if (p && GetEngineValueFromProperty(p) != v) {
                auto iany = META_NS::GetInternalAny(p);
                auto eval = interface_cast<META_NS::IValue>(v);
                if (!iany || !eval || !META_NS::IsCompatibleWith(*iany, eval->GetValue())) {
                    CORE_LOG_W("Removing incompatible custom property: %s", pname.c_str());
                    meta->RemoveProperty(p);
                    p = nullptr;
                }
            }
            if (p) {
                if (GetEngineValueFromProperty(p) != v) {
                    auto value = p->GetValue().Clone();
                    if (value && ecsobj->AttachProperty(p, v).GetResult()) {
                        // set the value from the users property
                        p->SetValue(*value);
                    } else {
                        CORE_LOG_W("Failed to attach custom property: %s", pname.c_str());
                        meta->RemoveProperty(p);
                    }
                }
            } else {
                p = ecsobj->CreateProperty(v).GetResult();
                if (p) {
                    meta->AddProperty(p);
                }
            }
            props.insert(pname);
        }
        for (auto&& p : meta->GetProperties()) {
            // we remove properties with engine access, user added, not yet attached properties are left
            if (!props.count(p->GetName()) && GetEngineValueFromProperty(p)) {
                meta->RemoveProperty(p);
            }
        }
        ccs->SetValue(meta);
    }
    return true;
}

META_NS::IMetadata::Ptr Material::GetCustomProperties() const
{
    return CustomProperties()->GetValue();
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
        obj->GetScene()->SyncProperty(material_->MaterialShader(), META_NS::EngineSyncDirection::AUTO);
        obj->GetScene()->SyncProperty(material_->CustomProperties(), META_NS::EngineSyncDirection::FROM_ENGINE);
        return manager->ConstructValues(allCustomProps, { "", synced_values });
    };
    return obj->GetScene()->AddTask(doSync).Wait();
}

void Material::CopyTextureData(const META_NS::IProperty::ConstPtr& p)
{
    if (META_NS::ArrayProperty<const ITexture::Ptr> arr { p }) {
        for (size_t i = 0; i != arr->GetSize(); ++i) {
            if (auto in = interface_cast<META_NS::IMetadata>(arr->GetValueAt(i))) {
                if (auto out = interface_cast<META_NS::IMetadata>(Textures()->GetValueAt(i))) {
                    META_NS::CopyToDefaultAndReset(*in, *out);
                }
            }
        }
    }
}

bool Material::SetResource(const CORE_NS::IResource::Ptr& p)
{
    auto ores = interface_cast<META_NS::IObjectResource>(p);
    if (!ores || p->GetResourceType() != resourceType_.ToUid()) {
        CORE_LOG_W("Invalid resource type");
        return false;
    }
    if (auto resm = interface_cast<META_NS::IMetadata>(p)) {
        if (auto m = static_cast<META_NS::IMetadata*>(this)) {
            for (auto&& p : resm->GetProperties()) {
                if (p->GetName() == "Textures") {
                    CopyTextureData(p);
                } else if (p->GetName() == "CustomProperties") {
                    // skip for now
                } else {
                    META_NS::CopyToDefaultAndReset(p, *m);
                }
            }
        }
    }
    std::unique_lock lock { mutex_ };
    resource_ = p->GetResourceId();
    return true;
}
CORE_NS::IResource::Ptr Material::CreateResource() const
{
    auto r = Super::CreateResource();
    if (auto i = interface_cast<META_NS::IMetadata>(r)) {
        if (auto p = i->GetProperty("CustomProperties")) {
            i->RemoveProperty(p);
        }
    }
    return r;
}

SCENE_END_NAMESPACE()
