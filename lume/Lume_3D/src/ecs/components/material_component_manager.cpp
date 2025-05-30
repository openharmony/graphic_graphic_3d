/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <base/containers/refcnt_ptr.h>
#include <base/containers/unordered_map.h>
#include <base/util/compile_time_hashes.h>
#include <base/util/errors.h>
#include <base/util/hash.h>
#include <core/ecs/intf_ecs.h>
#include <core/intf_engine.h>
#include <core/property/intf_property_api.h>
#include <core/property/property_types.h>
#include <render/device/intf_shader_manager.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>

#include "util/property_util.h"

#define IMPLEMENT_MANAGER
#include <algorithm>
#include <iterator>

#include <core/property_tools/property_macros.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::string_view;
using BASE_NS::vector;
using CORE3D_NS::MaterialComponent;

/** Extend propertysystem with the types */
DECLARE_PROPERTY_TYPE(MaterialComponent::Type);
DECLARE_PROPERTY_TYPE(MaterialComponent::TextureTransform);
DECLARE_PROPERTY_TYPE(MaterialComponent::TextureInfo);
DECLARE_PROPERTY_TYPE(MaterialComponent::Shader);
DECLARE_PROPERTY_TYPE(MaterialComponent::RenderSort);
DECLARE_PROPERTY_TYPE(vector<MaterialComponent::TextureTransform>);
DECLARE_PROPERTY_TYPE(CORE_NS::IPropertyHandle*);

// Declare their metadata
ENUM_TYPE_METADATA(MaterialComponent::Type, ENUM_VALUE(METALLIC_ROUGHNESS, "Metallic Roughness"),
    ENUM_VALUE(SPECULAR_GLOSSINESS, "Specular Glossiness"), ENUM_VALUE(UNLIT, "Unlit"),
    ENUM_VALUE(UNLIT_SHADOW_ALPHA, "Unlit Shadow Alpha"), ENUM_VALUE(CUSTOM, "Custom material"),
    ENUM_VALUE(CUSTOM_COMPLEX, "Custom complex material"))

ENUM_TYPE_METADATA(MaterialComponent::LightingFlagBits, ENUM_VALUE(SHADOW_RECEIVER_BIT, "Shadow Receiver"),
    ENUM_VALUE(SHADOW_CASTER_BIT, "Shadow Caster"), ENUM_VALUE(PUNCTUAL_LIGHT_RECEIVER_BIT, "Punctual Light Receiver"),
    ENUM_VALUE(INDIRECT_LIGHT_RECEIVER_BIT, "Indirect Light Receiver"))

ENUM_TYPE_METADATA(MaterialComponent::ExtraRenderingFlagBits, ENUM_VALUE(DISCARD_BIT, "Discard Special Materials"),
    ENUM_VALUE(DISABLE_BIT, "Disable Material Rendering"), ENUM_VALUE(ALLOW_GPU_INSTANCING_BIT, "Allow GPU Instancing"))

DATA_TYPE_METADATA(MaterialComponent::TextureTransform, MEMBER_PROPERTY(translation, "Translation", 0),
    MEMBER_PROPERTY(rotation, "Rotation", 0), MEMBER_PROPERTY(scale, "Scale", 0))

DATA_TYPE_METADATA(MaterialComponent::TextureInfo, MEMBER_PROPERTY(image, "Texture", 0),
    MEMBER_PROPERTY(sampler, "Sampler", 0), MEMBER_PROPERTY(factor, "Factor", 0),
    MEMBER_PROPERTY(transform, "Transform", 0))

DATA_TYPE_METADATA(MaterialComponent::Shader, MEMBER_PROPERTY(shader, "Shader", 0),
    MEMBER_PROPERTY(graphicsState, "Graphics State", 0))

DATA_TYPE_METADATA(MaterialComponent::RenderSort, MEMBER_PROPERTY(renderSortLayer, "Render Sort Layer", 0),
    MEMBER_PROPERTY(renderSortLayerOrder, "Render Sort Layer Order (fine ordering)", 0))

constexpr PropertyTypeDecl TEXTURE_INFO_T = PROPERTYTYPE(MaterialComponent::TextureInfo);

static constexpr inline bool operator==(const Property& lhs, const Property& rhs) noexcept
{
    return (lhs.type == rhs.type) && (lhs.hash == rhs.hash) && (lhs.name == rhs.name);
}
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;
using BASE_NS::exchange;
using BASE_NS::forward;
using BASE_NS::move;
using BASE_NS::refcnt_ptr;
using BASE_NS::string;
using BASE_NS::string_view;
using BASE_NS::Uid;
using BASE_NS::unordered_map;
using BASE_NS::vector;

using CORE_NS::Entity;
using CORE_NS::GetInstance;
using CORE_NS::GetManager;
using CORE_NS::IClassRegister;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::IPropertyApi;
using CORE_NS::IPropertyHandle;
using CORE_NS::Property;
using CORE_NS::PropertyFlags;
using CORE_NS::ScopedHandle;

using RENDER_NS::IRenderContext;
using RENDER_NS::IShaderManager;
using RENDER_NS::RenderHandle;
using RENDER_NS::RenderHandleReference;
using RENDER_NS::UID_RENDER_CONTEXT;

