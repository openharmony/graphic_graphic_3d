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
#include <3d/ecs/components/material_component.h>
#include <atomic>
#include <meta/api/make_callback.h>
#include <meta/interface/detail/array_property.h>
#include <meta/api/property/array_element_bind.h>
#include <meta/ext/concrete_base_object.h>

#include "bind_templates.inl"
#include "intf_proxy_object_holder.h"
#include "node_impl.h"
#include "task_utils.h"

using CORE3D_NS::MaterialComponent;
META_BEGIN_NAMESPACE()
META_TYPE(MaterialComponent::TextureInfo);
META_END_NAMESPACE()

using SCENE_NS::MakeTask;
#define GET_HOLDER(info) interface_pointer_cast<SCENE_NS::IProxyObjectHolder>(info)->Holder()

namespace {
template<typename... Types>
inline void BindMetaProperty(
    PropertyHandlerArrayHolder& handler, META_NS::IProperty::Ptr& clone, META_NS::IProperty::Ptr& prop)
{
    (BindIfCorrectType<Types>(handler, clone, prop) || ...);
}

void BindMetaProperty(
    PropertyHandlerArrayHolder& handler, META_NS::IProperty::Ptr& clone, META_NS::IProperty::Ptr& prop)
{
    BindMetaProperty<BASE_NS::Math::Vec4, BASE_NS::Math::UVec4, BASE_NS::Math::IVec4, BASE_NS::Math::Vec3,
        BASE_NS::Math::UVec3, BASE_NS::Math::IVec3, BASE_NS::Math::Vec2, BASE_NS::Math::UVec2, BASE_NS::Math::IVec2,
        float, int32_t, uint32_t, BASE_NS::Math::Mat3X3, BASE_NS::Math::Mat4X4, bool, CORE_NS::Entity,
        CORE_NS::EntityReference>(handler, clone, prop);
}

bool IsGltfResource(BASE_NS::string_view uri, BASE_NS::string_view resourcePath)
{
    // Image uris loaded form a glTF are in a format like this: "file://model.gltf/images/0"

    // There must be an identifier string at the end of the uri after the '/'.
    const auto idSeparator = uri.find_last_of('/');
    if (idSeparator == BASE_NS::string_view::npos) {
        return false;
    }

    // We don't care what is after the separator, but it should be preceded by <resourcePath>" e.g. /images".
    auto rest = uri.substr(0, idSeparator);
    if (!rest.ends_with(resourcePath)) {
        return false;
    }
    rest = rest.substr(0, rest.size() - resourcePath.size());

    // Take the last 5 characters in lower case to check the extension.
    if (rest.size() < 5) {
        return false;
    }
    const auto extension = BASE_NS::string(rest.substr(rest.size() - 5, 5)).toLower();
    if (!extension.ends_with(".glb") && !extension.ends_with(".gltf")) {
        return false;
    }

    return true;
}

bool IsGltfImage(BASE_NS::string_view uri)
{
    return IsGltfResource(uri, "/images");
}

bool IsGltfMaterial(BASE_NS::string_view uri)
{
    return IsGltfResource(uri, "/materials");
}

class MaterialImpl : public META_NS::ConcreteBaseMetaObjectFwd<MaterialImpl, NodeImpl, SCENE_NS::ClassId::Material,
                         SCENE_NS::IMaterial, ITextureStorage> {
    static constexpr BASE_NS::string_view MATERIAL_COMPONENT_NAME = "MaterialComponent";

    static constexpr BASE_NS::string_view CUSTOM_PREFIX = "MaterialComponent.customProperties.";
    static constexpr size_t CUSTOM_PREFIX_SIZE = CUSTOM_PREFIX.size();
    static constexpr BASE_NS::string_view TEXTURE_PREFIX = "MaterialComponent.TextureInfo.";
    static constexpr size_t TEXTURE_PREFIX_SIZE = TEXTURE_PREFIX.size();

    bool Build(const IMetadata::Ptr& data) override
    {
        bool ret = false;
        if (ret = NodeImpl::Build(data); ret) {
            // bind everything from material component, otherwise obey the wishes of other components
            PropertyNameMask()[MATERIAL_COMPONENT_NAME] = {};
            OnTypeChanged(); // initialize inputs
        }
        return ret;
    }

    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IMaterial, uint8_t, Type, 0)

    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IMaterial, float, AlphaCutoff, 1.f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IMaterial, uint32_t, LightingFlags,
        CORE3D_NS::MaterialComponent::LightingFlagBits::SHADOW_RECEIVER_BIT |
            CORE3D_NS::MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT |
            CORE3D_NS::MaterialComponent::LightingFlagBits::PUNCTUAL_LIGHT_RECEIVER_BIT |
            CORE3D_NS::MaterialComponent::LightingFlagBits::INDIRECT_LIGHT_RECEIVER_BIT)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IMaterial, SCENE_NS::IShader::Ptr, MaterialShader, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IMaterial, SCENE_NS::IGraphicsState::Ptr, MaterialShaderState, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IMaterial, SCENE_NS::IShader::Ptr, DepthShader, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IMaterial, SCENE_NS::IGraphicsState::Ptr, DepthShaderState, {})

    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(SCENE_NS::IMaterial, META_NS::IObject::Ptr, CustomProperties, {})
    META_IMPLEMENT_INTERFACE_ARRAY_PROPERTY(SCENE_NS::IMaterial, SCENE_NS::ITextureInfo::Ptr, Inputs, {})
    META_IMPLEMENT_INTERFACE_ARRAY_PROPERTY(SCENE_NS::IMaterial, SCENE_NS::ITextureInfo::Ptr, Textures, {})


    bool updatingInputs_ { false };

    void Activate() override
    {
        auto sceneHolder = SceneHolder();
        if (!sceneHolder) {
            return;
        }

        auto task = META_NS::MakeCallback<META_NS::ITaskQueueTask>(
            [e = ecsObject_->GetEntity(), w = BASE_NS::weak_ptr(sceneHolder)] {
                auto sh = w.lock();
                if (sh) {
                    sh->SetEntityActive(e, true);
                }
                return false;
            });
        sceneHolder->QueueEngineTask(task, false);
    }

    void Deactivate() override
    {
        auto sceneHolder = SceneHolder();
        if (!sceneHolder) {
            return;
        }

        auto task = META_NS::MakeCallback<META_NS::ITaskQueueTask>(
            [e = ecsObject_->GetEntity(), w = BASE_NS::weak_ptr(sceneHolder)] {
                auto sh = w.lock();
                if (sh) {
                    sh->SetEntityActive(e, false);
                }
                return false;
            });
        sceneHolder->QueueEngineTask(task, false);
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

            initializeTaskToken_ = scene->AddEngineTask(MakeTask([me = BASE_NS::weak_ptr(GetSelf()),
                                                                     materialName = name,
                                                                     fullpath = path + name, entity]() {
                if (auto self = static_pointer_cast<NodeImpl>(me.lock())) {
                    if (auto sceneHolder = self->SceneHolder()) {
                        CORE_NS::Entity materialEntity = entity;

                        if (CORE_NS::EntityUtil::IsValid(materialEntity)) {
                            SCENE_PLUGIN_VERBOSE_LOG("binding material: %s", materialName.c_str());
                            if (auto proxyIf = interface_pointer_cast<SCENE_NS::IEcsProxyObject>(self->EcsObject())) {
                                proxyIf->SetCommonListener(sceneHolder->GetCommonEcsListener());
                            }
                            self->EcsObject()->DefineTargetProperties(self->PropertyNameMask());
                            self->EcsObject()->SetEntity(sceneHolder->GetEcs(), materialEntity);
                            sceneHolder->QueueApplicationTask(
                                MakeTask(
                                    [fullpath](auto selfObject) {
                                        if (auto self = static_pointer_cast<NodeImpl>(selfObject)) {
                                            self->CompleteInitialization(fullpath);
                                            self->SetStatus(SCENE_NS::INode::NODE_STATUS_CONNECTED);
                                            if (auto node = interface_pointer_cast<SCENE_NS::INode>(selfObject)) {
                                                META_NS::Invoke<META_NS::IOnChanged>(node->OnLoaded());
                                            }
                                        }
                                        return false;
                                    },
                                    me),
                                false);
                        } else {
                            CORE_LOG_W("Could not find '%s' material", fullpath.c_str());
                            sceneHolder->QueueApplicationTask(
                                MakeTask(
                                    [](auto selfObject) {
                                        if (auto self = static_pointer_cast<NodeImpl>(selfObject)) {
                                            self->SetStatus(SCENE_NS::INode::NODE_STATUS_DISCONNECTED);
                                        }
                                        return false;
                                    },
                                    me),
                                false);
                        }
                    }
                }
                return false;
            }),
                false);
        }
    }

