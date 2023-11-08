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
#include <base/util/compile_time_hashes.h>
#include <core/ecs/intf_ecs.h>
#include <core/intf_engine.h>
#include <core/property/intf_property_api.h>
#include <core/property/property_types.h>
#include <render/device/intf_shader_manager.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>

#define IMPLEMENT_MANAGER
#include <algorithm>
#include <iterator>

#include "PropertyTools/property_macros.h"

CORE_BEGIN_NAMESPACE()
using BASE_NS::string_view;
using BASE_NS::vector;
using CORE3D_NS::MaterialComponent;

/** Extend propertysystem with the types */
DECLARE_PROPERTY_TYPE(MaterialComponent::Type);
DECLARE_PROPERTY_TYPE(MaterialComponent::TextureTransform);
DECLARE_PROPERTY_TYPE(MaterialComponent::TextureInfo);
DECLARE_PROPERTY_TYPE(MaterialComponent::Shader);
DECLARE_PROPERTY_TYPE(vector<MaterialComponent::TextureTransform>);
DECLARE_PROPERTY_TYPE(CORE_NS::IPropertyHandle*);

// Declare their metadata
BEGIN_ENUM(MaterialTypeMetaData, MaterialComponent::Type)
DECL_ENUM(MaterialComponent::Type, METALLIC_ROUGHNESS, "Metallic Roughness")
DECL_ENUM(MaterialComponent::Type, SPECULAR_GLOSSINESS, "Specular Glossiness")
DECL_ENUM(MaterialComponent::Type, UNLIT, "Unlit")
DECL_ENUM(MaterialComponent::Type, UNLIT_SHADOW_ALPHA, "Unlit Shadow Alpha")
DECL_ENUM(MaterialComponent::Type, CUSTOM, "Custom material")
DECL_ENUM(MaterialComponent::Type, CUSTOM_COMPLEX, "Custom complex material")
END_ENUM(MaterialTypeMetaData, MaterialComponent::Type)

BEGIN_ENUM(MaterialLightingFlagBitsMetaData, MaterialComponent::LightingFlagBits)
DECL_ENUM(MaterialComponent::LightingFlagBits, SHADOW_RECEIVER_BIT, "Shadow Receiver")
DECL_ENUM(MaterialComponent::LightingFlagBits, SHADOW_CASTER_BIT, "Shadow Caster")
DECL_ENUM(MaterialComponent::LightingFlagBits, PUNCTUAL_LIGHT_RECEIVER_BIT, "Punctual Light Receiver")
DECL_ENUM(MaterialComponent::LightingFlagBits, INDIRECT_LIGHT_RECEIVER_BIT, "Indirect Light Receiver")
END_ENUM(MaterialLightingFlagBitsMetaData, MaterialComponent::LightingFlagBits)

BEGIN_ENUM(ExtraMaterialRenderingFlagBitsMetaData, MaterialComponent::ExtraRenderingFlagBits)
DECL_ENUM(MaterialComponent::ExtraRenderingFlagBits, DISCARD_BIT, "Discard")
END_ENUM(ExtraMaterialRenderingFlagBitsMetaData, MaterialComponent::ExtraRenderingFlagBits)

BEGIN_METADATA(TextureTransformMetaData, MaterialComponent::TextureTransform)
DECL_PROPERTY2(MaterialComponent::TextureTransform, translation, "Translation", 0)
DECL_PROPERTY2(MaterialComponent::TextureTransform, rotation, "Rotation", 0)
DECL_PROPERTY2(MaterialComponent::TextureTransform, scale, "Scale", 0)
END_METADATA(TextureTransformMetaData, MaterialComponent::TextureTransform)

BEGIN_METADATA(TextureInfoMetaData, MaterialComponent::TextureInfo)
DECL_PROPERTY2(MaterialComponent::TextureInfo, image, "Texture", 0)
DECL_PROPERTY2(MaterialComponent::TextureInfo, sampler, "Sampler", 0)
DECL_PROPERTY2(MaterialComponent::TextureInfo, factor, "Factor", 0)
DECL_PROPERTY2(MaterialComponent::TextureInfo, transform, "Transform", 0)
END_METADATA(TextureInfoMetaData, MaterialComponent::TextureInfo)

