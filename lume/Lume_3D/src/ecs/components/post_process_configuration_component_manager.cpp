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

#include <3d/ecs/components/post_process_configuration_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/type_traits.h>
#include <core/ecs/intf_ecs.h>
#include <core/intf_engine.h>
#include <render/datastore/intf_render_data_store_post_process.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_shader_manager.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"
#include "util/json_util.h"
#include "util/property_util.h"

#define IMPLEMENT_MANAGER
#include "PropertyTools/property_macros.h"

CORE_BEGIN_NAMESPACE()
using CORE3D_NS::PostProcessConfigurationComponent;

DECLARE_PROPERTY_TYPE(CORE_NS::IPropertyHandle*);
DECLARE_PROPERTY_TYPE(RENDER_NS::RenderHandle);
DECLARE_PROPERTY_TYPE(RENDER_NS::RenderHandleReference);
DECLARE_PROPERTY_TYPE(PostProcessConfigurationComponent::PostProcessEffect);
DECLARE_PROPERTY_TYPE(BASE_NS::vector<PostProcessConfigurationComponent::PostProcessEffect>);

BEGIN_METADATA(
    PostProcessConfigurationComponentPostProcessEffectMetaData, PostProcessConfigurationComponent::PostProcessEffect)
DECL_PROPERTY2(PostProcessConfigurationComponent::PostProcessEffect, name, "Effect Name", 0)
DECL_PROPERTY2(
    PostProcessConfigurationComponent::PostProcessEffect, globalUserFactorIndex, "Global User Factor Index", 0)
DECL_PROPERTY2(PostProcessConfigurationComponent::PostProcessEffect, shader, "Shader", 0)
DECL_PROPERTY2(PostProcessConfigurationComponent::PostProcessEffect, enabled, "Enabled", 0)
DECL_PROPERTY2(PostProcessConfigurationComponent::PostProcessEffect, flags, "Additional Non Typed Flags", 0)
DECL_PROPERTY2(PostProcessConfigurationComponent::PostProcessEffect, factor, "Global Factor", 0)
DECL_PROPERTY2(PostProcessConfigurationComponent::PostProcessEffect, customProperties, "Custom Properties", 0)
END_METADATA(
    PostProcessConfigurationComponentPostProcessEffectMetaData, PostProcessConfigurationComponent::PostProcessEffect)

// Needed to get containerMethods through MetaData() -> Property -> metaData -> containerMethods
DECLARE_CONTAINER_API(PPCC_PPE, PostProcessConfigurationComponent::PostProcessEffect);
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

namespace {
constexpr uint32_t CUSTOM_PROPERTY_POD_CONTAINER_BYTE_SIZE { PostProcessConstants::USER_LOCAL_FACTOR_BYTE_SIZE };
constexpr string_view CUSTOM_PROPERTIES = "customProperties";
constexpr string_view CUSTOM_PROPERTY_DATA = "data";
constexpr string_view NAME = "name";
constexpr string_view DISPLAY_NAME = "displayName";

void UpdateCustomPropertyMetadata(const json::value& customProperties, CustomPropertyPodContainer& properties)
{
    if (customProperties && customProperties.is_array()) {
        for (const auto& ref : customProperties.array_) {
            if (const auto customProps = ref.find(CUSTOM_PROPERTIES); customProps && customProps->is_array()) {
                // process custom properties i.e. local factors
                for (const auto& propRef : customProps->array_) {
                    if (const auto customData = propRef.find(CUSTOM_PROPERTY_DATA); customData) {
                        // reserve the property count
                        properties.ReservePropertyCount(customData->array_.size());
                        for (const auto& dataValue : customData->array_) {
                            if (dataValue.is_object()) {
                                string_view name;
                                string_view displayName;
                                string_view type;
                                const json::value* value = nullptr;
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

                                const PropertyTypeDecl typeDecl =
                                    CustomPropertyPodHelper::GetPropertyTypeDeclaration(type);
                                const size_t align = CustomPropertyPodHelper::GetPropertyTypeAlignment(typeDecl);
                                const size_t offset = [](size_t value, size_t align) -> size_t {
                                    if (align == 0U) {
                                        return value;
                                    }
                                    return ((value + align - 1U) / align) * align;
                                }(properties.GetByteSize(), align);
                                properties.AddOffsetProperty(name, displayName, offset, typeDecl);
                                CustomPropertyPodHelper::SetCustomPropertyBlobValue(
                                    typeDecl, value, properties, offset);
                            }
                        }
                    }
                }
            }
        }
    }
}
} // namespace