namespace {
constexpr string_view PROPERTIES = "properties";
constexpr string_view NAME = "name";
constexpr string_view DISPLAY_NAME = "displayName";
constexpr string_view MATERIAL_COMPONENT_NAME = CORE_NS::GetName<MaterialComponent>();

// RenderDataDefaultMaterial::MAX_MATERIAL_CUSTOM_PROPERTY_BYTE_SIZE
constexpr uint32_t CUSTOM_PROPERTY_POD_CONTAINER_BYTE_SIZE { 256u };
constexpr string_view CUSTOM_PROPERTIES = "customProperties";
constexpr string_view CUSTOM_BINDING_PROPERTIES = "customBindingProperties";
constexpr string_view CUSTOM_PROPERTY_DATA = "data";

constexpr uint32_t MODIFIED = 0x80000000;

template<typename Container, typename Predicate>
inline typename Container::const_iterator FindIf(const Container& container, Predicate&& predicate)
{
    return std::find_if(container.cbegin(), container.cend(), forward<Predicate>(predicate));
}

template<typename Container, typename Predicate>
inline typename Container::iterator FindIf(Container& container, Predicate&& predicate)
{
    return std::find_if(container.begin(), container.end(), forward<Predicate>(predicate));
}

const CORE_NS::json::value* FindMaterialComponentJson(const CORE_NS::json::value& metaJson)
{
    if (auto material = FindIf(metaJson.array_,
            [](const CORE_NS::json::value& value) {
                auto nameJson = value.find(NAME);
                return nameJson && nameJson->is_string() && nameJson->string_ == MATERIAL_COMPONENT_NAME;
            });
        material != metaJson.array_.end()) {
        return &(*material);
    }
    return nullptr;
}

void HandlePropertyObject(vector<Property>& newProperties, vector<string>& newStringPropStorage,
    const Property& property, const CORE_NS::json::value& value, const uint32_t index)
{
    const auto& containerProperty = property.metaData.containerMethods->property;

    // calculate offset to current struct in the array
    const auto offset = property.offset + containerProperty.size * index;
    Property newProperty = containerProperty;

    // add the currect struct offset to the member offset
    newProperty.offset += offset;

    // update 'name' and 'displayName' if needed
    if (auto name = value.find(NAME); name && name->is_string()) {
        newStringPropStorage.push_back(string(name->string_));
        newProperty.name = newStringPropStorage.back();
        newProperty.hash = BASE_NS::FNV1aHash(newProperty.name.data(), newProperty.name.size());
    }
    if (auto displayName = value.find(DISPLAY_NAME); displayName && displayName->is_string()) {
        newStringPropStorage.push_back(string(displayName->string_));
        newProperty.displayName = newStringPropStorage.back();
    }
    newProperties.push_back(newProperty);
}

void HandlePropertyArray(vector<Property>& newProperties, vector<string>& newStringPropStorage,
    const Property& property, const CORE_NS::json::value::array& propertyArray)
{
    uint32_t index = 0u;
    // reserve strings, name and display name (reason for * 2)
    newStringPropStorage.reserve(propertyArray.size() * 2);
    for (const CORE_NS::json::value& value : propertyArray) {
        // if the array has objects, the metadata should have containerMethods for
        // the members
        if (value.is_object() && !value.object_.empty() && property.metaData.containerMethods) {
            HandlePropertyObject(newProperties, newStringPropStorage, property, value, index);
        } else {
            // NOTE: handle non-object properties?
        }
        ++index;
    }
}

void AppendProperties(vector<Property>& newProperties, vector<string>& newStringPropStorage,
    const CORE_NS::json::value& material, const array_view<const Property> componentMetaData)
{
    const CORE_NS::json::value* propertiesJson = material.find(PROPERTIES);
    if (!propertiesJson || !propertiesJson->is_object()) {
        return;
    }
    for (const auto& propertyObject : propertiesJson->object_) {
        const auto propertyIt =
            FindIf(componentMetaData, [key = propertyObject.key](const Property& p) { return p.name == key; });
        if (propertyIt == componentMetaData.cend()) {
            continue;
        }
        // if it's an array, make sure sizes match
#if (CORE3D_VALIDATION_ENABLED == 1)
        if (propertyObject.value.is_array() && propertyIt->type.isArray &&
            propertyObject.value.array_.size() > propertyIt->count) {
            CORE_LOG_W("CORE3D_VALIDATION: material property metadata count mismatch (%u <= %u)",
                uint32_t(propertyObject.value.array_.size()), uint32_t(propertyIt->count));
        }
#endif
        if (propertyObject.value.is_array() && propertyIt->type.isArray &&
            propertyObject.value.array_.size() <= propertyIt->count) {
            HandlePropertyArray(newProperties, newStringPropStorage, *propertyIt, propertyObject.value.array_);
        } else {
            // NOTE: handle non-array properties?
        }
    }
}

void MapTextureSlots(array_view<MaterialComponent::TextureInfo> textures, size_t baseOffset,
    array_view<const Property> newProperties, array_view<const Property> oldProperties)
{
    MaterialComponent::TextureInfo tmp[MaterialComponent::TextureIndex::TEXTURE_COUNT];
    for (const auto& newProp : newProperties) {
        if (newProp.type == CORE_NS::TEXTURE_INFO_T) {
            if (auto it = std::find(oldProperties.cbegin(), oldProperties.cend(), newProp);
                it != oldProperties.cend()) {
                auto& oldProp = *it;
                // theoretically the offset of TextureInfos can't be less than the offset of the first and this could be
                // an assertion
                if (oldProp.offset >= baseOffset && newProp.offset >= baseOffset) {
                    auto oldIndex = (oldProp.offset - baseOffset) / sizeof(MaterialComponent::TextureInfo);
                    auto newIndex = (newProp.offset - baseOffset) / sizeof(MaterialComponent::TextureInfo);
                    if (newIndex < BASE_NS::countof(tmp) && oldIndex < textures.size()) {
                        tmp[newIndex] = BASE_NS::move(textures[oldIndex]);
                    }
                }
            }
        }
    }
    std::move(std::begin(tmp), std::end(tmp), std::begin(textures));
}

void UpdateCustomPropertyMetadata(const CORE_NS::json::value& customProperties, CustomPropertyPodContainer& properties)
{
    for (const CORE_NS::json::value& propertyValue : customProperties.array_) {
        if (propertyValue.is_object()) {
            // NOTE: set and binding are currently ignored as the custom material property cannot be mapped
            // to user defined sets and bindings
            for (const auto& object : propertyValue.object_) {
                if (object.key == CUSTOM_PROPERTY_DATA && object.value.is_array()) {
                    // reserve the property count
                    properties.ReservePropertyCount(object.value.array_.size());
                    for (const auto& dataValue : object.value.array_) {
                        if (dataValue.is_object()) {
                            string_view name;
                            string_view displayName;
                            string_view type;
                            const CORE_NS::json::value* value = nullptr;
                            for (const auto& dataObject : dataValue.object_) {
                                if (dataObject.key == NAME && dataObject.value.is_string()) {
                                    name = dataObject.value.string_;
                                } else if (dataObject.key == DISPLAY_NAME && dataObject.value.is_string()) {
                                    displayName = dataObject.value.string_;
                                } else if (dataObject.key == "type" && dataObject.value.is_string()) {
                                    type = dataObject.value.string_;
                                } else if (dataObject.key == "value") {
                                    value = &dataObject.value;
                                }
                            }
                            const CORE_NS::PropertyTypeDecl typeDecl =
                                CustomPropertyPodHelper::GetPropertyTypeDeclaration(type);
                            const size_t align = CustomPropertyPodHelper::GetPropertyTypeAlignment(typeDecl);
                            const size_t offset = [](size_t value, size_t align) -> size_t {
                                if (align == 0U) {
                                    return value;
                                }
                                return ((value + align - 1U) / align) * align;
                            }(properties.GetByteSize(), align);
                            properties.AddOffsetProperty(name, displayName, offset, typeDecl);
                            CustomPropertyPodHelper::SetCustomPropertyBlobValue(typeDecl, value, properties, offset);
                        }
                    }
                }
            }
        }
    }
}

void UpdateCustomBindingPropertyMetadata(
    const CORE_NS::json::value& customProperties, CustomPropertyBindingContainer& properties)
{
    for (const CORE_NS::json::value& propertyValue : customProperties.array_) {
        if (propertyValue.is_object()) {
            // NOTE: set and binding are currently ignored as the custom material property cannot be mapped
            // to user defined sets and bindings
            for (const auto& object : propertyValue.object_) {
                if (object.key == CUSTOM_PROPERTY_DATA && object.value.is_array()) {
                    // reserve the property count
                    properties.ReservePropertyCount(object.value.array_.size());
                    for (const auto& dataValue : object.value.array_) {
                        if (dataValue.is_object()) {
                            string_view name;
                            string_view displayName;
                            string_view type;
                            for (const auto& dataObject : dataValue.object_) {
                                if (dataObject.key == NAME && dataObject.value.is_string()) {
                                    name = dataObject.value.string_;
                                } else if (dataObject.key == DISPLAY_NAME && dataObject.value.is_string()) {
                                    displayName = dataObject.value.string_;
                                } else if (dataObject.key == "type" && dataObject.value.is_string()) {
                                    type = dataObject.value.string_;
                                }
                            }
                            const CORE_NS::PropertyTypeDecl typeDecl =
                                CustomPropertyBindingHelper::GetPropertyTypeDeclaration(type);
                            const size_t align = CustomPropertyBindingHelper::GetPropertyTypeAlignment(typeDecl);
                            const size_t offset = [](size_t value, size_t align) -> size_t {
                                if (align == 0U) {
                                    return value;
                                }
                                return ((value + align - 1U) / align) * align;
                            }(properties.GetByteSize(), align);
                            properties.AddOffsetProperty(name, displayName, offset, typeDecl);
                        }
                    }
                }
            }
        }
    }
}

BASE_NS::unique_ptr<CustomPropertyPodContainer> CreateCustomPropertyPodContainer(CustomPropertyWriteSignal& signal,
    const CORE_NS::json::value* const propertiesJson, const CustomPropertyPodContainer* oldContainer)
{
    // create and fill new POD container
    auto newPod = BASE_NS::make_unique<CustomPropertyPodContainer>(signal, CUSTOM_PROPERTY_POD_CONTAINER_BYTE_SIZE);
    if (propertiesJson) {
        UpdateCustomPropertyMetadata(*propertiesJson, *newPod);
    }

    if (oldContainer) {
        newPod->CopyValues(*oldContainer);
    }
    return newPod;
}
} // namespace