BEGIN_METADATA(MaterialComponentShaderMetaData, MaterialComponent::Shader)
DECL_PROPERTY2(MaterialComponent::Shader, shader, "Shader", 0)
DECL_PROPERTY2(MaterialComponent::Shader, graphicsState, "Graphics State", 0)
END_METADATA(MaterialComponentShaderMetaData, MaterialComponent::Shader)
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::string_view;
using BASE_NS::countof;
using BASE_NS::vector;
using BASE_NS::Uid;
using BASE_NS::string;
using BASE_NS::refcnt_ptr;
using BASE_NS::unordered_map;
using BASE_NS::move;
using BASE_NS::exchange;
using BASE_NS::forward;

using CORE_NS::Property;
using CORE_NS::IPropertyApi;
using CORE_NS::IEcs;
using CORE_NS::IPropertyHandle;
using CORE_NS::IComponentManager;
using CORE_NS::Entity;
using CORE_NS::PropertyFlags;
using CORE_NS::ScopedHandle;
using CORE_NS::GetInstance;
using CORE_NS::IClassRegister;
using CORE_NS::GetManager;

using RENDER_NS::RenderHandleReference;
using RENDER_NS::RenderHandle;
using RENDER_NS::IRenderContext;
using RENDER_NS::UID_RENDER_CONTEXT;
using RENDER_NS::IShaderManager;

namespace {
constexpr string_view PROPERTIES = "properties";
constexpr string_view SET = "set";
constexpr string_view BINDING = "binding";
constexpr string_view NAME = "name";
constexpr string_view DISPLAY_NAME = "displayName";
constexpr string_view MATERIAL_COMPONENT_NAME = CORE_NS::GetName<MaterialComponent>();

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

void AppendProperties(vector<Property>& newProperties, const CORE_NS::json::value& material,
    const array_view<const Property> componentMetaData)
{
    if (const CORE_NS::json::value* propertiesJson = material.find(PROPERTIES);
        propertiesJson && propertiesJson->is_object()) {
        for (const auto& propertyObject : propertiesJson->object_) {
            if (const auto propertyIt =
                    FindIf(componentMetaData, [key = propertyObject.key](const Property& p) { return p.name == key; });
                propertyIt != componentMetaData.end()) {
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
                    uint32_t index = 0u;
                    for (const CORE_NS::json::value& value : propertyObject.value.array_) {
                        // if the array has objects, the metadata should have containerMethods for
                        // the members
                        if (value.is_object() && !value.object_.empty() && propertyIt->metaData.containerMethods) {
                            const auto& containerProperty = propertyIt->metaData.containerMethods->property;

                            // calculate offset to current struct in the array
                            const auto offset = propertyIt->offset + containerProperty.size * index;
                            Property newProperty = containerProperty;

                            // add the currect struct offset to the member offset
                            newProperty.offset += offset;

                            // update 'name' and 'displayName' if needed
                            if (auto name = value.find(NAME); name && name->is_string()) {
                                newProperty.name = name->string_;
                            }
                            if (auto displayName = value.find(DISPLAY_NAME); displayName && displayName->is_string()) {
                                newProperty.displayName = displayName->string_;
                            }
                            newProperties.push_back(newProperty);
                        } else {
                            // NOTE: handle non-object properties?
                        }
                        ++index;
                    }
                } else {
                    // NOTE: handle non-array properties?
                }
            }
        }
    }
}
} // namespace