class PostProcessConfigurationComponentManager final : public IPostProcessConfigurationComponentManager,
                                                       public IPropertyApi {
    using ComponentId = IComponentManager::ComponentId;

public:
    explicit PostProcessConfigurationComponentManager(IEcs& ecs) noexcept;
    ~PostProcessConfigurationComponentManager() override;

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

    // IPostProcessConfigurationComponentManager
    PostProcessConfigurationComponent Get(ComponentId index) const override;
    PostProcessConfigurationComponent Get(Entity entity) const override;
    void Set(ComponentId index, const PostProcessConfigurationComponent& aData) override;
    void Set(Entity entity, const PostProcessConfigurationComponent& aData) override;
    ScopedHandle<const PostProcessConfigurationComponent> Read(ComponentId index) const override;
    ScopedHandle<const PostProcessConfigurationComponent> Read(Entity entity) const override;
    ScopedHandle<PostProcessConfigurationComponent> Write(ComponentId index) override;
    ScopedHandle<PostProcessConfigurationComponent> Write(Entity entity) override;

    // internal, non-public
    void Updated(Entity entity);

private:
    BEGIN_PROPERTY(PostProcessConfigurationComponent, componentProperties_)
#include <3d/ecs/components/post_process_configuration_component.h>
    END_PROPERTY();
    static constexpr array_view<const Property> componentMetaData_ { componentProperties_,
        countof(componentProperties_) };

    bool IsMatchingHandle(const IPropertyHandle& handle);

    static constexpr uint64_t typeHash_ =
        BASE_NS::CompileTime::FNV1aHash(CORE_NS::GetName<PostProcessConfigurationComponent>().data(),
            CORE_NS::GetName<PostProcessConfigurationComponent>().size());

    class ComponentHandle : public IPropertyHandle, IPropertyApi {
    public:
        ComponentHandle() = delete;
        ComponentHandle(PostProcessConfigurationComponentManager* owner, Entity entity) noexcept;
        ComponentHandle(PostProcessConfigurationComponentManager* owner, Entity entity,
            const PostProcessConfigurationComponent& data) noexcept;
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

        void UpdateMetadata();

        PostProcessConfigurationComponentManager* manager_ { nullptr };
        Entity entity_;
        uint32_t generation_ { 0u };
        bool dirty_ { false };
        mutable std::atomic_int32_t rLocked_ { 0 };
        mutable bool wLocked_ { false };
        PostProcessConfigurationComponent data_;

        // shader per single post process
        vector<Entity> effectShaders_;
        // custom property blob per single post process shader
        vector<unique_ptr<CustomPropertyPodContainer>> customProperties_;
    };

    IEcs& ecs_;
    IShaderManager& shaderManager_;
    IRenderHandleComponentManager* renderHandleManager_ { nullptr };

    uint32_t generationCounter_ { 0u };
    uint32_t modifiedFlags_ { 0u };
    unordered_map<Entity, ComponentId> entityComponent_;
    vector<ComponentHandle> components_;
    BASE_NS::vector<Entity> added_;
    BASE_NS::vector<Entity> removed_;
    BASE_NS::vector<Entity> updated_;
};

PostProcessConfigurationComponentManager::PostProcessConfigurationComponentManager(IEcs& ecs) noexcept
    : ecs_(ecs), shaderManager_(GetInstance<IRenderContext>(
                     *ecs.GetClassFactory().GetInterface<IClassRegister>(), UID_RENDER_CONTEXT)
                                    ->GetDevice()
                                    .GetShaderManager()),
      renderHandleManager_(GetManager<IRenderHandleComponentManager>(ecs))
{}

PostProcessConfigurationComponentManager::~PostProcessConfigurationComponentManager() = default;

// IPropertyApi
size_t PostProcessConfigurationComponentManager::PropertyCount() const
{
    return componentMetaData_.size();
}

const Property* PostProcessConfigurationComponentManager::MetaData(size_t index) const
{
    if (index < componentMetaData_.size()) {
        return &componentMetaData_[index];
    }
    return nullptr;
}

array_view<const Property> PostProcessConfigurationComponentManager::MetaData() const
{
    return componentMetaData_;
}

IPropertyHandle* PostProcessConfigurationComponentManager::Create() const
{
    return new ComponentHandle(const_cast<PostProcessConfigurationComponentManager*>(this), {}, {});
}

IPropertyHandle* PostProcessConfigurationComponentManager::Clone(const IPropertyHandle* src) const
{
    if (src) {
        auto owner = src->Owner();
        if (owner == this) {
            auto* h = static_cast<const ComponentHandle*>(src);
            return new ComponentHandle(const_cast<PostProcessConfigurationComponentManager*>(this), {}, h->data_);
        } else if (owner) {
            return owner->Clone(src);
        }
    }
    return nullptr;
}