class MaterialComponentManager : public IMaterialComponentManager, public IPropertyApi {
    using ComponentId = IComponentManager::ComponentId;

public:
    explicit MaterialComponentManager(IEcs& ecs) noexcept;
    ~MaterialComponentManager() override;

    // IPropertyApi
    size_t PropertyCount() const override;
    const Property* MetaData(size_t index) const override;
    array_view<const Property> MetaData() const override;
    IPropertyHandle* Create() const override;
    IPropertyHandle* Clone(const IPropertyHandle*) const override;
    void Release(IPropertyHandle*) const override;
    uint64_t Type() const override;

    // IComponentManager
    virtual string_view GetName() const override;
    virtual Uid GetUid() const override;
    size_t GetComponentCount() const override;
    const IPropertyApi& GetPropertyApi() const override;
    Entity GetEntity(ComponentId index) const override;
    uint32_t GetComponentGeneration(ComponentId index) const override;
    bool HasComponent(Entity entity) const override;
    IComponentManager::ComponentId GetComponentId(Entity entity) const override;
    void Create(Entity entity) override;
    bool Destroy(Entity entity) override;
    void Gc() override;
    void Destroy(array_view<const Entity> gcList) override;
    vector<Entity> GetAddedComponents() override;
    vector<Entity> GetRemovedComponents() override;
    vector<Entity> GetUpdatedComponents() override;
    vector<Entity> GetMovedComponents() override;
    CORE_NS::ComponentManagerModifiedFlags GetModifiedFlags() const override;
    void ClearModifiedFlags() override;
    uint32_t GetGenerationCounter() const override;
    void SetData(Entity entity, const IPropertyHandle& dataHandle) override;
    const IPropertyHandle* GetData(Entity entity) const override;
    IPropertyHandle* GetData(Entity entity) override;
    void SetData(ComponentId index, const IPropertyHandle& dataHandle) override;
    const IPropertyHandle* GetData(ComponentId index) const override;
    IPropertyHandle* GetData(ComponentId index) override;
    IEcs& GetEcs() const override;

    // IMaterialComponentManager
    MaterialComponent Get(ComponentId index) const override;
    MaterialComponent Get(Entity entity) const override;
    void Set(ComponentId index, const MaterialComponent& aData) override;
    void Set(Entity entity, const MaterialComponent& aData) override;
    ScopedHandle<const MaterialComponent> Read(ComponentId index) const override;
    ScopedHandle<const MaterialComponent> Read(Entity entity) const override;
    ScopedHandle<MaterialComponent> Write(ComponentId index) override;
    ScopedHandle<MaterialComponent> Write(Entity entity) override;

    // internal, non-public
    void Updated(Entity entity);

private:
    bool IsMatchingHandle(const IPropertyHandle& handle);

    BEGIN_PROPERTY(MaterialComponent, componentProperties_)
#include <3d/ecs/components/material_component.h>
    END_PROPERTY();

    static constexpr array_view<const Property> componentMetaData_ { componentProperties_ };

    static constexpr CORE_NS::Property texturesMetaData_[] = {
        RENAMED_MEMBER_PROPERTY(textures[MaterialComponent::TextureIndex::BASE_COLOR], baseColor, "Base Color", 0),
        RENAMED_MEMBER_PROPERTY(textures[MaterialComponent::TextureIndex::NORMAL], normal, "Normal Map", 0),
        RENAMED_MEMBER_PROPERTY(textures[MaterialComponent::TextureIndex::MATERIAL], material, "Material", 0),
        RENAMED_MEMBER_PROPERTY(textures[MaterialComponent::TextureIndex::EMISSIVE], emissive, "Emissive", 0),
        RENAMED_MEMBER_PROPERTY(
            textures[MaterialComponent::TextureIndex::AO], ambientOcclusion, "Ambient Occlusion", 0),
        RENAMED_MEMBER_PROPERTY(textures[MaterialComponent::TextureIndex::CLEARCOAT], clearcoat, "Clearcoat", 0),
        RENAMED_MEMBER_PROPERTY(textures[MaterialComponent::TextureIndex::CLEARCOAT_ROUGHNESS], clearcoatRoughness,
            "Clearcoat Roughness", 0),
        RENAMED_MEMBER_PROPERTY(
            textures[MaterialComponent::TextureIndex::CLEARCOAT_NORMAL], clearcoatNormal, "Clearcoat Normal", 0),

        RENAMED_MEMBER_PROPERTY(textures[MaterialComponent::TextureIndex::SHEEN], sheen, "Sheen", 0),
        RENAMED_MEMBER_PROPERTY(
            textures[MaterialComponent::TextureIndex::TRANSMISSION], transmission, "Transmission", 0),
        RENAMED_MEMBER_PROPERTY(textures[MaterialComponent::TextureIndex::SPECULAR], specular, "Specular", 0),
    };

    static constexpr array_view<const Property> defaultMetaData_ { texturesMetaData_ };

    static constexpr uint64_t typeHash_ = BASE_NS::CompileTime::FNV1aHash(CORE_NS::GetName<MaterialComponent>().data());