#if _MSC_VER
// visual studio 2017 does not handle [[deprecated]] properly.
// so disable deprecation warning for this declaration.
// it will still properly warn in use.
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

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

    bool Clone(IPropertyHandle& dst, const IPropertyHandle& src) const override;
    void Destroy(IPropertyHandle& dataHandle) const override;

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
    static constexpr array_view<const Property> componentMetaData_ { componentProperties_,
        countof(componentProperties_) };
    // clang-format off
    static constexpr CORE_NS::Property texturesMetaData_[] = {
        DECL_NAMED_PROPERTY2(baseColor, textures[MaterialComponent::TextureIndex::BASE_COLOR], "Base color", 0)
        DECL_NAMED_PROPERTY2(normal, textures[MaterialComponent::TextureIndex::NORMAL], "Normal map", 0)
        DECL_NAMED_PROPERTY2(material, textures[MaterialComponent::TextureIndex::MATERIAL], "Material", 0)
        DECL_NAMED_PROPERTY2(emissive, textures[MaterialComponent::TextureIndex::EMISSIVE], "Emissive", 0)
        DECL_NAMED_PROPERTY2(ambientOcclusion, textures[MaterialComponent::TextureIndex::AO], "Ambient occlusion", 0)
        DECL_NAMED_PROPERTY2(clearcoat, textures[MaterialComponent::TextureIndex::CLEARCOAT], "Clearcoat", 0)
        DECL_NAMED_PROPERTY2(clearcoatRoughness, textures[MaterialComponent::TextureIndex::CLEARCOAT_ROUGHNESS],
            "Clearcoat roughness", 0)
        DECL_NAMED_PROPERTY2(clearcoatNormal, textures[MaterialComponent::TextureIndex::CLEARCOAT_NORMAL],
            "Clearcoat normal", 0)
        DECL_NAMED_PROPERTY2(sheen, textures[MaterialComponent::TextureIndex::SHEEN], "Sheen, roughness in alpha", 0)
        DECL_NAMED_PROPERTY2(transmission, textures[MaterialComponent::TextureIndex::TRANSMISSION], "Transmission", 0)
        DECL_NAMED_PROPERTY2(specular, textures[MaterialComponent::TextureIndex::SPECULAR], "Specular", 0)
    };
    // clang-format on
    static constexpr array_view<const Property> defaultMetaData_ { texturesMetaData_, countof(texturesMetaData_) };

    static constexpr uint64_t typeHash_ = BASE_NS::CompileTime::FNV1aHash(
        CORE_NS::GetName<MaterialComponent>().data(), CORE_NS::GetName<MaterialComponent>().size());

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
        const CORE_NS::json::value* customProperties { nullptr };
        uint32_t cnt { 0u };
    };

    class CustomProperties : public IPropertyHandle, IPropertyApi {
    public:
        CustomProperties() = default;

        ~CustomProperties() override = default;

        CustomProperties(const CustomProperties& other) = delete;
        CustomProperties(CustomProperties&& other) noexcept;
        CustomProperties& operator=(const CustomProperties& other) = delete;
        CustomProperties& operator=(CustomProperties&& other) noexcept;

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
        bool Clone(IPropertyHandle& dst, const IPropertyHandle& src) const override;
        void Destroy(IPropertyHandle& dst) const override;

        void Reset();
        void Update(const CORE_NS::json::value& customProperties);
        uintptr_t AppendSet(uintptr_t offset, const CORE_NS::json::value& value);
        uintptr_t AppendBinding(uintptr_t offset, const CORE_NS::json::value& value);
        uintptr_t AppendVec4(string_view name, string_view displayName, uintptr_t offset,
            const CORE_NS::json::value* value);
        uintptr_t AppendUVec4(string_view name, string_view displayName, uintptr_t offset,
            const CORE_NS::json::value* value);
        uintptr_t AppendFloat(string_view name, string_view displayName, uintptr_t offset,
            const CORE_NS::json::value* value);
        uintptr_t AppendUint(string_view name, string_view displayName, uintptr_t offset,
            const CORE_NS::json::value* value);
        struct Strings {
            string name;
            string displayName;
        };
        vector<Strings> metaStrings_;
        vector<Property> metaData_;
        vector<uint8_t> data_;
    };

    class ComponentHandle : public IPropertyHandle, IPropertyApi {
    public:
        ComponentHandle() = delete;
        ComponentHandle(MaterialComponentManager* owner, Entity entity) noexcept;
        ComponentHandle(MaterialComponentManager* owner, Entity entity, const MaterialComponent& data) noexcept;
        ~ComponentHandle() override = default;
        ComponentHandle(const ComponentHandle& other) = delete;
        ComponentHandle(ComponentHandle&& other) noexcept;
        ComponentHandle& operator=(const ComponentHandle& other) = delete;
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
        bool Clone(IPropertyHandle& dst, const IPropertyHandle& src) const override;
        void Destroy(IPropertyHandle& dst) const override;

        void UpdateMetadata() const;

        MaterialComponentManager* manager_ { nullptr };
        Entity entity_;
        mutable RenderHandleReference cachedShader_;
        mutable refcnt_ptr<ReferencedProperties> metaData_;
        uint32_t generation_ { 0 };
        mutable uint32_t rLocked_ { 0 };
        mutable bool wLocked_ { false };
        bool dirty_ { false };
        mutable MaterialComponent data_;
        mutable CustomProperties custom_;
    };

    IEcs& ecs_;
    IShaderManager& shaderManager_;
    IRenderHandleComponentManager* renderHandleManager_ { nullptr };

    uint32_t generationCounter_ { 0 };
    uint32_t modifiedFlags_ { 0 };
    unordered_map<Entity, ComponentId> entityComponent_;
    vector<ComponentHandle> components_;
    vector<Entity> added_;
    vector<Entity> removed_;
    vector<Entity> updated_;

    unordered_map<RenderHandle, refcnt_ptr<ReferencedProperties>> properties_;
};