void PostProcessConfigurationComponentManager::Release(IPropertyHandle* dst) const
{
    if (dst) {
        auto owner = dst->Owner();
        if (owner == this) {
            // we can only destroy things we "own" (know)
            auto* handle = static_cast<ComponentHandle*>(dst);
            if (auto id = GetComponentId(handle->entity_); id != IComponentManager::INVALID_COMPONENT_ID) {
                if (&components_[id] == handle) {
                    // This is one of the components (bound to an entity) so do nothing
                    return;
                }
            }
            delete handle;
        } else if (owner) {
            owner->Release(dst);
        }
    }
}

uint64_t PostProcessConfigurationComponentManager::Type() const
{
    return typeHash_;
}

// IComponentManager
string_view PostProcessConfigurationComponentManager::GetName() const
{
    constexpr auto name = CORE_NS::GetName<PostProcessConfigurationComponent>();
    return name;
}

Uid PostProcessConfigurationComponentManager::GetUid() const
{
    return IPostProcessConfigurationComponentManager::UID;
}

size_t PostProcessConfigurationComponentManager::GetComponentCount() const
{
    return components_.size();
}

const IPropertyApi& PostProcessConfigurationComponentManager::GetPropertyApi() const
{
    return *this;
}

Entity PostProcessConfigurationComponentManager::GetEntity(ComponentId index) const
{
    if (index < components_.size()) {
        return components_[index].entity_;
    }
    return Entity();
}

uint32_t PostProcessConfigurationComponentManager::GetComponentGeneration(ComponentId index) const
{
    if (index < components_.size()) {
        return components_[index].generation_;
    }
    return 0;
}

bool PostProcessConfigurationComponentManager::HasComponent(Entity entity) const
{
    return GetComponentId(entity) != IComponentManager::INVALID_COMPONENT_ID;
}

IComponentManager::ComponentId PostProcessConfigurationComponentManager::GetComponentId(Entity entity) const
{
    if (auto it = entityComponent_.find(entity); it != entityComponent_.end()) {
        return it->second;
    }
    return IComponentManager::INVALID_COMPONENT_ID;
}

void PostProcessConfigurationComponentManager::Create(Entity entity)
{
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
            if (auto dst = ScopedHandle<PostProcessConfigurationComponent>(&components_[it->second]); dst) {
                *dst = {};
            }
        }
    }
}

bool PostProcessConfigurationComponentManager::Destroy(Entity entity)
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