    struct ReferencedProperties {
        void Ref()
        {
            ++cnt;
        }

        void Unref()
        {
            if (--cnt == 0) {
                delete this;
            }
        }

        vector<Property> properties;
        // need to be stored to have a valid string_view in properties (for name and display name)
        vector<string> stringStorage;
        uint32_t cnt { 0u };
    };

    class ComponentHandle;

    class MaterialPropertyBindingSignal final : public CustomPropertyWriteSignal {
    public:
        explicit MaterialPropertyBindingSignal(MaterialComponentManager::ComponentHandle* componentHandle)
            : componentHandle_(componentHandle)
        {}
        ~MaterialPropertyBindingSignal() override = default;

        void Signal() override
        {
            componentHandle_->BindBindingProperties();

            // update generation etc..
            ++componentHandle_->generation_;
            if (CORE_NS::EntityUtil::IsValid(componentHandle_->entity_)) {
                componentHandle_->dirty_ = true;
                componentHandle_->manager_->Updated(componentHandle_->entity_);
            }
        }

    private:
        MaterialComponentManager::ComponentHandle* componentHandle_ {};
    };

    class MaterialPropertySignal final : public CustomPropertyWriteSignal {
    public:
        explicit MaterialPropertySignal(MaterialComponentManager::ComponentHandle* componentHandle)
            : componentHandle_(componentHandle)
        {}
        ~MaterialPropertySignal() override = default;

        void Signal() override
        {
            // update generation etc..
            ++componentHandle_->generation_;
            if (CORE_NS::EntityUtil::IsValid(componentHandle_->entity_)) {
                componentHandle_->dirty_ = true;
                componentHandle_->manager_->Updated(componentHandle_->entity_);
            }
        }

    private:
        MaterialComponentManager::ComponentHandle* componentHandle_ {};
    };

    class ComponentHandle : public IPropertyHandle, IPropertyApi {
    public:
        ComponentHandle() = delete;
        ComponentHandle(MaterialComponentManager* owner, Entity entity) noexcept;
        ComponentHandle(MaterialComponentManager* owner, Entity entity, const MaterialComponent& data) noexcept;
        ~ComponentHandle() override = default;
        ComponentHandle(const ComponentHandle& other) = delete;
        ComponentHandle(ComponentHandle&& other) noexcept;
        ComponentHandle& operator=(const ComponentHandle& other);
        ComponentHandle& operator=(ComponentHandle&& other) noexcept;

        // IPropertyHandle
        const IPropertyApi* Owner() const override;
        size_t Size() const override;
        const void* RLock() const override;
        void RUnlock() const override;
        void* WLock() override;
        void WUnlock() override;

        // IPropertyApi
        size_t PropertyCount() const override;
        const Property* MetaData(size_t index) const override;
        array_view<const Property> MetaData() const override;
        uint64_t Type() const override;
        IPropertyHandle* Create() const override;
        IPropertyHandle* Clone(const IPropertyHandle* src) const override;
        void Release(IPropertyHandle* handle) const override;

        void UpdateMetadata();
        void FillMetadata(const RENDER_NS::RenderHandleReference& currentShader,
            const CORE_NS::json::value*& propertiesJson, const CORE_NS::json::value*& propertiesBindingsJson,
            bool updatedShader, array_view<const Property> previousProperties);
        void CreateCustomBindings(
            const CORE_NS::json::value& propertiesBindingsJson, const CustomPropertyBindingContainer* oldBindings);
        // additional method to bind the data from binding properties
        // NOTE: should use IShaderPipelineBinder later
        void BindBindingProperties();

        MaterialComponentManager* manager_ { nullptr };
        Entity entity_;
        struct CachedShader {
            // we do not keep reference here, if it's not in use then it could be destroyed
            RenderHandle shader {};
            // frame index to see the shader "timestamp"
            uint64_t frameIndex { ~0ULL };
        };
        CachedShader cachedShader_;
        refcnt_ptr<ReferencedProperties> metaData_;
        uint32_t generation_ { 0 };
#ifndef NDEBUG
        mutable uint32_t rLocked_ { 0 };
        mutable bool wLocked_ { false };
#endif
        bool dirty_ { false };
        MaterialComponent data_;
        BASE_NS::unique_ptr<CustomPropertyPodContainer> custom_;
        BASE_NS::unique_ptr<CustomPropertyBindingContainer> customBindings_;

        // used with the custom bindings
        MaterialPropertySignal propertySignal_;
        MaterialPropertyBindingSignal propertyBindingSignal_;
    };

    IEcs& ecs_;
    IRenderHandleComponentManager* renderHandleManager_ { nullptr };
    IShaderManager* shaderManager_ { nullptr };

    uint32_t generationCounter_ { 0 };
    uint32_t modifiedFlags_ { 0 };
    unordered_map<Entity, ComponentId> entityComponent_;
    vector<ComponentHandle> components_;
    vector<Entity> added_;
    vector<Entity> removed_;
    vector<Entity> updated_;
    vector<Entity> moved_;

    unordered_map<RenderHandle, refcnt_ptr<ReferencedProperties>> properties_;
};

MaterialComponentManager::MaterialComponentManager(IEcs& ecs) noexcept
    : ecs_(ecs), renderHandleManager_(GetManager<IRenderHandleComponentManager>(ecs))
{
    if (CORE_NS::IEngine* engine = ecs_.GetClassFactory().GetInterface<CORE_NS::IEngine>(); engine) {
        if (IRenderContext* renderContext =
                GetInstance<IRenderContext>(*engine->GetInterface<IClassRegister>(), UID_RENDER_CONTEXT);
            renderContext) {
            shaderManager_ = &renderContext->GetDevice().GetShaderManager();
        }
    }

    // Initial reservation for 64 components/entities.
    // Will resize as needed.
    constexpr size_t initialComponentReserveSize = 64;
    components_.reserve(initialComponentReserveSize);
    entityComponent_.reserve(initialComponentReserveSize);

    auto result =
        properties_.insert_or_assign(RenderHandle {}, refcnt_ptr<ReferencedProperties>(new ReferencedProperties));
    auto& properties = result.first->second->properties;
    properties.reserve(componentMetaData_.size() + defaultMetaData_.size());
    properties.append(componentMetaData_.begin(), componentMetaData_.end());
    properties.append(defaultMetaData_.begin(), defaultMetaData_.end());
}

MaterialComponentManager::~MaterialComponentManager() = default;

// IPropertyApi
size_t MaterialComponentManager::PropertyCount() const
{
    return componentMetaData_.size();
}

const Property* MaterialComponentManager::MetaData(size_t index) const
{
    if (index < componentMetaData_.size()) {
        return &componentMetaData_[index];
    }
    return nullptr;
}

array_view<const Property> MaterialComponentManager::MetaData() const
{
    return componentMetaData_;
}

IPropertyHandle* MaterialComponentManager::Create() const
{
    return new ComponentHandle(const_cast<MaterialComponentManager*>(this), {}, {});
}