#if _MSC_VER
// revert to old warnings. (re-enables the deprecation warning)
#pragma warning(pop)
#endif

MaterialComponentManager::MaterialComponentManager(IEcs& ecs) noexcept
    : ecs_(ecs), shaderManager_(GetInstance<IRenderContext>(
                     *ecs.GetClassFactory().GetInterface<IClassRegister>(), UID_RENDER_CONTEXT)
                                    ->GetDevice()
                                    .GetShaderManager()),
      renderHandleManager_(GetManager<IRenderHandleComponentManager>(ecs))
{
    // Initial reservation for 64 components/entities.
    // Will resize as needed.
    constexpr size_t initialComponentReserveSize = 64;
    components_.reserve(initialComponentReserveSize);
    entityComponent_.reserve(initialComponentReserveSize);

    auto result =
        properties_.insert_or_assign(RenderHandle {}, refcnt_ptr<ReferencedProperties>(new ReferencedProperties));
    auto& properties = result.first->second->properties;
    properties.insert(properties.end(), componentMetaData_.begin(), componentMetaData_.end());
    properties.insert(properties.end(), defaultMetaData_.begin(), defaultMetaData_.end());
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
    if (src->Owner() == this) {
        auto* h = static_cast<const ComponentHandle*>(src);
        return new ComponentHandle(const_cast<MaterialComponentManager*>(this), {}, h->data_);
    }
    return nullptr;
}

void MaterialComponentManager::Release(IPropertyHandle* dst) const
{
    if (dst) {
        if (dst->Owner() == this) {
            // we can only destroy things we "own" (know)
            auto* handle = static_cast<ComponentHandle*>(dst);
            if (auto id = GetComponentId(handle->entity_); id != IComponentManager::INVALID_COMPONENT_ID) {
                if (&components_[id] == handle) {
                    // This is one of the components (bound to an entity) so do nothing
                    return;
                }
            }
            delete handle;
        }
    }
}

uint64_t MaterialComponentManager::Type() const
{
    return typeHash_;
}

bool MaterialComponentManager::Clone(IPropertyHandle& dst, const IPropertyHandle& src) const
{
    if (&src == &dst) {
        return true;
    }
    if (src.Owner()->Type() != dst.Owner()->Type()) {
        return false;
    }
    if (src.Owner() != this) {
        return false;
    }
    bool result = false;
    void* wdata = dst.WLock();
    if (wdata) {
        const MaterialComponent* rdata = (const MaterialComponent*)src.RLock();
        if (rdata) {
            ::new (wdata) MaterialComponent(*rdata);
            result = true;
        }
        src.RUnlock();
    }
    dst.WUnlock();

    return result;
}

void MaterialComponentManager::Destroy(IPropertyHandle& dataHandle) const
{
    if (dataHandle.Owner() == this) {
        // call destructor.
        auto dstPtr = (MaterialComponent*)dataHandle.WLock();
        if (dstPtr) {
            dstPtr->~MaterialComponent();
        }
        dataHandle.WUnlock();
    }
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
    if (auto it = entityComponent_.find(entity); it != entityComponent_.end()) {
        return it->second;
    }
    return IComponentManager::INVALID_COMPONENT_ID;
}