public: // ISerialization
    void ApplyTextureInfoImage(size_t arrayIndex, SCENE_NS::ITextureInfo& info)
    {
        if (auto sceneHolder = SceneHolder()) {
            bool shouldReset { true };
            auto value = info.Image()->GetValue();
            if (auto uriBitmap = interface_cast<SCENE_NS::IBitmap>(value)) {
                auto uri = META_NS::GetValue(uriBitmap->Uri());
                if (!uri.empty()) {
                    shouldReset = false;

                    if (IsGltfImage(uri)) {
                        return;
                    }

                    sceneHolder->QueueEngineTask(
                        MakeTask(
                            [uri, entityId = EcsObject()->GetEntity().id, arrayIndex](auto sh) {
                                CORE_NS::Entity target { entityId };
                                auto image = sh->LoadImage(uri);
                                sh->SetTexture(arrayIndex, target, image);

                                return false;
                            },
                            sceneHolder),
                        false);
                }
            }
            if (shouldReset) {
                sceneHolder->QueueEngineTask(MakeTask(
                                                 [entity = EcsObject()->GetEntity(), arrayIndex](auto sh) {
                                                     sh->SetTexture(arrayIndex, entity, {});
                                                     return false;
                                                 },
                                                 sceneHolder),
                    false);
            }
        }
    }

    void SubscribeToTextureInfo(size_t arrayIndex, SCENE_NS::ITextureInfo::Ptr info)
    {
        info->SetTextureSlotIndex(arrayIndex);

        // EcsObject would provide use entity ref while we would like to use the enums to define which
        // sampler to use Perhaps we create new factory method for scene-interface and then apply the direct
        // biding from now on
        GET_HOLDER(info)
            .NewHandler(info->Sampler(), nullptr)
            .Subscribe(info->Sampler(), [this, prop = info->Sampler(), arrayIndex]() {
                if (auto sceneHolder = SceneHolder()) {
                    sceneHolder->QueueEngineTask(
                        MakeTask(
                            [value = prop->GetValue(), entityId = EcsObject()->GetEntity().id, arrayIndex](auto sh) {
                                CORE_NS::Entity target { entityId };
                                sh->SetSampler(
                                    arrayIndex, target, static_cast<SCENE_NS::ITextureInfo::SamplerId>(value));

                                return false;
                            },
                            sceneHolder),
                        false);
                }
            });

        GET_HOLDER(info)
            .NewHandler(info->Image(), nullptr)
            .Subscribe(info->Image(),
                [this, textureInfo = info.get(), arrayIndex]() { ApplyTextureInfoImage(arrayIndex, *textureInfo); });
    }
    /*
    bool Export(
        META_NS::Serialization::IExportContext& context, META_NS::Serialization::ClassPrimitive& value) const override
    {
        SCENE_PLUGIN_VERBOSE_LOG("MaterialImpl::%s %s", __func__, Name()->Get().c_str());

        // This is not a shared material and therefore we will export it here.
        return Fwd::Export(context, value);
    }

    bool Import(
        META_NS::Serialization::IImportContext& context, const META_NS::Serialization::ClassPrimitive& value) override
    {
        META_ACCESS_PROPERTY(Inputs)->Reset();
        META_ACCESS_PROPERTY(CustomProperties)->Reset();

        if (!Fwd::Import(context, value)) {
            return false;
        }

        if (Inputs()) {
            // Subscribe all texture infos.
            for (auto i = 0; i < Inputs()->Size(); ++i) {
                auto info = Inputs()->Get(i);

                auto textureSlotIndex = info->GetTextureSlotIndex();
                if (textureSlotIndex == ~0u) {
                    textureSlotIndex = i;
                }

                info->SetTextureSlotIndex(textureSlotIndex);
            }
        }

        return true;
    }
    */
    SCENE_NS::ITextureInfo::Ptr GetTextureInfoByIndex(
        uint32_t index, BASE_NS::vector<SCENE_NS::ITextureInfo::Ptr>& textures)
    {
        // First, look from current inputs.
        auto existingInfo = GetTextureInfo(index);
        if (existingInfo) {
            // Found existing info, we absolutely want to re-use it.
            // This is to keep e.g. property animations working (they bind to this exact object).
            auto it = std::find(textures.begin(), textures.end(), existingInfo);
            if (it == textures.end()) {
                // Add this info to textures list.
                textures.push_back(existingInfo);
            }
            return existingInfo;
        }

        for (auto& info : textures) {
            if (info->GetTextureSlotIndex() == index) {
                return info;
            }
        }

        return {};
    }

    SCENE_NS::ITextureInfo::Ptr BindTextureSlot(const BASE_NS::string_view& propName, META_NS::IMetadata::Ptr& meta,
        BASE_NS::vector<SCENE_NS::ITextureInfo::Ptr>& textures)
    {
        SCENE_NS::ITextureInfo::Ptr result;

        // fork out the identifier
        size_t ix = TEXTURE_PREFIX_SIZE;
        ix = propName.find('.', ix + 1);
        size_t textureSlotId = 0;

        if (ix == BASE_NS::string_view::npos) {
            return result;
        } else {
            // how cool is this
            for (int ii = TEXTURE_PREFIX_SIZE; ii < ix; ii++) {
                textureSlotId += (propName[ii] - '0') * pow(10, (ix - ii - 1)); // 10 exponent
            }
        }

        auto textureSlotIndex = textureSlotId - 1;

        bool applyPropertyToEcs = false;
        if (textureSlotId) {
            auto info = GetTextureInfoByIndex(textureSlotIndex, textures);
            if (!info) {
                info = GetObjectRegistry().Create<SCENE_NS::ITextureInfo>(SCENE_NS::ClassId::TextureInfo);
                if (!info) {
                    return {};
                }

                info->SetTextureSlotIndex(textureSlotIndex);
                textures.push_back(info);
            }

            GET_HOLDER(info).SetSceneHolder(SceneHolder());

            {
                // Update name.
                BASE_NS::string textureSlotName(propName.substr(ix + 1));
                size_t dotIndex = textureSlotName.find('.');
                if (dotIndex != BASE_NS::string_view::npos && dotIndex != 0) {
                    textureSlotName = textureSlotName.substr(0, dotIndex);
                }
                if (GetValue(info->Name()) != textureSlotName) {
                    SetValue(info->Name(), textureSlotName);
                }
            }

            // progress one more dot
            ix = propName.find('.', ix + 1);

            BASE_NS::string propertyName(propName.substr(ix));
            if (propertyName == ".factor") {
                BindChanges<BASE_NS::Math::Vec4>(GET_HOLDER(info), info->Factor(), meta, propName);
            } else if (propertyName == ".transform.translation") {
                BindChanges<BASE_NS::Math::Vec2>(GET_HOLDER(info), info->Translation(), meta, propName);
            } else if (propertyName == ".transform.rotation") {
                BindChanges<float>(GET_HOLDER(info), info->Rotation(), meta, propName);
            } else if (propertyName == ".transform.scale") {
                BindChanges<BASE_NS::Math::Vec2>(GET_HOLDER(info), info->Scale(), meta, propName);
            } else if (propertyName == ".image") {
                auto prop = meta->GetPropertyByName(propName);

                // Here we will initialize the image property.
                if (info->Image()->IsValueSet()) {
                    // If the property value is set by the user, we will propagate it to ecs.
                    ApplyTextureInfoImage(textureSlotIndex, *info);
                } else {
                    // If the property value is not set, we will fetch it from ecs.
                    CheckImageHandle(textureSlotIndex, BASE_NS::weak_ptr<SCENE_NS::ITextureInfo>(info));
                }

                prop->OnChanged()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>(
                    [this, textureSlotIndex, w_info = Base::weak_ptr(info)]() {
                        CheckImageHandle(textureSlotIndex, w_info);
                    }));
                GET_HOLDER(info).MarkRelated(info->Image(), prop);

                SubscribeToTextureInfo(textureSlotIndex, info);
            } else if (propertyName == ".sampler") {
                GET_HOLDER(info).MarkRelated(info->Sampler(), meta->GetPropertyByName(propName));
            }

            result = info;
        }

        return result;
    }

    void CheckImageHandle(size_t arrayIx, BASE_NS::weak_ptr<SCENE_NS::ITextureInfo> w_info)
    {
        // Queue engine task
        if (auto sceneHolder = SceneHolder()) {
            sceneHolder->QueueEngineTask(
                MakeTask(
                    [entity = EcsObject()->GetEntity(), arrayIx, w_info](SceneHolder::Ptr sh) {
                        // resolve entity
                        CORE_NS::Entity image;
                        BASE_NS::string name;
                        RENDER_NS::GpuImageDesc imageDesc;
                        RENDER_NS::RenderHandleReference handle;
                        if (sh->GetImageEntity(entity, arrayIx, image)) {
                            // get entity by uri
                            sh->GetEntityUri(image, name);
                            sh->GetImageHandle(image, handle, imageDesc);
                        }
                        SCENE_PLUGIN_VERBOSE_LOG("%s: resolved name: %s", __func__, name.c_str());
                        // if we get something valid out of those
                        // let the toolkit side check if we have a bitmap and its uri matches
                        if (!name.empty()) {
                            sh->QueueEngineTask(
                                MakeTask(
                                    [name](auto info, RENDER_NS::RenderHandleReference handle, auto size) {
                                        if (auto bitmap = META_NS::GetValue(info->Image())) {
                                            if (auto uriBitmap = interface_cast<SCENE_NS::IBitmap>(bitmap)) {
                                                if (auto uri = META_NS::GetValue(uriBitmap->Uri());
                                                    !uri.empty() && (uri == name)) {
                                                    return false;
                                                }
                                            }
                                        }
                                        auto uriBitmap = META_NS::GetObjectRegistry().Create<SCENE_NS::IBitmap>(
                                            SCENE_NS::ClassId::Bitmap);
                                        META_NS::SetValue(uriBitmap->Uri(), name);
                                        // TODO: should parse the renderhandle from the entity and set it to the bitmap
                                        // also. as we "create" it from the ecs, so the image SHOULD already be loaded.
                                        uriBitmap->SetRenderHandle(handle, size);
                                        info->Image()->SetDefaultValue(uriBitmap, true);
                                        return false;
                                    },
                                    w_info, handle, BASE_NS::Math::UVec2(imageDesc.width, imageDesc.height)),
                                false);
                        }
                        return false;
                    },
                    sceneHolder),
                false);
        }
    }

    SCENE_NS::ITextureInfo::Ptr GetTextureInfo(size_t ix) const override
    {
        if (Inputs()) {
            for (auto& info : Inputs()->GetValue()) {
                if (info->GetTextureSlotIndex() == ix) {
                    return info;
                }
            }
        }

        return SCENE_NS::ITextureInfo::Ptr {};
    }

    void UpdateInputProperties()
    {
        auto type = META_NS::GetValue(META_ACCESS_PROPERTY(Type));
        bool includeDefaultMappings =
            (type == SCENE_NS::IMaterial::METALLIC_ROUGHNESS || type == SCENE_NS::IMaterial::SPECULAR_GLOSSINESS ||
                type == SCENE_NS::IMaterial::UNLIT || type == SCENE_NS::IMaterial::UNLIT_SHADOW_ALPHA);

        // Base Color
        if (auto meta =
                interface_pointer_cast<META_NS::IMetadata>(GetTextureInfo(CORE3D_NS::MaterialComponent::BASE_COLOR))) {
            if (auto color = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_COLOR)) {
                if (!includeDefaultMappings) {
                    meta->RemoveProperty(color);
                }
            } else {
                auto c = META_NS::ConstructProperty<SCENE_NS::Color>(SCENE_NS::IMaterial::MAPPED_INPUTS_COLOR);
                meta->AddProperty(c);
            }
        }

        // Normal
        if (auto meta =
                interface_pointer_cast<META_NS::IMetadata>(GetTextureInfo(CORE3D_NS::MaterialComponent::NORMAL))) {
            if (auto scale = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_NORMAL_SCALE)) {
                if (!includeDefaultMappings) {
                    meta->RemoveProperty(scale);
                }
            } else {
                auto s = META_NS::ConstructProperty<float>(SCENE_NS::IMaterial::MAPPED_INPUTS_NORMAL_SCALE);
                meta->AddProperty(s);
            }
        }

        //  Material
        if (auto meta =
                interface_pointer_cast<META_NS::IMetadata>(GetTextureInfo(CORE3D_NS::MaterialComponent::MATERIAL))) {
            // SCENE_NS::IMaterial::METALLIC_ROUGHNESS
            if (auto roughness = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_ROUGHNESS)) {
                if (type != SCENE_NS::IMaterial::METALLIC_ROUGHNESS) {
                    meta->RemoveProperty(roughness);
                }
            } else if (type == SCENE_NS::IMaterial::METALLIC_ROUGHNESS) {
                auto roughness = META_NS::ConstructProperty<float>(SCENE_NS::IMaterial::MAPPED_INPUTS_ROUGHNESS);
                meta->AddProperty(roughness);
            }
            if (auto metallic = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_METALLIC)) {
                if (type != SCENE_NS::IMaterial::METALLIC_ROUGHNESS) {
                    meta->RemoveProperty(metallic);
                }
            } else if (type == SCENE_NS::IMaterial::METALLIC_ROUGHNESS) {
                auto metallic = META_NS::ConstructProperty<float>(SCENE_NS::IMaterial::MAPPED_INPUTS_METALLIC);
                meta->AddProperty(metallic);
            }
            if (auto reflectance = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_REFLECTANCE)) {
                if (type != SCENE_NS::IMaterial::METALLIC_ROUGHNESS) {
                    meta->RemoveProperty(reflectance);
                }
            } else if (type == SCENE_NS::IMaterial::METALLIC_ROUGHNESS) {
                auto reflectance = META_NS::ConstructProperty<float>(SCENE_NS::IMaterial::MAPPED_INPUTS_REFLECTANCE);
                meta->AddProperty(reflectance);
            }
            // SCENE_NS::IMaterial::SPECULAR_GLOSSINESS
            if (auto colorRGB = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_COLOR)) {
                if (type != SCENE_NS::IMaterial::SPECULAR_GLOSSINESS) {
                    meta->RemoveProperty(colorRGB);
                }
            } else if (type == SCENE_NS::IMaterial::SPECULAR_GLOSSINESS) {
                auto colorRGB = META_NS::ConstructProperty<SCENE_NS::Color>(SCENE_NS::IMaterial::MAPPED_INPUTS_COLOR);
                meta->AddProperty(colorRGB);
            }
            if (auto glossiness = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_GLOSSINESS)) {
                if (type != SCENE_NS::IMaterial::SPECULAR_GLOSSINESS) {
                    meta->RemoveProperty(glossiness);
                }
            } else if (type == SCENE_NS::IMaterial::SPECULAR_GLOSSINESS) {
                auto glossiness = META_NS::ConstructProperty<float>(SCENE_NS::IMaterial::MAPPED_INPUTS_GLOSSINESS);
                meta->AddProperty(glossiness);
            }
        }

        // Emissive
        if (auto meta =
                interface_pointer_cast<META_NS::IMetadata>(GetTextureInfo(CORE3D_NS::MaterialComponent::EMISSIVE))) {
            if (auto color = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_COLOR)) {
                if (!includeDefaultMappings) {
                    meta->RemoveProperty(color);
                }
            } else {
                auto c = META_NS::ConstructProperty<SCENE_NS::Color>(SCENE_NS::IMaterial::MAPPED_INPUTS_COLOR);
                meta->AddProperty(c);
            }
            if (auto prop = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_INTENSITY)) {
                if (!includeDefaultMappings) {
                    meta->RemoveProperty(prop);
                }
            } else {
                auto p = META_NS::ConstructProperty<float>(SCENE_NS::IMaterial::MAPPED_INPUTS_INTENSITY);
                meta->AddProperty(p);
            }
        }

        // Ambient Occlusion
        if (auto meta = interface_pointer_cast<META_NS::IMetadata>(GetTextureInfo(CORE3D_NS::MaterialComponent::AO))) {
            if (auto prop = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_STRENGTH)) {
                if (!includeDefaultMappings) {
                    meta->RemoveProperty(prop);
                }
            } else {
                auto p = META_NS::ConstructProperty<float>(SCENE_NS::IMaterial::MAPPED_INPUTS_STRENGTH);
                meta->AddProperty(p);
            }
        }

        // Clear Coat
        if (auto meta =
                interface_pointer_cast<META_NS::IMetadata>(GetTextureInfo(CORE3D_NS::MaterialComponent::CLEARCOAT))) {
            if (auto prop = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_INTENSITY)) {
                if (!includeDefaultMappings) {
                    meta->RemoveProperty(prop);
                }
            } else {
                auto p = META_NS::ConstructProperty<float>(SCENE_NS::IMaterial::MAPPED_INPUTS_INTENSITY);
                meta->AddProperty(p);
            }
        }

        // Clear Coat Roughness
        if (auto meta = interface_pointer_cast<META_NS::IMetadata>(
                GetTextureInfo(CORE3D_NS::MaterialComponent::CLEARCOAT_ROUGHNESS))) {
            if (auto prop = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_ROUGHNESS)) {
                if (!includeDefaultMappings) {
                    meta->RemoveProperty(prop);
                }
            } else {
                auto p = META_NS::ConstructProperty<float>(SCENE_NS::IMaterial::MAPPED_INPUTS_ROUGHNESS);
                meta->AddProperty(p);
            }
        }

        // Clear Coat Normal
        if (auto meta = interface_pointer_cast<META_NS::IMetadata>(
                GetTextureInfo(CORE3D_NS::MaterialComponent::CLEARCOAT_NORMAL))) {
            if (auto prop = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_NORMAL_SCALE)) {
                if (!includeDefaultMappings) {
                    meta->RemoveProperty(prop);
                }
            } else {
                auto p = META_NS::ConstructProperty<float>(SCENE_NS::IMaterial::MAPPED_INPUTS_NORMAL_SCALE);
                meta->AddProperty(p);
            }
        }

        // Sheen
        if (auto meta =
                interface_pointer_cast<META_NS::IMetadata>(GetTextureInfo(CORE3D_NS::MaterialComponent::SHEEN))) {
            if (auto prop = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_COLOR)) {
                if (!includeDefaultMappings) {
                    meta->RemoveProperty(prop);
                }
            } else {
                auto p = META_NS::ConstructProperty<SCENE_NS::Color>(SCENE_NS::IMaterial::MAPPED_INPUTS_COLOR);
                meta->AddProperty(p);
            }
            if (auto prop = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_ROUGHNESS)) {
                if (!includeDefaultMappings) {
                    meta->RemoveProperty(prop);
                }
            } else {
                auto p = META_NS::ConstructProperty<float>(SCENE_NS::IMaterial::MAPPED_INPUTS_ROUGHNESS);
                meta->AddProperty(p);
            }
        }

        // TRANSMISSION
        if (auto meta = interface_pointer_cast<META_NS::IMetadata>(
                GetTextureInfo(CORE3D_NS::MaterialComponent::TRANSMISSION))) {
            if (auto prop = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_TRANSMISSION)) {
                if (!includeDefaultMappings) {
                    meta->RemoveProperty(prop);
                }
            } else {
                auto p = META_NS::ConstructProperty<float>(SCENE_NS::IMaterial::MAPPED_INPUTS_TRANSMISSION);
                meta->AddProperty(p);
            }
        }

        // Specular
        if (auto meta =
                interface_pointer_cast<META_NS::IMetadata>(GetTextureInfo(CORE3D_NS::MaterialComponent::SPECULAR))) {
            if (auto color = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_COLOR)) {
                if (!includeDefaultMappings) {
                    meta->RemoveProperty(color);
                }
            } else {
                auto c = META_NS::ConstructProperty<SCENE_NS::Color>(SCENE_NS::IMaterial::MAPPED_INPUTS_COLOR);
                meta->AddProperty(c);
            }
            if (auto prop = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_STRENGTH)) {
                if (!includeDefaultMappings) {
                    meta->RemoveProperty(prop);
                }
            } else {
                auto p = META_NS::ConstructProperty<float>(SCENE_NS::IMaterial::MAPPED_INPUTS_STRENGTH);
                meta->AddProperty(p);
            }
        }
    }

    void BindInputProperties()
    {
        auto type = META_NS::GetValue(META_ACCESS_PROPERTY(Type));

        // Base Color
        if (auto meta =
                interface_pointer_cast<META_NS::IMetadata>(GetTextureInfo(CORE3D_NS::MaterialComponent::BASE_COLOR))) {
            if (auto color = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_COLOR)) {
                ConvertBindChanges<SCENE_NS::Color, BASE_NS::Math::Vec4>(GET_HOLDER(meta),
                    META_NS::Property<SCENE_NS::Color>(color), meta, SCENE_NS::IMaterial::MAPPED_INPUTS_FACTOR);
            }
        }

        // Normal
        if (auto meta =
                interface_pointer_cast<META_NS::IMetadata>(GetTextureInfo(CORE3D_NS::MaterialComponent::NORMAL))) {
            if (auto scale = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_NORMAL_SCALE)) {
                BindChanges<BASE_NS::Math::Vec4, float>(GET_HOLDER(meta), META_NS::Property<float>(scale), meta,
                    SCENE_NS::IMaterial::MAPPED_INPUTS_FACTOR, 0);
            }
        }

        //  Material
        if (auto meta =
                interface_pointer_cast<META_NS::IMetadata>(GetTextureInfo(CORE3D_NS::MaterialComponent::MATERIAL))) {
            // SCENE_NS::IMaterial::METALLIC_ROUGHNESS
            if (type == SCENE_NS::IMaterial::METALLIC_ROUGHNESS) {
                if (auto roughness = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_ROUGHNESS)) {
                    BindChanges<BASE_NS::Math::Vec4, float>(GET_HOLDER(meta), META_NS::Property<float>(roughness),
                        meta, SCENE_NS::IMaterial::MAPPED_INPUTS_FACTOR, 1);
                }
                if (auto metallic = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_METALLIC)) {
                    BindChanges<BASE_NS::Math::Vec4, float>(GET_HOLDER(meta), META_NS::Property<float>(metallic),
                        meta, SCENE_NS::IMaterial::MAPPED_INPUTS_FACTOR, 2); // 2 factor
                }
                if (auto reflectance = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_REFLECTANCE)) {
                    BindChanges<BASE_NS::Math::Vec4, float>(GET_HOLDER(meta), META_NS::Property<float>(reflectance),
                        meta, SCENE_NS::IMaterial::MAPPED_INPUTS_FACTOR, 3); // 3 factor
                }
            }

            // SCENE_NS::IMaterial::SPECULAR_GLOSSINESS
            if (type == SCENE_NS::IMaterial::SPECULAR_GLOSSINESS) {
                if (auto colorRGB = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_COLOR)) {
                    BASE_NS::vector<size_t> slots = { 0, 0, 1, 1, 2, 2 };
                    BindSlottedChanges<BASE_NS::Math::Vec4, SCENE_NS::Color>(GET_HOLDER(meta),
                        META_NS::Property<SCENE_NS::Color>(colorRGB), meta, SCENE_NS::IMaterial::MAPPED_INPUTS_FACTOR,
                        slots);
                }
                if (auto glossiness = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_GLOSSINESS)) {
                    BindChanges<BASE_NS::Math::Vec4, float>(GET_HOLDER(meta), META_NS::Property<float>(glossiness),
                        meta, SCENE_NS::IMaterial::MAPPED_INPUTS_FACTOR, 3);
                }
            }
        }

        // Emissive
        if (auto meta =
                interface_pointer_cast<META_NS::IMetadata>(GetTextureInfo(CORE3D_NS::MaterialComponent::EMISSIVE))) {
            if (auto color = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_COLOR)) {
                BASE_NS::vector<size_t> slots = { 0, 0, 1, 1, 2, 2 };
                BindSlottedChanges<BASE_NS::Math::Vec4, SCENE_NS::Color>(GET_HOLDER(meta),
                    META_NS::Property<SCENE_NS::Color>(color), meta, SCENE_NS::IMaterial::MAPPED_INPUTS_FACTOR,
                    slots);
            }
            if (auto prop = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_INTENSITY)) {
                BindChanges<BASE_NS::Math::Vec4, float>(GET_HOLDER(meta), META_NS::Property<float>(prop), meta,
                    SCENE_NS::IMaterial::MAPPED_INPUTS_FACTOR, 3);
            }
        }

        // Ambient Occlusion
        if (auto meta = interface_pointer_cast<META_NS::IMetadata>(GetTextureInfo(CORE3D_NS::MaterialComponent::AO))) {
            if (auto prop = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_STRENGTH)) {
                BindChanges<BASE_NS::Math::Vec4, float>(GET_HOLDER(meta), META_NS::Property<float>(prop), meta,
                    SCENE_NS::IMaterial::MAPPED_INPUTS_FACTOR, 0);
            }
        }

        // Clear Coat
        if (auto meta =
                interface_pointer_cast<META_NS::IMetadata>(GetTextureInfo(CORE3D_NS::MaterialComponent::CLEARCOAT))) {
            if (auto prop = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_INTENSITY)) {
                BindChanges<BASE_NS::Math::Vec4, float>(GET_HOLDER(meta), META_NS::Property<float>(prop), meta,
                    SCENE_NS::IMaterial::MAPPED_INPUTS_FACTOR, 0);
            }
        }

        // Clear Coat Roughness
        if (auto meta = interface_pointer_cast<META_NS::IMetadata>(
                GetTextureInfo(CORE3D_NS::MaterialComponent::CLEARCOAT_ROUGHNESS))) {
            if (auto prop = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_ROUGHNESS)) {
                BindChanges<BASE_NS::Math::Vec4, float>(GET_HOLDER(meta), META_NS::Property<float>(prop), meta,
                    SCENE_NS::IMaterial::MAPPED_INPUTS_FACTOR, 1);
            }
        }

        // Clear Coat Normal
        if (auto meta = interface_pointer_cast<META_NS::IMetadata>(
                GetTextureInfo(CORE3D_NS::MaterialComponent::CLEARCOAT_NORMAL))) {
            if (auto prop = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_NORMAL_SCALE)) {
                BindChanges<BASE_NS::Math::Vec4, float>(GET_HOLDER(meta), META_NS::Property<float>(prop), meta,
                    SCENE_NS::IMaterial::MAPPED_INPUTS_FACTOR, 0);
            }
        }

        // Sheen
        if (auto meta =
                interface_pointer_cast<META_NS::IMetadata>(GetTextureInfo(CORE3D_NS::MaterialComponent::SHEEN))) {
            if (auto prop = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_COLOR)) {
                BASE_NS::vector<size_t> slots = { 0, 0, 1, 1, 2, 2 };
                BindSlottedChanges<BASE_NS::Math::Vec4, SCENE_NS::Color>(GET_HOLDER(meta),
                    META_NS::Property<SCENE_NS::Color>(prop), meta, SCENE_NS::IMaterial::MAPPED_INPUTS_FACTOR, slots);
            }
            if (auto prop = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_ROUGHNESS)) {
                BindChanges<BASE_NS::Math::Vec4, float>(GET_HOLDER(meta), META_NS::Property<float>(prop), meta,
                    SCENE_NS::IMaterial::MAPPED_INPUTS_FACTOR, 3);
            }
        }

        // TRANSMISSION
        if (auto meta = interface_pointer_cast<META_NS::IMetadata>(
                GetTextureInfo(CORE3D_NS::MaterialComponent::TRANSMISSION))) {
            if (auto prop = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_TRANSMISSION)) {
                BindChanges<BASE_NS::Math::Vec4, float>(GET_HOLDER(meta), META_NS::Property<float>(prop), meta,
                    SCENE_NS::IMaterial::MAPPED_INPUTS_FACTOR, 0);
            }
        }

        // Specular
        if (auto meta =
                interface_pointer_cast<META_NS::IMetadata>(GetTextureInfo(CORE3D_NS::MaterialComponent::SPECULAR))) {
            if (auto color = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_COLOR)) {
                BASE_NS::vector<size_t> slots = { 0, 0, 1, 1, 2, 2 };
                BindSlottedChanges<BASE_NS::Math::Vec4, SCENE_NS::Color>(GET_HOLDER(meta),
                    META_NS::Property<SCENE_NS::Color>(color), meta, SCENE_NS::IMaterial::MAPPED_INPUTS_FACTOR,
                    slots);
            }
            if (auto prop = meta->GetPropertyByName(SCENE_NS::IMaterial::MAPPED_INPUTS_STRENGTH)) {
                BindChanges<BASE_NS::Math::Vec4, float>(GET_HOLDER(meta), META_NS::Property<float>(prop), meta,
                    SCENE_NS::IMaterial::MAPPED_INPUTS_FACTOR, 3); // 3 factor
            }
        }
    }

    void OnTypeChanged()
    {
        UpdateInputProperties();
        BindInputProperties();
    }

    // We intend to define the object only on demand
    // If we have a previous object, we could try to preserve it
    void UpdateCustomProperties()
    {
        // Store and remove old custom properties.
        BASE_NS::vector<META_NS::IProperty::Ptr> oldCustomProperties;

        auto properties = interface_pointer_cast<META_NS::IMetadata>(META_ACCESS_PROPERTY(CustomProperties)->GetValue());
        if (properties) {
            oldCustomProperties = properties->GetAllProperties();
            properties->GetPropertyContainer()->RemoveAll();
        }

        // Find new custom properties.
        BASE_NS::vector<META_NS::IProperty::Ptr> newCustomProperties;

        auto meta = interface_pointer_cast<META_NS::IMetadata>(ecsObject_);
        if (meta) {
            // Collect all custom properties from ECS object.
            auto metaProps = meta->GetAllProperties();
            for (auto&& prop : metaProps) {
                if (prop->GetName().compare(0, CUSTOM_PREFIX_SIZE, CUSTOM_PREFIX) == 0) {
                    newCustomProperties.push_back(prop);
                }
            }
        }

        // If there are no custom properties, then reset.
        if (newCustomProperties.empty()) {
            META_ACCESS_PROPERTY(CustomProperties)->Reset();
            return;
        }

        // Otherwise ensure that we have container for the custom properties.
        if (!properties) {
            properties = GetObjectRegistry().Create<META_NS::IMetadata>(SCENE_NS::ClassId::CustomPropertiesHolder);
            META_ACCESS_PROPERTY(CustomProperties)->SetValue(interface_pointer_cast<META_NS::IObject>(properties));
        }

        GET_HOLDER(properties).SetSceneHolder(sceneHolder_.lock());

        // Then create and bind all the custom properties.
        for (auto& prop : newCustomProperties) {
            // This is the new custom property.
            META_NS::IProperty::Ptr property;

            BASE_NS::string propertyName(prop->GetName().substr(CUSTOM_PREFIX_SIZE));

            // Try to re-use the old custom property if we have one, it may have a meaningful value set.
            for (auto& existing : oldCustomProperties) {
                if (existing->GetName() == propertyName && existing->GetTypeId() == prop->GetTypeId()) {
                    property = existing;
                    break;
                }
            }

            if (!property) {
                // Clone property.
                property = META_NS::DuplicatePropertyType(META_NS::GetObjectRegistry(), prop, propertyName);
            }

            // Add it to target.
            properties->AddProperty(property);

            // Bind it to ecs property.
            BindMetaProperty(GET_HOLDER(properties), property, prop);
        }
    }

    void UpdateInputs(bool forceRecursive = false, bool forceFullReset = true)
    {
        if (updatingInputs_ && !forceRecursive) {
            return;
        }

        if (forceFullReset) {
            updatingInputs_ = true;

            propHandler_.Reset();
            META_ACCESS_PROPERTY(Inputs)->Reset();
            META_ACCESS_PROPERTY(CustomProperties)->Reset();

            auto entity = EcsObject()->GetEntity();
            auto ecs = EcsObject()->GetEcs();

            // Reset the object
            EcsObject()->BindObject(nullptr, entity);

            auto scene = EcsScene();
            auto ecsObject = EcsObject();

            Initialize(
                scene, ecsObject, {}, META_NS::GetValue(Path()), META_NS::GetValue(Name()), sceneHolder_, entity);

            updatingInputs_ = false;
        } else {
            // This will potentially go wrong if the proxy properties contain set values
            // The values previously set will replace the values from engine
            if (UpdateAllInputProperties()) {
                updatingInputs_ = true;
                META_ACCESS_PROPERTY(Inputs)->Reset();
                META_ACCESS_PROPERTY(CustomProperties)->Reset();
                CompleteInitialization(META_NS::GetValue(Name()));
                updatingInputs_ = false;
            }
        }
    }

    static constexpr BASE_NS::string_view MATERIAL_SHADER_NAME { "MaterialComponent.materialShader.shader" };
    static constexpr BASE_NS::string_view DEPTH_SHADER_NAME { "MaterialComponent.depthShader.shader" };

    SCENE_NS::ITextureInfo::Ptr FindTextureInfo(BASE_NS::string_view name)
    {
        if (META_ACCESS_PROPERTY(Inputs)) {
            for (auto& info : Inputs()->GetValue()) {
                if (META_NS::GetValue(info->Name()) == name) {
                    return info;
                }
            }
        }
        return {};
    }

    void CopyTextureInfoProperties(SCENE_NS::ITextureInfo::Ptr from, SCENE_NS::ITextureInfo::Ptr to)
    {
        if (from->Factor()->IsValueSet()) {
            to->Factor()->SetValue(from->Factor()->GetValue());
        }

        if (from->Rotation()->IsValueSet()) {
            to->Rotation()->SetValue(from->Rotation()->GetValue());
        }

        if (from->Scale()->IsValueSet()) {
            to->Scale()->SetValue(from->Scale()->GetValue());
        }

        if (from->Translation()->IsValueSet()) {
            to->Translation()->SetValue(from->Translation()->GetValue());
        }

        // Sampler enumeration works now using enums, it should follow uri
        // but uri information is not available from engine yet
        if (from->Sampler()->IsValueSet()) {
            to->Sampler()->SetValue(from->Sampler()->GetValue());
        }

        // image goes through uri implementation
        if (auto data = META_NS::GetValue(from->Image())) {
            to->Image()->SetValue(from->Image()->GetValue());
        }
    }

    bool SynchronizeInputsFromMetadata()
    {
        auto meta = interface_pointer_cast<META_NS::IMetadata>(ecsObject_);
        if (!meta) {
            return false;
        }
        // Texture slots need to be bound before other properties
	BASE_NS::vector<SCENE_NS::ITextureInfo::Ptr> textures;
	auto prop = 
            meta->GetArrayPropertyByName<CORE3D_NS::MaterialComponent::TextureInfo>("MaterialComponent.textures"); 
	// slot names (unsure-what-they-were-before-but-matches-core3d-enum)
	const char* TextureIndexName[] = { "BASE_COLOR", "NORMAL", "MATERIAL", "EMISSIVE", "AO", "CLEARCOAT",
	    "CLEARCOAT_ROUGHNESS", "CLEARCOAT_NORMAL", "SHEEN", "TRANSMISSION", "SPECULAR" };
	auto v = prop->GetValue();
	auto shp = SceneHolder();	
	for (auto t : v) {
	    int textureSlotIndex = textures.size();
	    // create wrapping things..
	    auto info = GetTextureInfoByIndex(textureSlotIndex, textures);
	    if (!info) {
	        auto& obr = GetObjectRegistry();
		auto params = obr.ConstructMetadata();
		using IntfPtr = META_NS::SharedPtrIInterface;
		params->AddProperty(META_NS::ConstructProperty<META_NS::IProperty::Ptr>(
		    "textureInfoArray", nullptr, META_NS::ObjectFlagBits::INTERNAL | META_NS::ObjectFlagBits::NATIVE));
		params->AddProperty(META_NS::ConstructProperty<uint32_t>(
		    "textureSlotIndex", 0, META_NS::ObjectFlagBits::INTERNAL | META_NS::ObjectFlagBits::NATIVE));
		params->AddProperty(META_NS::ConstructProperty<uintptr_t>(
		    "sceneHolder", 0, META_NS::ObjectFlagBits::INTERNAL | META_NS::ObjectFlagBits::NATIVE));
		
		// yes this is ugly.
		params->GetPropertyByName<uintptr_t>("sceneHolder")->SetValue((uintptr_t)&shp);
		params->GetPropertyByName<IntfPtr>("textureInfoArray")
		    ->SetValue(interface_pointer_cast<CORE_NS::IInterface>(prop.GetProperty()));
		params->GetPropertyByName<uint32_t>("textureSlotIndex")->SetValue(textureSlotIndex);
		info = obr.Create<SCENE_NS::ITextureInfo>(SCENE_NS::ClassId::TextureInfo, params);
		if (!info) {
		    return {};
		}

		info->SetTextureSlotIndex(textureSlotIndex);
		textures.push_back(info);
	    } else {
	        textures.push_back(nullptr);
	    }
	}

        BASE_NS::vector<SCENE_NS::ITextureInfo::Ptr> prevInputs;
        if (META_ACCESS_PROPERTY(Inputs)) {
            auto size = META_ACCESS_PROPERTY(Inputs)->GetSize();
            for (size_t ii = 0; ii < size; ii++) {
                prevInputs.push_back(META_ACCESS_PROPERTY(Inputs)->GetValueAt(ii));
            }
        }

        META_ACCESS_PROPERTY(Inputs)->Reset();

        // Sort texture infos.
        std::sort(textures.begin(), textures.end(), [](const auto& a, const auto& b) {
            // Sort based on texture-slot index.
            return a->GetTextureSlotIndex() < b->GetTextureSlotIndex();
        });

        // Assign to property.
        Inputs()->SetValue(textures);

        // Copy values from old inputs to new ones.
        for (auto& from : prevInputs) {
            if (auto to = FindTextureInfo(META_NS::GetValue(from->Name()))) {
                CopyTextureInfoProperties(from, to);
            }
        }

        return true;
    }

    // return true if something changes
    bool UpdateAllInputProperties()
    {
        auto meta = interface_pointer_cast<META_NS::IMetadata>(EcsObject());
        if (!meta) {
            return false;
        }

        auto oldCount = meta->GetPropertyContainer()->GetSize();

        // This updates all properties from ecs and detaches properties that do not exist any more.
        EcsObject()->DefineTargetProperties(PropertyNameMask());

        auto newCount = meta->GetPropertyContainer()->GetSize();

        auto allProperties = meta->GetAllProperties();

        // if we add or remove something these all cannot match
        return !((newCount == oldCount) && (oldCount != meta->GetPropertyContainer()->GetSize()));
    }

    bool CompleteInitialization(const BASE_NS::string& path) override
    {
        if (!NodeImpl::CompleteInitialization(path)) {
            return false;
        }

        auto meta = interface_pointer_cast<META_NS::IMetadata>(ecsObject_);
        if (!meta) {
            return false;
        }


        if (DepthShader()->GetValue()) {
            SceneHolder()->SetShader(
                EcsObject()->GetEntity(), SceneHolder::ShaderType::DEPTH_SHADER, DepthShader()->GetValue());
        } else {
            // Try to introspect depth shader from material.
            auto shader = SceneHolder()->GetShader(EcsObject()->GetEntity(), SceneHolder::ShaderType::DEPTH_SHADER);
            if (shader) {
                DepthShader()->SetValue(shader);
            }
        }

        if (DepthShaderState()->GetValue()) {
            SceneHolder()->SetGraphicsState(
                EcsObject()->GetEntity(), SceneHolder::ShaderType::DEPTH_SHADER, DepthShaderState()->GetValue());
        } else {
            // Try to introspect depth shader from material.
            auto state =
                SceneHolder()->GetGraphicsState(EcsObject()->GetEntity(), SceneHolder::ShaderType::DEPTH_SHADER);
            if (state) {
                DepthShaderState()->SetValue(state);
            }
        }

        if (MaterialShader()->GetValue()) {
            SceneHolder()->SetShader(
                EcsObject()->GetEntity(), SceneHolder::ShaderType::MATERIAL_SHADER, MaterialShader()->GetValue());
        } else {
            // Try to introspect material shader from material.
            auto shader = SceneHolder()->GetShader(EcsObject()->GetEntity(), SceneHolder::ShaderType::MATERIAL_SHADER);
            if (shader) {
                MaterialShader()->SetValue(shader);
            }
        }

        if (MaterialShaderState()->GetValue()) {
            SceneHolder()->SetGraphicsState(
                EcsObject()->GetEntity(), SceneHolder::ShaderType::MATERIAL_SHADER, MaterialShaderState()->GetValue());
        } else {
            // Try to introspect depth shader from material.
            auto state =
                SceneHolder()->GetGraphicsState(EcsObject()->GetEntity(), SceneHolder::ShaderType::MATERIAL_SHADER);
            if (state) {
                MaterialShaderState()->SetValue(state);
            }
        }

        // Shader may have changed, so update all properties.
        UpdateAllInputProperties();

        // Properties up-to-date, synchronize all inputs.
        SynchronizeInputsFromMetadata();

        propHandler_.NewHandler(nullptr, nullptr).Subscribe(META_ACCESS_PROPERTY(Type), [this]() { OnTypeChanged(); });

        BindChanges(propHandler_, META_ACCESS_PROPERTY(Type), meta, "MaterialComponent.type");

        // Shader will either come as an entity ref from the engine, or from serialized info. have a listener
        // that attaches the new ecs object to entity if one appears
        propHandler_.NewHandler(nullptr, nullptr).Subscribe(DepthShader(), [this]() {
            SceneHolder()->SetShader(
                EcsObject()->GetEntity(), SceneHolder::ShaderType::DEPTH_SHADER, DepthShader()->GetValue());
            UpdateInputs();
        });

        propHandler_.NewHandler(nullptr, nullptr).Subscribe(MaterialShader(), [this]() {
            // Material shader has changed.
            SceneHolder()->SetShader(
                EcsObject()->GetEntity(), SceneHolder::ShaderType::MATERIAL_SHADER, MaterialShader()->GetValue());
            UpdateInputs();
        });

        propHandler_.NewHandler(nullptr, nullptr).Subscribe(DepthShaderState(), [this]() {
            SceneHolder()->SetGraphicsState(
                EcsObject()->GetEntity(), SceneHolder::ShaderType::DEPTH_SHADER, DepthShaderState()->GetValue());
        });

        propHandler_.NewHandler(nullptr, nullptr).Subscribe(MaterialShaderState(), [this]() {
            // Material shader has changed.
            SceneHolder()->SetGraphicsState(
                EcsObject()->GetEntity(), SceneHolder::ShaderType::MATERIAL_SHADER, MaterialShaderState()->GetValue());
        });

        propHandler_.MarkRelated(MaterialShader(), meta->GetPropertyByName("MaterialComponent.materialShader.shader"));
        propHandler_.MarkRelated(DepthShader(), meta->GetPropertyByName("MaterialComponent.depthShader.shader"));
        propHandler_.MarkRelated(
            MaterialShaderState(), meta->GetPropertyByName("MaterialComponent.materialShader.graphicsState"));
        propHandler_.MarkRelated(
            DepthShaderState(), meta->GetPropertyByName("MaterialComponent.depthShader.graphicsState"));

        // make sure that inputs are up to date
        OnTypeChanged();

        // Update custom properties.
        UpdateCustomProperties();

        BindChanges(propHandler_, META_ACCESS_PROPERTY(AlphaCutoff), meta, "MaterialComponent.alphaCutoff");
        BindChanges(propHandler_, META_ACCESS_PROPERTY(LightingFlags), meta, "MaterialComponent.materialLightingFlags");
        return true;
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

    void SetImage(SCENE_NS::IBitmap::Ptr bitmap, BASE_NS::string_view textureSlot) override
    {
        auto size = Inputs()->GetSize();
        for (size_t ii = 0; ii < size; ii++) {
            if (META_NS::GetValue(Inputs()->GetValueAt(ii)->Name()).compare(textureSlot) == 0) {
                SetImage(bitmap, ii);
                break;
            }
        }
    }

    void SetImage(SCENE_NS::IBitmap::Ptr bitmap, size_t index) override
    {
        if (bitmap) {
            auto status = META_NS::GetValue(bitmap->Status());
            if (status == SCENE_NS::IBitmap::BitmapStatus::COMPLETED) {
                if (auto sceneHolder = SceneHolder()) {
                    sceneHolder->QueueEngineTask(
                        MakeTask(
                            [bitmap, index, weakSelf = BASE_NS::weak_ptr(GetSelf())](auto sh) {
                                CORE_NS::Entity imageEntity = sh->BindUIBitmap(bitmap, true);
                                auto image = sh->GetEcs()->GetEntityManager().GetReferenceCounted(imageEntity);
                                sh->SetTexture(index,
                                    interface_pointer_cast<INodeEcsInterfacePrivate>(weakSelf)
                                        ->EcsObject()
                                        ->GetEntity(),
                                    image);
                                sh->QueueApplicationTask(MakeTask([bitmap, index, weakSelf]() {
                                    if (auto me = interface_pointer_cast<SCENE_NS::IMaterial>(weakSelf)) {
                                        if (auto input = me->Inputs()->GetValueAt(index)) {
                                            input->Image()->SetValue(bitmap);
                                        }
                                    }
                                    return false;
                                }),
                                    false);

                                return false;
                            },
                            sceneHolder),
                        false);
                }

            } else {
                // should basically subscribe to dynamic content instead
                // Give uri based loading a shot
                if (auto input = Inputs()->GetValueAt(index)) {
                    input->Image()->SetValue(bitmap);
                }
            }
        } else { // reset existing image if there is one
            if (auto sceneHolder = SceneHolder()) {
                sceneHolder->QueueEngineTask(MakeTask(
                                                 [entityId = EcsObject()->GetEntity().id](auto sh) {
                                                     CORE_NS::Entity target { entityId };
                                                     // The assumption is that using base color ix is correct thing to
                                                     // do
                                                     sh->SetTexture(
                                                         CORE3D_NS::MaterialComponent::BASE_COLOR, target, {});

                                                     return false;
                                                 },
                                                 sceneHolder),
                    false);
            }
        }
    }
};
} // namespace
SCENE_BEGIN_NAMESPACE()

void RegisterMaterialImpl()
{
    META_NS::GetObjectRegistry().RegisterObjectType<MaterialImpl>();
}
void UnregisterMaterialImpl()
{
    META_NS::GetObjectRegistry().UnregisterObjectType<MaterialImpl>();
}

SCENE_END_NAMESPACE()