IPropertyHandle* MaterialComponentManager::Clone(const IPropertyHandle* src) const
{
    if (src) {
        auto owner = src->Owner();
        if (owner == this) {
            auto* h = static_cast<const ComponentHandle*>(src);
            return new ComponentHandle(const_cast<MaterialComponentManager*>(this), {}, h->data_);
        } else if (owner) {
            return owner->Clone(src);
        }
    }
    return nullptr;
}

void MaterialComponentManager::Release(IPropertyHandle* dst) const
{
    if (!dst) {
        return;
    }
    auto owner = dst->Owner();
    if (owner == this) {
        // we can only destroy things we "own" (know)
        const auto* handle = static_cast<ComponentHandle*>(dst);
        if (const auto id = GetComponentId(handle->entity_);
            (id != IComponentManager::INVALID_COMPONENT_ID) && (&components_[id] == handle)) {
            // This is one of the components (bound to an entity) so do nothing
            return;
        }
        delete handle;
    } else if (owner) {
        owner->Release(dst);
    }
}

uint64_t MaterialComponentManager::Type() const
{
    return typeHash_;
}

// IComponentManager
string_view MaterialComponentManager::GetName() const
{
    constexpr auto name = CORE_NS::GetName<MaterialComponent>();
    return name;
}

Uid MaterialComponentManager::GetUid() const
{
    return IMaterialComponentManager::UID;
}

size_t MaterialComponentManager::GetComponentCount() const
{
    return components_.size();
}

const IPropertyApi& MaterialComponentManager::GetPropertyApi() const
{
    return *this;
}

Entity MaterialComponentManager::GetEntity(ComponentId index) const
{
    if (index < components_.size()) {
        return components_[index].entity_;
    }
    return Entity();
}

uint32_t MaterialComponentManager::GetComponentGeneration(ComponentId index) const
{
    if (index < components_.size()) {
        return components_[index].generation_;
    }
    return 0;
}

bool MaterialComponentManager::HasComponent(Entity entity) const
{
    return GetComponentId(entity) != IComponentManager::INVALID_COMPONENT_ID;
}

IComponentManager::ComponentId MaterialComponentManager::GetComponentId(Entity entity) const
{
    if (CORE_NS::EntityUtil::IsValid(entity)) {
        if (auto it = entityComponent_.find(entity); it != entityComponent_.end()) {
            return it->second;
        }
    }
    return IComponentManager::INVALID_COMPONENT_ID;
}

void MaterialComponentManager::Create(Entity entity)
{
    if (CORE_NS::EntityUtil::IsValid(entity)) {
        if (auto it = entityComponent_.find(entity); it == entityComponent_.end()) {
            entityComponent_.insert({ entity, static_cast<ComponentId>(components_.size()) });
            const auto oldCapacity = components_.capacity();
            auto& component = components_.emplace_back(this, entity);
            if (components_.capacity() != oldCapacity) {
                moved_.reserve(moved_.size() + components_.size());
                for (const auto& handle : components_) {
                    moved_.push_back(handle.entity_);
                }
                modifiedFlags_ |= CORE_NS::CORE_COMPONENT_MANAGER_COMPONENT_MOVED_BIT;
            }
            added_.push_back(entity);
            modifiedFlags_ |= CORE_NS::CORE_COMPONENT_MANAGER_COMPONENT_ADDED_BIT;
            generationCounter_++;

            // lock/unlock for toggling component updated behavior.
            component.WLock();
            component.WUnlock();
        } else {
            if (auto dst = CORE_NS::ScopedHandle<MaterialComponent>(&components_[it->second]); dst) {
                *dst = {};
            }
        }
    }
}

bool MaterialComponentManager::Destroy(Entity entity)
{
    if (CORE_NS::EntityUtil::IsValid(entity)) {
        if (auto it = entityComponent_.find(entity); it != entityComponent_.end()) {
            components_[it->second].entity_ = {}; // invalid entity. (marks it as ready for re-use)
            entityComponent_.erase(it);
            removed_.push_back(entity);
            modifiedFlags_ |= CORE_NS::CORE_COMPONENT_MANAGER_COMPONENT_REMOVED_BIT;
            generationCounter_++;
            return true;
        }
    }
    return false;
}

void MaterialComponentManager::Gc()
{
    const bool hasRemovedComponents = modifiedFlags_ & CORE_NS::CORE_COMPONENT_MANAGER_COMPONENT_REMOVED_BIT;
    if (!hasRemovedComponents) {
        return;
    }
    ComponentId componentCount = static_cast<ComponentId>(components_.size());
    for (ComponentId id = 0; id < componentCount;) {
        if (CORE_NS::EntityUtil::IsValid(components_[id].entity_)) {
            ++id;
            continue;
        }
        // invalid entity.. if so clean garbage
        // find last valid and swap with it
        ComponentId rid = componentCount - 1;
        while ((rid > id) && !CORE_NS::EntityUtil::IsValid(components_[rid].entity_)) {
            --rid;
        }
        if ((rid > id) && CORE_NS::EntityUtil::IsValid(components_[rid].entity_)) {
            moved_.push_back(components_[rid].entity_);
            // fix the entityComponent_ map (update the component id for the entity)
            entityComponent_[components_[rid].entity_] = id;
            components_[id] = BASE_NS::move(components_[rid]);
        }
        --componentCount;
    }
    if (!moved_.empty()) {
        modifiedFlags_ |= CORE_NS::CORE_COMPONENT_MANAGER_COMPONENT_MOVED_BIT;
    }
    if (components_.size() > componentCount) {
        auto diff = static_cast<typename decltype(components_)::difference_type>(componentCount);
        components_.erase(components_.cbegin() + diff, components_.cend());
    }
}

void MaterialComponentManager::Destroy(const array_view<const Entity> gcList)
{
    for (const Entity e : gcList) {
        Destroy(e);
    }
}

vector<Entity> MaterialComponentManager::GetAddedComponents()
{
    return BASE_NS::move(added_);
}

vector<Entity> MaterialComponentManager::GetRemovedComponents()
{
    return BASE_NS::move(removed_);
}

vector<Entity> MaterialComponentManager::GetUpdatedComponents()
{
    vector<Entity> updated;
    if (modifiedFlags_ & MODIFIED) {
        modifiedFlags_ &= ~MODIFIED;
        updated.reserve(components_.size() / 2); // 2: half
        for (auto& handle : components_) {
            if (handle.dirty_) {
                handle.dirty_ = false;
                updated.push_back(handle.entity_);
            }
        }
    }
    return updated;
}

vector<Entity> MaterialComponentManager::GetMovedComponents()
{
    return BASE_NS::move(moved_);
}

CORE_NS::ComponentManagerModifiedFlags MaterialComponentManager::GetModifiedFlags() const
{
    return modifiedFlags_ & ~MODIFIED;
}

void MaterialComponentManager::ClearModifiedFlags()
{
    modifiedFlags_ &= MODIFIED;
}