void MaterialComponentManager::Create(Entity entity)
{
    CORE_ASSERT_MSG(CORE_NS::EntityUtil::IsValid(entity), "Invalid Entity");
    if (CORE_NS::EntityUtil::IsValid(entity)) {
        if (auto it = entityComponent_.find(entity); it == entityComponent_.end()) {
            entityComponent_.insert({ entity, static_cast<ComponentId>(components_.size()) });
            auto& component = components_.emplace_back(this, entity);
            added_.push_back(entity);
            modifiedFlags_ |= CORE_NS::CORE_COMPONENT_MANAGER_COMPONENT_ADDED_BIT;
            generationCounter_++;

            // lock/unlock for toggling component updated behavior.
            component.WLock();
            component.WUnlock();
        } else {
            if (auto dst = ScopedHandle<MaterialComponent>(&components_[it->second]); dst) {
                *dst = {};
            }
        }
    }
}

bool MaterialComponentManager::Destroy(Entity entity)
{
    CORE_ASSERT_MSG(CORE_NS::EntityUtil::IsValid(entity), "Invalid Entity");
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
        auto* it = &components_[id];
        // invalid entity.. if so clean garbage
        if (!CORE_NS::EntityUtil::IsValid(it->entity_)) {
            // find last valid and swap with it
            for (ComponentId rid = componentCount - 1; rid > id; rid--) {
                auto* rit = &components_[rid];
                // valid entity? if so swap the components.
                if (CORE_NS::EntityUtil::IsValid(rit->entity_)) {
                    // fix the entityComponent_ map (update the component id for the entity)
                    entityComponent_[rit->entity_] = id;
                    *it = move(*rit);
                    break;
                }
            }
            componentCount--;
            continue;
        }
        id++;
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
            if (auto dst = ScopedHandle<MaterialComponent>(&components_[it->second]); dst) {
                *dst = *src;
            }
        }
    }
}

const IPropertyHandle* MaterialComponentManager::GetData(Entity entity) const
{
    if (const auto it = entityComponent_.find(entity); it != entityComponent_.end()) {
        if (it->second < components_.size()) {
            return &components_[it->second];
        }
    }
    return nullptr;
}

IPropertyHandle* MaterialComponentManager::GetData(Entity entity)
{
    if (const auto it = entityComponent_.find(entity); it != entityComponent_.end()) {
        if (it->second < components_.size()) {
            return &components_[it->second];
        }
    }
    return nullptr;
}