void PostProcessConfigurationComponentManager::Gc()
{
    const bool hasRemovedComponents = modifiedFlags_ & CORE_NS::CORE_COMPONENT_MANAGER_COMPONENT_REMOVED_BIT;
    if (!hasRemovedComponents) {
        return;
    }
    ComponentId componentCount = static_cast<ComponentId>(components_.size());
    for (ComponentId id = 0; id < componentCount;) {
        auto* it = &components_[id];
        // invalid entity, if so clean garbage
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

void PostProcessConfigurationComponentManager::Destroy(const array_view<const Entity> gcList)
{
    for (const Entity e : gcList) {
        Destroy(e);
    }
}

vector<Entity> PostProcessConfigurationComponentManager::GetAddedComponents()
{
    return BASE_NS::move(added_);
}

vector<Entity> PostProcessConfigurationComponentManager::GetRemovedComponents()
{
    return BASE_NS::move(removed_);
}

vector<Entity> PostProcessConfigurationComponentManager::GetUpdatedComponents()
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

CORE_NS::ComponentManagerModifiedFlags PostProcessConfigurationComponentManager::GetModifiedFlags() const
{
    return modifiedFlags_ & ~MODIFIED;
}

void PostProcessConfigurationComponentManager::ClearModifiedFlags()
{
    modifiedFlags_ &= MODIFIED;
}

uint32_t PostProcessConfigurationComponentManager::GetGenerationCounter() const
{
    return generationCounter_;
}

void PostProcessConfigurationComponentManager::SetData(Entity entity, const IPropertyHandle& dataHandle)
{
    if (!IsMatchingHandle(dataHandle)) {
        return;
    }
    if (const auto src = ScopedHandle<const PostProcessConfigurationComponent>(&dataHandle); src) {
        if (const auto it = entityComponent_.find(entity); it != entityComponent_.end()) {
            if (auto dst = ScopedHandle<PostProcessConfigurationComponent>(&components_[it->second]); dst) {
                *dst = *src;
            }
        }
    }
}

const IPropertyHandle* PostProcessConfigurationComponentManager::GetData(Entity entity) const
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

IPropertyHandle* PostProcessConfigurationComponentManager::GetData(Entity entity)
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

void PostProcessConfigurationComponentManager::SetData(ComponentId index, const IPropertyHandle& dataHandle)
{
    if (!IsMatchingHandle(dataHandle)) {
        // We could verify the metadata here.
        // And in copy only the matching properties one-by-one also.
        return;
    }
    if (index < components_.size()) {
        if (const auto src = ScopedHandle<const PostProcessConfigurationComponent>(&dataHandle); src) {
            if (auto dst = ScopedHandle<PostProcessConfigurationComponent>(&components_[index]); dst) {
                *dst = *src;
            }
        }
    }
}

const IPropertyHandle* PostProcessConfigurationComponentManager::GetData(ComponentId index) const
{
    if (index < components_.size()) {
        return &components_[index];
    }
    return nullptr;
}

IPropertyHandle* PostProcessConfigurationComponentManager::GetData(ComponentId index)
{
    if (index < components_.size()) {
        return &components_[index];
    }
    return nullptr;
}

IEcs& PostProcessConfigurationComponentManager::GetEcs() const
{
    return ecs_;
}

// IPostProcessConfigurationComponentManager
PostProcessConfigurationComponent PostProcessConfigurationComponentManager::Get(ComponentId index) const
{
    if (auto handle = Read(index); handle) {
        return *handle;
    }
    return PostProcessConfigurationComponent {};
}

PostProcessConfigurationComponent PostProcessConfigurationComponentManager::Get(Entity entity) const
{
    if (auto handle = Read(entity); handle) {
        return *handle;
    }
    return PostProcessConfigurationComponent {};
}

void PostProcessConfigurationComponentManager::Set(ComponentId index, const PostProcessConfigurationComponent& data)
{
    if (auto handle = Write(index); handle) {
        *handle = data;
    }
}

void PostProcessConfigurationComponentManager::Set(Entity entity, const PostProcessConfigurationComponent& data)
{
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

ScopedHandle<const PostProcessConfigurationComponent> PostProcessConfigurationComponentManager::Read(
    ComponentId index) const
{
    return ScopedHandle<const PostProcessConfigurationComponent> { GetData(index) };
}

ScopedHandle<const PostProcessConfigurationComponent> PostProcessConfigurationComponentManager::Read(
    Entity entity) const
{
    return ScopedHandle<const PostProcessConfigurationComponent> { GetData(entity) };
}

ScopedHandle<PostProcessConfigurationComponent> PostProcessConfigurationComponentManager::Write(ComponentId index)
{
    return ScopedHandle<PostProcessConfigurationComponent> { GetData(index) };
}

ScopedHandle<PostProcessConfigurationComponent> PostProcessConfigurationComponentManager::Write(Entity entity)
{
    return ScopedHandle<PostProcessConfigurationComponent> { GetData(entity) };
}

// Internal
void PostProcessConfigurationComponentManager::Updated(Entity entity)
{
    CORE_ASSERT_MSG(CORE_NS::EntityUtil::IsValid(entity), "Invalid ComponentId, bound to INVALID_ENTITY");
    modifiedFlags_ |= CORE_NS::CORE_COMPONENT_MANAGER_COMPONENT_UPDATED_BIT | MODIFIED;
    generationCounter_++;
}

bool PostProcessConfigurationComponentManager::IsMatchingHandle(const IPropertyHandle& dataHandle)
{
    if (dataHandle.Owner() == this) {
        return true;
    }
    if (dataHandle.Owner()->Type() == typeHash_) {
        return true;
    }
    return false;
}

// Handle implementation
PostProcessConfigurationComponentManager::ComponentHandle::ComponentHandle(
    PostProcessConfigurationComponentManager* owner, Entity entity) noexcept
    : ComponentHandle(owner, entity, {})
{}

PostProcessConfigurationComponentManager::ComponentHandle::ComponentHandle(
    PostProcessConfigurationComponentManager* owner, Entity entity,
    const PostProcessConfigurationComponent& data) noexcept
    : manager_(owner), entity_(entity), data_(data)
{}

PostProcessConfigurationComponentManager::ComponentHandle::ComponentHandle(ComponentHandle&& other) noexcept
    : manager_(other.manager_), entity_(exchange(other.entity_, {})), generation_(exchange(other.generation_, 0u)),
      rLocked_(other.rLocked_.exchange(0)), wLocked_(exchange(other.wLocked_, false)), data_(exchange(other.data_, {})),
      customProperties_(exchange(other.customProperties_, {}))
{}

typename PostProcessConfigurationComponentManager::ComponentHandle&
PostProcessConfigurationComponentManager::ComponentHandle::operator=(ComponentHandle&& other) noexcept
{
    if (this != &other) {
        CORE_ASSERT(manager_ == other.manager_);
        entity_ = exchange(other.entity_, {});
        generation_ = exchange(other.generation_, 0u);
        dirty_ = exchange(other.dirty_, false);
        rLocked_ = other.rLocked_.exchange(0);
        wLocked_ = exchange(other.wLocked_, false);
        data_ = exchange(other.data_, {});
        effectShaders_ = exchange(other.effectShaders_, {});
        customProperties_ = exchange(other.customProperties_, {});
    }
    return *this;
}

// ComponentHandle IPropertyHandle
const IPropertyApi* PostProcessConfigurationComponentManager::ComponentHandle::Owner() const
{
    return this;
}

size_t PostProcessConfigurationComponentManager::ComponentHandle::Size() const
{
    return sizeof(PostProcessConfigurationComponent);
}

const void* PostProcessConfigurationComponentManager::ComponentHandle::RLock() const
{
    CORE_ASSERT(manager_);
    CORE_ASSERT(!wLocked_);
    rLocked_++;
    return &data_;
}

void PostProcessConfigurationComponentManager::ComponentHandle::RUnlock() const
{
    CORE_ASSERT(manager_);
    CORE_ASSERT(rLocked_ > 0);
    rLocked_--;
}

void* PostProcessConfigurationComponentManager::ComponentHandle::WLock()
{
    CORE_ASSERT(manager_);
    CORE_ASSERT(rLocked_ <= 1 && !wLocked_);
    wLocked_ = true;
    return &data_;
}

void PostProcessConfigurationComponentManager::ComponentHandle::WUnlock()
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
size_t PostProcessConfigurationComponentManager::ComponentHandle::PropertyCount() const
{
    return manager_->PropertyCount();
}

const Property* PostProcessConfigurationComponentManager::ComponentHandle::MetaData(size_t index) const
{
    return manager_->MetaData(index);
}

array_view<const Property> PostProcessConfigurationComponentManager::ComponentHandle::MetaData() const
{
    return manager_->MetaData();
}

uint64_t PostProcessConfigurationComponentManager::ComponentHandle::Type() const
{
    return manager_->Type();
}

IPropertyHandle* PostProcessConfigurationComponentManager::ComponentHandle::Create() const
{
    return new ComponentHandle(manager_, {}, {});
}

IPropertyHandle* PostProcessConfigurationComponentManager::ComponentHandle::Clone(const IPropertyHandle* src) const
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

void PostProcessConfigurationComponentManager::ComponentHandle::Release(IPropertyHandle* handle) const
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

void PostProcessConfigurationComponentManager::ComponentHandle::UpdateMetadata()
{
    if (!manager_->renderHandleManager_) {
        if (manager_->renderHandleManager_ = GetManager<IRenderHandleComponentManager>(manager_->ecs_);
            !manager_->renderHandleManager_) {
            return;
        }
    }
    // resize for all single post processes
    effectShaders_.resize(data_.postProcesses.size());
    customProperties_.resize(data_.postProcesses.size());
    // loop through all post process effects for new shaders
    // NOTE: could cache shaders and/or check which single post processes are actually changed
    for (size_t idx = 0; idx < data_.postProcesses.size(); ++idx) {
        if (effectShaders_[idx] != data_.postProcesses[idx].shader) {
            effectShaders_[idx] = data_.postProcesses[idx].shader;
            const auto currentShader = manager_->renderHandleManager_->GetRenderHandleReference(effectShaders_[idx]);

            auto newPod = make_unique<CustomPropertyPodContainer>(CUSTOM_PROPERTY_POD_CONTAINER_BYTE_SIZE);
            if (currentShader) {
                if (const json::value* metaJson = manager_->shaderManager_.GetMaterialMetadata(currentShader);
                    metaJson && metaJson->is_array()) {
                    UpdateCustomPropertyMetadata(*metaJson, *newPod);
                }
            }

            auto& properties = customProperties_[idx];
            if (properties) {
                newPod->CopyValues(*properties);
            }
            properties = BASE_NS::move(newPod);
        }

        // re-fetch the property always
        data_.postProcesses[idx].customProperties = customProperties_[idx].get();
    }
}

//

IComponentManager* IPostProcessConfigurationComponentManagerInstance(IEcs& ecs)
{
    return new PostProcessConfigurationComponentManager(ecs);
}

void IPostProcessConfigurationComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<PostProcessConfigurationComponentManager*>(instance);
}

CORE3D_END_NAMESPACE()