uint32_t MaterialComponentManager::GetGenerationCounter() const
{
    return generationCounter_;
}

void MaterialComponentManager::SetData(Entity entity, const IPropertyHandle& dataHandle)
{
    if (!IsMatchingHandle(dataHandle)) {
        return;
    }
    if (const auto src = ScopedHandle<const MaterialComponent>(&dataHandle); src) {
        if (const auto it = entityComponent_.find(entity); it != entityComponent_.end()) {
            components_[it->second] = static_cast<const ComponentHandle&>(dataHandle);
        }
    }
}

const IPropertyHandle* MaterialComponentManager::GetData(Entity entity) const
{
    if (CORE_NS::EntityUtil::IsValid(entity)) {
        if (const auto it = entityComponent_.find(entity); it != entityComponent_.end()) {
            if (it->second < components_.size()) {
                return &components_[it->second];
            }
        }
    }
    return nullptr;
}

IPropertyHandle* MaterialComponentManager::GetData(Entity entity)
{
    if (CORE_NS::EntityUtil::IsValid(entity)) {
        if (const auto it = entityComponent_.find(entity); it != entityComponent_.end()) {
            if (it->second < components_.size()) {
                return &components_[it->second];
            }
        }
    }
    return nullptr;
}

void MaterialComponentManager::SetData(ComponentId index, const IPropertyHandle& dataHandle)
{
    if (!IsMatchingHandle(dataHandle)) {
        // We could verify the metadata here.
        // And in copy only the matching properties one-by-one also.
        return;
    }
    if (index < components_.size()) {
        if (const auto src = ScopedHandle<const MaterialComponent>(&dataHandle); src) {
            components_[index] = static_cast<const ComponentHandle&>(dataHandle);
        }
    }
}

const IPropertyHandle* MaterialComponentManager::GetData(ComponentId index) const
{
    if (index < components_.size()) {
        return &components_[index];
    }
    return nullptr;
}

IPropertyHandle* MaterialComponentManager::GetData(ComponentId index)
{
    if (index < components_.size()) {
        return &components_[index];
    }
    return nullptr;
}

IEcs& MaterialComponentManager::GetEcs() const
{
    return ecs_;
}

// IMaterialComponentManager
MaterialComponent MaterialComponentManager::Get(ComponentId index) const
{
    if (auto handle = Read(index); handle) {
        return *handle;
    }
    return MaterialComponent {};
}

MaterialComponent MaterialComponentManager::Get(Entity entity) const
{
    if (auto handle = Read(entity); handle) {
        return *handle;
    }
    return MaterialComponent {};
}

void MaterialComponentManager::Set(ComponentId index, const MaterialComponent& data)
{
    if (auto handle = Write(index); handle) {
        *handle = data;
    }
}

void MaterialComponentManager::Set(Entity entity, const MaterialComponent& data)
{
    if (CORE_NS::EntityUtil::IsValid(entity)) {
        if (auto handle = Write(entity); handle) {
            *handle = data;
            // NOTE: customProperties are valid when updating itself
        } else {
            entityComponent_.insert({ entity, static_cast<ComponentId>(components_.size()) });
            const auto oldCapacity = components_.capacity();
            auto& component = components_.emplace_back(this, entity, data);
            if (components_.capacity() != oldCapacity) {
                moved_.reserve(moved_.size() + components_.size());
                for (const auto& componentHandle : components_) {
                    moved_.push_back(componentHandle.entity_);
                }
                modifiedFlags_ |= CORE_NS::CORE_COMPONENT_MANAGER_COMPONENT_MOVED_BIT;
            }
            added_.push_back(entity);
            modifiedFlags_ |= CORE_NS::CORE_COMPONENT_MANAGER_COMPONENT_ADDED_BIT;
            generationCounter_++;

            // lock/unlock for toggling component updated behavior.
            component.WLock();
            component.WUnlock();
        }
    }
}

ScopedHandle<const MaterialComponent> MaterialComponentManager::Read(ComponentId index) const
{
    return ScopedHandle<const MaterialComponent> { GetData(index) };
}

ScopedHandle<const MaterialComponent> MaterialComponentManager::Read(Entity entity) const
{
    return ScopedHandle<const MaterialComponent> { GetData(entity) };
}

ScopedHandle<MaterialComponent> MaterialComponentManager::Write(ComponentId index)
{
    return ScopedHandle<MaterialComponent> { GetData(index) };
}

ScopedHandle<MaterialComponent> MaterialComponentManager::Write(Entity entity)
{
    return ScopedHandle<MaterialComponent> { GetData(entity) };
}

// Internal
void MaterialComponentManager::Updated(Entity entity)
{
    CORE_ASSERT_MSG(CORE_NS::EntityUtil::IsValid(entity), "Invalid ComponentId, bound to INVALID_ENTITY");
    modifiedFlags_ |= CORE_NS::CORE_COMPONENT_MANAGER_COMPONENT_UPDATED_BIT | MODIFIED;
    generationCounter_++;
}

bool MaterialComponentManager::IsMatchingHandle(const IPropertyHandle& dataHandle)
{
    if (dataHandle.Owner() == this) {
        return true;
    }
    if (dataHandle.Owner() && (dataHandle.Owner()->Type() == typeHash_)) {
        return true;
    }
    return false;
}

// Handle implementation
MaterialComponentManager::ComponentHandle::ComponentHandle(MaterialComponentManager* owner, Entity entity) noexcept
    : ComponentHandle(owner, entity, {})
{}

WARNING_SCOPE_START(W_THIS_USED_BASE_INITIALIZER_LIST);
MaterialComponentManager::ComponentHandle::ComponentHandle(
    MaterialComponentManager* owner, Entity entity, const MaterialComponent& data) noexcept
    : manager_(owner), entity_(entity), data_(data), propertySignal_(this), propertyBindingSignal_(this)
{}

MaterialComponentManager::ComponentHandle::ComponentHandle(ComponentHandle&& other) noexcept
    : manager_(other.manager_), entity_(exchange(other.entity_, {})), cachedShader_(exchange(other.cachedShader_, {})),
      metaData_(exchange(other.metaData_, nullptr)), generation_(exchange(other.generation_, 0U)),
#ifndef NDEBUG
      rLocked_(exchange(other.rLocked_, 0U)), wLocked_(exchange(other.wLocked_, false)),
#endif
      data_(exchange(other.data_, {})), custom_(exchange(other.custom_, {})),
      customBindings_(exchange(other.customBindings_, {})), propertySignal_(this), propertyBindingSignal_(this)
{
    if (custom_) {
        custom_->UpdateSignal(propertySignal_);
    }
    if (customBindings_) {
        customBindings_->UpdateSignal(propertyBindingSignal_);
    }
}
WARNING_SCOPE_END();