void MaterialComponentManager::SetData(ComponentId index, const IPropertyHandle& dataHandle)
{
    CORE_ASSERT_MSG(index < components_.size(), "Invalid ComponentId");
    if (!IsMatchingHandle(dataHandle)) {
        // We could verify the metadata here.
        // And in copy only the matching properties one-by-one also.
        return;
    }
    if (index < components_.size()) {
        if (const auto src = ScopedHandle<const MaterialComponent>(&dataHandle); src) {
            if (auto dst = ScopedHandle<MaterialComponent>(&components_[index]); dst) {
                *dst = *src;
            }
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
    CORE_ASSERT_MSG(CORE_NS::EntityUtil::IsValid(entity), "Invalid Entity");
    if (CORE_NS::EntityUtil::IsValid(entity)) {
        if (auto handle = Write(entity); handle) {
            *handle = data;
        } else {
            entityComponent_.insert({ entity, static_cast<ComponentId>(components_.size()) });
            auto& component = components_.emplace_back(this, entity, data);
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
    if (dataHandle.Owner()->Type() == typeHash_) {
        return true;
    }
    return false;
}

// Custom properties implementation

// CustomProperties IPropertyHandle
const IPropertyApi* MaterialComponentManager::CustomProperties::Owner() const
{
    return this;
}

size_t MaterialComponentManager::CustomProperties::Size() const
{
    return data_.size();
}

const void* MaterialComponentManager::CustomProperties::RLock() const
{
    return data_.data();
}

void MaterialComponentManager::CustomProperties::RUnlock() const {}

void* MaterialComponentManager::CustomProperties::WLock()
{
    return data_.data();
}

void MaterialComponentManager::CustomProperties::WUnlock() {}

// CustomProperties IPropertyApi
size_t MaterialComponentManager::CustomProperties::PropertyCount() const
{
    return metaData_.size();
}

const Property* MaterialComponentManager::CustomProperties::MetaData(size_t index) const
{
    if (index < metaData_.size()) {
        return &metaData_[index];
    }

    return nullptr;
}

array_view<const Property> MaterialComponentManager::CustomProperties::MetaData() const
{
    return { metaData_ };
}

uint64_t MaterialComponentManager::CustomProperties::Type() const
{
    return 0;
}

IPropertyHandle* MaterialComponentManager::CustomProperties::Create() const
{
    return nullptr;
}

IPropertyHandle* MaterialComponentManager::CustomProperties::Clone(const IPropertyHandle* src) const
{
    return nullptr;
}

void MaterialComponentManager::CustomProperties::Release(IPropertyHandle* handle) const {}

bool MaterialComponentManager::CustomProperties::Clone(IPropertyHandle& dst, const IPropertyHandle& src) const
{
    return false;
}

void MaterialComponentManager::CustomProperties::Destroy(IPropertyHandle& dst) const {}

void MaterialComponentManager::CustomProperties::Reset()
{
    metaStrings_.clear();
    metaData_.clear();
    data_.clear();
}

void MaterialComponentManager::CustomProperties::Update(const CORE_NS::json::value& customProperties)
{
    for (const CORE_NS::json::value& propertyValue : customProperties.array_) {
        if (propertyValue.is_object()) {
            uintptr_t offset = 0U;
            for (const auto& object : propertyValue.object_) {
                if (object.key == SET) {
                    offset += AppendSet(offset, object.value);
                } else if (object.key == BINDING) {
                    offset += AppendBinding(offset, object.value);
                } else if (object.key == "data" && object.value.is_array()) {
                    metaStrings_.reserve(object.value.array_.size());
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

                            if (type == "vec4") {
                                offset += AppendVec4(name, displayName, offset, value);
                            } else if (type == "uvec4") {
                                offset += AppendUVec4(name, displayName, offset, value);
                            } else if (type == "float") {
                                offset += AppendFloat(name, displayName, offset, value);
                            } else if (type == "uint") {
                                offset += AppendUint(name, displayName, offset, value);
                            }
                        }
                    }
                }
            }
        }
    }
}

uintptr_t MaterialComponentManager::CustomProperties::AppendSet(uintptr_t offset, const CORE_NS::json::value& value)
{
    Property meta {
        SET,                                // name
        BASE_NS::CompileTime::FNV1aHash(SET.data()), // hash
        CORE_NS::PropertyType::UINT32_T,             // type
        1U,                                 // count
        sizeof(uint32_t),                   // size
        offset,                             // offset
        "",                                 // displayName
        0U,                                 // flags
        {},                                 // metaData
    };
    metaData_.push_back(meta);
    data_.resize(data_.size() + meta.size);
    if (value.is_number()) {
        *static_cast<uint32_t*>(static_cast<void*>(data_.data() + meta.offset)) = value.as_number<uint32_t>();
    }
    return meta.size;
}

uintptr_t MaterialComponentManager::CustomProperties::AppendBinding(uintptr_t offset, const CORE_NS::json::value& value)
{
    Property meta {
        BINDING,                                // name
        BASE_NS::CompileTime::FNV1aHash(BINDING.data()), // hash
        CORE_NS::PropertyType::UINT32_T,                 // type
        1U,                                     // count
        sizeof(uint32_t),                       // size
        offset,                                 // offset
        "",                                     // displayName
        0U,                                     // flags
        {},                                     // metaData
    };
    metaData_.push_back(meta);
    data_.resize(data_.size() + meta.size);
    if (value.is_number()) {
        *static_cast<uint32_t*>(static_cast<void*>(data_.data() + meta.offset)) = value.as_number<uint32_t>();
    }
    return meta.size;
}

uintptr_t MaterialComponentManager::CustomProperties::AppendVec4(
    string_view name, string_view displayName, uintptr_t offset, const CORE_NS::json::value* value)
{
    metaStrings_.push_back({ string { name }, string { displayName } });
    const auto& strings = metaStrings_.back();
    const Property meta {
        strings.name,            // name
        FNV1aHash(strings.name), // hash
        CORE_NS::PropertyType::VEC4_T,    // type
        1U,                      // count
        sizeof(BASE_NS::Math::Vec4),      // size
        offset,                  // offset
        strings.displayName,     // displayName
        0U,                      // flags
        {},                      // metaData
    };
    metaData_.push_back(meta);
    data_.resize(data_.size() + meta.size);
    if (value) {
        BASE_NS::Math::Vec4 vec4 {};
        if (value->is_array()) {
            const auto count = BASE_NS::Math::min(countof(vec4.data), value->array_.size());
            std::transform(value->array_.data(), value->array_.data() + count, vec4.data,
                [](const CORE_NS::json::value& value) { return value.is_number() ? value.as_number<float>() : 0.f; });
        } else if (value->is_number()) {
            vec4.data[0U] = value->as_number<float>();
        }
        *static_cast<BASE_NS::Math::Vec4*>(static_cast<void*>(data_.data() + meta.offset)) = vec4;
    }
    return meta.size;
}

uintptr_t MaterialComponentManager::CustomProperties::AppendUVec4(
    string_view name, string_view displayName, uintptr_t offset, const CORE_NS::json::value* value)
{
    metaStrings_.push_back({ string { name }, string { displayName } });
    const auto& strings = metaStrings_.back();
    const Property meta {
        strings.name,            // name
        FNV1aHash(strings.name), // hash
        CORE_NS::PropertyType::UVEC4_T,   // type
        1U,                      // count
        sizeof(BASE_NS::Math::UVec4),     // size
        offset,                  // offset
        strings.displayName,     // displayName
        0U,                      // flags
        {},                      // metaData
    };
    metaData_.push_back(meta);
    data_.resize(data_.size() + meta.size);
    if (value) {
        BASE_NS::Math::UVec4 vec4 {};
        if (value->is_array()) {
            const auto count = BASE_NS::Math::min(countof(vec4.data), value->array_.size());
            std::transform(value->array_.data(), value->array_.data() + count, vec4.data,
                [](const CORE_NS::json::value& value) { return value.is_number() ? value.as_number<uint32_t>() : 0U; });
        } else if (value->is_number()) {
            vec4.data[0U] = value->as_number<uint32_t>();
        }
        *static_cast<BASE_NS::Math::UVec4*>(static_cast<void*>(data_.data() + meta.offset)) = vec4;
    }
    return meta.size;
}

uintptr_t MaterialComponentManager::CustomProperties::AppendFloat(
    string_view name, string_view displayName, uintptr_t offset, const CORE_NS::json::value* value)
{
    metaStrings_.push_back({ string { name }, string { displayName } });
    const auto& strings = metaStrings_.back();
    const Property meta {
        strings.name,            // name
        FNV1aHash(strings.name), // hash
        CORE_NS::PropertyType::FLOAT_T,   // type
        1U,                      // count
        sizeof(float),           // size
        offset,                  // offset
        strings.displayName,     // displayName
        0U,                      // flags
        {},                      // metaData
    };
    metaData_.push_back(meta);
    data_.resize(data_.size() + meta.size);
    if (value && value->is_number()) {
        *static_cast<float*>(static_cast<void*>(data_.data() + meta.offset)) = value->as_number<float>();
    }
    return meta.size;
}

uintptr_t MaterialComponentManager::CustomProperties::AppendUint(
    string_view name, string_view displayName, uintptr_t offset, const CORE_NS::json::value* value)
{
    metaStrings_.push_back({ string { name }, string { displayName } });
    const auto& strings = metaStrings_.back();
    const Property meta {
        strings.name,            // name
        FNV1aHash(strings.name), // hash
        CORE_NS::PropertyType::UINT32_T,  // type
        1U,                      // count
        sizeof(uint32_t),        // size
        offset,                  // offset
        strings.displayName,     // displayName
        0U,                      // flags
        {},                      // metaData
    };
    metaData_.push_back(meta);
    data_.resize(data_.size() + meta.size);
    if (value && value->is_number()) {
        *static_cast<uint32_t*>(static_cast<void*>(data_.data() + meta.offset)) = value->as_number<uint32_t>();
    }
    return meta.size;
}

// Handle implementation
MaterialComponentManager::ComponentHandle::ComponentHandle(MaterialComponentManager* owner, Entity entity) noexcept
    : ComponentHandle(owner, entity, {})
{}

MaterialComponentManager::ComponentHandle::ComponentHandle(
    MaterialComponentManager* owner, Entity entity, const MaterialComponent& data) noexcept
    : manager_(owner), entity_(entity), data_(data)
{}

MaterialComponentManager::ComponentHandle::ComponentHandle(ComponentHandle&& other) noexcept
    : manager_(other.manager_), entity_(exchange(other.entity_, {})), cachedShader_(exchange(other.cachedShader_, {})),
      metaData_(exchange(other.metaData_, nullptr)), generation_(exchange(other.generation_, 0)),
      rLocked_(exchange(other.rLocked_, 0)), wLocked_(exchange(other.wLocked_, false)),
      data_(exchange(other.data_, {}))
{}

typename MaterialComponentManager::ComponentHandle& MaterialComponentManager::ComponentHandle::operator=(
    ComponentHandle&& other) noexcept
{
    if (this != &other) {
        CORE_ASSERT(manager_ == other.manager_);
        entity_ = exchange(other.entity_, {});
        cachedShader_ = exchange(other.cachedShader_, {});
        metaData_ = exchange(other.metaData_, nullptr);
        generation_ = exchange(other.generation_, 0);
        rLocked_ = exchange(other.rLocked_, 0);
        wLocked_ = exchange(other.wLocked_, false);
        data_ = exchange(other.data_, {});
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
    CORE_ASSERT(!wLocked_);
    rLocked_++;
    return &data_;
}

void MaterialComponentManager::ComponentHandle::RUnlock() const
{
    CORE_ASSERT(manager_);
    CORE_ASSERT(rLocked_ > 0);
    rLocked_--;
}

void* MaterialComponentManager::ComponentHandle::WLock()
{
    CORE_ASSERT(manager_);
    CORE_ASSERT(rLocked_ <= 1 && !wLocked_);
    wLocked_ = true;
    return &data_;
}

void MaterialComponentManager::ComponentHandle::WUnlock()
{
    CORE_ASSERT(manager_);
    CORE_ASSERT(wLocked_);
    wLocked_ = false;
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
    return manager_->Create();
}

IPropertyHandle* MaterialComponentManager::ComponentHandle::Clone(const IPropertyHandle* src) const
{
    return manager_->Clone(src);
}

void MaterialComponentManager::ComponentHandle::Release(IPropertyHandle* handle) const
{
    manager_->Release(handle);
}

bool MaterialComponentManager::ComponentHandle::Clone(IPropertyHandle& dst, const IPropertyHandle& src) const
{
    return false;
}

void MaterialComponentManager::ComponentHandle::Destroy(IPropertyHandle& dst) const
{
    // do nothing
}

// ComponentHandle
void MaterialComponentManager::ComponentHandle::UpdateMetadata() const
{
    if (!manager_->renderHandleManager_) {
        manager_->renderHandleManager_ = GetManager<IRenderHandleComponentManager>(manager_->ecs_);
        if (!manager_->renderHandleManager_) {
            return;
        }
    }
    auto currentShader = manager_->renderHandleManager_->GetRenderHandleReference(data_.materialShader.shader);
    if (!metaData_ || !(cachedShader_.GetHandle() == currentShader.GetHandle())) {
        cachedShader_ = currentShader;

        auto& propertyCache = manager_->properties_;
        if (auto cached = propertyCache.find(currentShader.GetHandle()); cached != propertyCache.end()) {
            metaData_ = cached->second;
        } else if (currentShader) {
            if (auto metaJson = manager_->shaderManager_.GetMaterialMetadata(currentShader);
                metaJson && metaJson->is_array()) {
                metaData_ = propertyCache
                                .insert({ currentShader.GetHandle(),
                                    refcnt_ptr<ReferencedProperties>(new ReferencedProperties) })
                                .first->second;

                auto& newProperties = metaData_->properties;
                newProperties.insert(newProperties.end(), componentMetaData_.begin(), componentMetaData_.end());

                // go through the metadata which should be an array of metadata for different component types
                if (auto material = FindMaterialComponentJson(*metaJson); material) {
                    AppendProperties(newProperties, *material, componentMetaData_);

                    if (const CORE_NS::json::value* propertiesJson = material->find("customProperties");
                        propertiesJson && propertiesJson->is_array()) {
                        metaData_->customProperties = propertiesJson;
                    }
                }
            }
        }

        custom_.Reset();
        if (metaData_ && metaData_->customProperties) {
            custom_.Update(*metaData_->customProperties);
            data_.customProperties = &custom_;
        } else {
            data_.customProperties = nullptr;
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