MaterialComponentManager::ComponentHandle& MaterialComponentManager::ComponentHandle::operator=(
    ComponentHandle&& other) noexcept
{
    if (this != &other) {
        CORE_ASSERT(manager_ == other.manager_);
        entity_ = exchange(other.entity_, {});
        cachedShader_ = exchange(other.cachedShader_, {});
        metaData_ = exchange(other.metaData_, nullptr);
        generation_ = exchange(other.generation_, 0U);
#ifndef NDEBUG
        rLocked_ = exchange(other.rLocked_, 0U);
        wLocked_ = exchange(other.wLocked_, false);
#endif
        dirty_ = exchange(other.dirty_, false);
        data_ = exchange(other.data_, {});
        custom_ = exchange(other.custom_, {});
        customBindings_ = exchange(other.customBindings_, {});
        if (custom_) {
            custom_->UpdateSignal(propertySignal_);
        }
        if (customBindings_) {
            customBindings_->UpdateSignal(propertyBindingSignal_);
        }
    }
    return *this;
}

// ComponentHandle IPropertyHandle
const IPropertyApi* MaterialComponentManager::ComponentHandle::Owner() const
{
    return this;
}

size_t MaterialComponentManager::ComponentHandle::Size() const
{
    return sizeof(MaterialComponent);
}

const void* MaterialComponentManager::ComponentHandle::RLock() const
{
    CORE_ASSERT(manager_);
#ifndef NDEBUG
    CORE_ASSERT(!wLocked_);
    rLocked_++;
#endif
    return &data_;
}

void MaterialComponentManager::ComponentHandle::RUnlock() const
{
    CORE_ASSERT(manager_);
#ifndef NDEBUG
    CORE_ASSERT(rLocked_ > 0U);
    rLocked_--;
#endif
}

void* MaterialComponentManager::ComponentHandle::WLock()
{
    CORE_ASSERT(manager_);
#ifndef NDEBUG
    CORE_ASSERT(rLocked_ <= 1U && !wLocked_);
    wLocked_ = true;
#endif
    return &data_;
}

void MaterialComponentManager::ComponentHandle::WUnlock()
{
    CORE_ASSERT(manager_);
#ifndef NDEBUG
    CORE_ASSERT(wLocked_);
    wLocked_ = false;
#endif
    // update generation etc..
    generation_++;
    if (CORE_NS::EntityUtil::IsValid(entity_)) {
        dirty_ = true;
        UpdateMetadata();
        manager_->Updated(entity_);
    }
}

// ComponentHandle IPropertyApi
size_t MaterialComponentManager::ComponentHandle::PropertyCount() const
{
    if (!metaData_) {
        return manager_->PropertyCount();
    } else {
        return metaData_->properties.size();
    }
}

const Property* MaterialComponentManager::ComponentHandle::MetaData(size_t index) const
{
    if (!metaData_) {
        return manager_->MetaData(index);
    } else if (index < metaData_->properties.size()) {
        return &metaData_->properties[index];
    }
    return nullptr;
}

array_view<const Property> MaterialComponentManager::ComponentHandle::MetaData() const
{
    if (!metaData_) {
        return manager_->MetaData();
    } else {
        return metaData_->properties;
    }
}

uint64_t MaterialComponentManager::ComponentHandle::Type() const
{
    return manager_->Type();
}

IPropertyHandle* MaterialComponentManager::ComponentHandle::Create() const
{
    return new ComponentHandle(manager_, {}, {});
}

IPropertyHandle* MaterialComponentManager::ComponentHandle::Clone(const IPropertyHandle* src) const
{
    if (src) {
        auto owner = src->Owner();
        if (owner == this) {
            auto* h = static_cast<const ComponentHandle*>(src);
            return new ComponentHandle(h->manager_, {}, h->data_);
        } else if (owner) {
            return owner->Clone(src);
        }
        return manager_->Clone(src);
    }
    return nullptr;
}

void MaterialComponentManager::ComponentHandle::Release(IPropertyHandle* handle) const
{
    if (handle) {
        auto owner = handle->Owner();
        if (owner == this) {
            auto* componentHandle = static_cast<ComponentHandle*>(handle);
            if (auto id = manager_->GetComponentId(componentHandle->entity_);
                id != IComponentManager::INVALID_COMPONENT_ID) {
                if (manager_->GetData(id) == componentHandle) {
                    // This is one of the components (bound to an entity) so do nothing
                    return;
                }
            }
            delete componentHandle;
        } else if (owner) {
            owner->Release(handle);
        }
    }
}

// ComponentHandle
MaterialComponentManager::ComponentHandle& MaterialComponentManager::ComponentHandle::operator=(
    const MaterialComponentManager::ComponentHandle& other)
{
    if (this != &other) {
        if (!manager_->renderHandleManager_) {
            manager_->renderHandleManager_ = GetManager<IRenderHandleComponentManager>(manager_->ecs_);
        }

        // copy the material component, but..
        data_ = other.data_;
        // ..forget the custom property values and create new ones
        data_.customProperties = nullptr;
        data_.customBindingProperties = nullptr;

        const CORE_NS::json::value* propertiesJson = nullptr;
        const CORE_NS::json::value* propertiesBindingsJson = nullptr;
        if (manager_ == other.manager_) {
            // for the same manager instance we can use the same metadata
            cachedShader_ = other.cachedShader_;
            metaData_ = other.metaData_;
        } else {
            // for a different manager instance find metadata matching the shader or create new metadata
            if (manager_->renderHandleManager_ && manager_->shaderManager_) {
                const auto currentShader =
                    manager_->renderHandleManager_->GetRenderHandleReference(data_.materialShader.shader);
                const auto frameIndex = currentShader ? manager_->shaderManager_->GetFrameIndex(currentShader) : 0U;
                const bool updatedShader = other.cachedShader_.frameIndex != frameIndex;
                cachedShader_ = { currentShader.GetHandle(), frameIndex };

                if (auto pos = manager_->properties_.find(cachedShader_.shader);
                    (pos != manager_->properties_.end()) && !updatedShader) {
                    metaData_ = pos->second;
                } else {
                    FillMetadata(currentShader, propertiesJson, propertiesBindingsJson, updatedShader,
                        other.metaData_ ? other.metaData_->properties : array_view<const Property> {});
                }
            }
        }

        // find the custom properties for the shader
        if (data_.materialShader.shader && !propertiesJson && !propertiesBindingsJson) {
            if (manager_->renderHandleManager_ && manager_->shaderManager_) {
                auto currentShader =
                    manager_->renderHandleManager_->GetRenderHandleReference(data_.materialShader.shader);

                if (auto* metaJson = manager_->shaderManager_->GetMaterialMetadata(currentShader);
                    metaJson && metaJson->is_array()) {
                    if (auto* material = FindMaterialComponentJson(*metaJson); material) {
                        if (auto* propJson = material->find(CUSTOM_PROPERTIES); propJson && propJson->is_array()) {
                            propertiesJson = propJson;
                        }
                        if (auto* propJson = material->find(CUSTOM_BINDING_PROPERTIES);
                            propJson && propJson->is_array()) {
                            propertiesBindingsJson = propJson;
                        }
                    }
                }
            }
        }

        if (metaData_ && (propertiesJson || propertiesBindingsJson)) {
            // create and fill new POD container based on 'other's container
            custom_ = CreateCustomPropertyPodContainer(propertySignal_, propertiesJson, other.custom_.get());
            data_.customProperties = custom_.get();

            // create and fill new binding container
            if (propertiesBindingsJson) {
                CreateCustomBindings(*propertiesBindingsJson, other.customBindings_.get());
            }
        } else if (!metaData_) {
            if (auto pos = manager_->properties_.find(RenderHandle {}); pos != manager_->properties_.cend()) {
                metaData_ = pos->second;
            }
        }

        ++generation_;
        dirty_ = true;
        manager_->Updated(entity_);
    }

    return *this;
}

void MaterialComponentManager::ComponentHandle::UpdateMetadata()
{
    if (!manager_->renderHandleManager_) {
        manager_->renderHandleManager_ = GetManager<IRenderHandleComponentManager>(manager_->ecs_);
        if (!manager_->renderHandleManager_) {
            return;
        }
    }
    if (!manager_->shaderManager_) {
        return;
    }
    const auto currentShader = manager_->renderHandleManager_->GetRenderHandleReference(data_.materialShader.shader);
    const auto frameIndex = currentShader ? manager_->shaderManager_->GetFrameIndex(currentShader) : 0U;
    const bool newShader = (cachedShader_.shader != currentShader.GetHandle());
    const bool updatedShader = !newShader && (cachedShader_.frameIndex != frameIndex);
    if (!metaData_ || newShader || updatedShader) {
        cachedShader_ = { currentShader.GetHandle(), frameIndex };

        const CORE_NS::json::value* propertiesJson = nullptr;
        const CORE_NS::json::value* propertiesBindingsJson = nullptr;
        auto& propertyCache = manager_->properties_;
        if ((newShader || updatedShader) && currentShader) {
            FillMetadata(currentShader, propertiesJson, propertiesBindingsJson, updatedShader,
                metaData_ ? metaData_->properties : array_view<const Property> {});
        } else if (auto cached = propertyCache.find(cachedShader_.shader); cached != propertyCache.end()) {
            metaData_ = cached->second;
        }

        data_.customProperties = nullptr;
        data_.customBindingProperties = nullptr;
        if (metaData_ && (propertiesJson || propertiesBindingsJson)) {
            // create and fill new POD container
            custom_ = CreateCustomPropertyPodContainer(
                propertySignal_, propertiesJson, updatedShader ? custom_.get() : nullptr);
            data_.customProperties = custom_.get();

            // create and fill new binding container
            if (propertiesBindingsJson) {
                CreateCustomBindings(*propertiesBindingsJson, updatedShader ? customBindings_.get() : nullptr);
            }
        } else if (!metaData_) {
            if (auto pos = manager_->properties_.find(RenderHandle {}); pos != manager_->properties_.cend()) {
                metaData_ = pos->second;
            }
        }
    }
}

void MaterialComponentManager::ComponentHandle::FillMetadata(const RenderHandleReference& currentShader,
    const CORE_NS::json::value*& propertiesJson, const CORE_NS::json::value*& propertiesBindingsJson,
    bool updatedShader, array_view<const Property> oldProperties)
{
    if (!manager_->shaderManager_) {
        return;
    }

    // start with a blank property collection
    auto newMetaData = refcnt_ptr<ReferencedProperties>(new ReferencedProperties);
    auto& newProperties = newMetaData->properties;

    // insert basic properties from the component
    newProperties.append(componentMetaData_.begin(), componentMetaData_.end());
    const auto basicProperties = newProperties.size();

    // go through the metadata which should be an array of metadata for different component types
    if (auto* metaJson = manager_->shaderManager_->GetMaterialMetadata(currentShader);
        metaJson && metaJson->is_array()) {
        if (auto* material = FindMaterialComponentJson(*metaJson); material) {
            // insert properties for textures
            AppendProperties(newProperties, newMetaData->stringStorage, *material, componentMetaData_);

            if (auto* propJson = material->find(CUSTOM_PROPERTIES); propJson && propJson->is_array()) {
                propertiesJson = propJson;
            }
            if (auto* propJson = material->find(CUSTOM_BINDING_PROPERTIES); propJson && propJson->is_array()) {
                propertiesBindingsJson = propJson;
            }
        }
    }
    // if the material had metadata with customized TextureInfos try to map old locations to new ones.
    if (updatedShader && (oldProperties.size() > basicProperties) && (newProperties.size() > basicProperties)) {
        MapTextureSlots(data_.textures, offsetof(MaterialComponent, textures),
            array_view(newProperties.cbegin().ptr() + basicProperties, newProperties.cend().ptr()),
            array_view(oldProperties.cbegin().ptr() + basicProperties, oldProperties.cend().ptr()));
    }

    // replace previous properties with the new updated properties in the cache and this component handle
    manager_->properties_.insert_or_assign(currentShader.GetHandle(), newMetaData);
    metaData_ = move(newMetaData);
}

void MaterialComponentManager::ComponentHandle::CreateCustomBindings(
    const CORE_NS::json::value& propertiesBindingsJson, const CustomPropertyBindingContainer* oldBindings)
{
    auto newBindings = BASE_NS::make_unique<CustomPropertyBindingContainer>(propertyBindingSignal_);
    UpdateCustomBindingPropertyMetadata(propertiesBindingsJson, *newBindings);
    if (oldBindings) {
        newBindings->CopyValues(*oldBindings);
    }
    customBindings_ = BASE_NS::move(newBindings);
    data_.customBindingProperties = customBindings_.get();
}

void MaterialComponentManager::ComponentHandle::BindBindingProperties()
{
    // fetch the bindings from properties, and try to bind them
    if (customBindings_ && (customBindings_->PropertyCount() > 0)) {
        uint32_t bindingIdx = 0;
        const uint32_t customBindingCount = static_cast<uint32_t>(customBindings_->PropertyCount());
        if (data_.customResources.size() < customBindingCount) {
            data_.customResources.resize(customBindingCount);
        }
        const uint32_t customResourceCount = static_cast<uint32_t>(data_.customResources.size());
        for (auto& prop : customBindings_->Owner()->MetaData()) {
            switch (prop.type) {
                case CORE_NS::PropertyType::ENTITY_REFERENCE_T: {
                    if (bindingIdx < customResourceCount) {
                        if (const auto ent = customBindings_->GetValue<CORE_NS::EntityReference>(bindingIdx); ent) {
                            data_.customResources[bindingIdx] = ent;
                        }
                    }
                } break;
                default: {
                } break;
            }
            bindingIdx++;
        }
    }
}

IComponentManager* IMaterialComponentManagerInstance(IEcs& ecs)
{
    return new MaterialComponentManager(ecs);
}

void IMaterialComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<MaterialComponentManager*>(instance);
}
CORE3D_END_NAMESPACE()
