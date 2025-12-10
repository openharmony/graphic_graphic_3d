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
#include "sampler.h"

#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_render_resource.h>

#include <3d/ecs/components/render_handle_component.h>
#include <render/device/intf_gpu_resource_manager.h>

SCENE_BEGIN_NAMESPACE()

static constexpr BASE_NS::string_view MAG_FILTER_PROPERTY = "MagFilter";
static constexpr BASE_NS::string_view MIN_FILTER_PROPERTY = "MinFilter";
static constexpr BASE_NS::string_view MIP_MAP_MODE_PROPERTY = "MipMapMode";
static constexpr BASE_NS::string_view ADDRESS_MODE_U_PROPERTY = "AddressModeU";
static constexpr BASE_NS::string_view ADDRESS_MODE_V_PROPERTY = "AddressModeV";
static constexpr BASE_NS::string_view ADDRESS_MODE_W_PROPERTY = "AddressModeW";
static constexpr BASE_NS::string_view DEFAULT_SAMPLER_NAME = "CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_REPEAT";

template<typename T, typename U>
bool SetSamplerValue(META_NS::IProperty* p, const U& f, bool setToDefault = false)
{
    if (p) {
        auto value = META_NS::Any<T>(static_cast<T>(f));
        if (!setToDefault) {
            p->SetValue(value);
        } else if (auto sp = interface_cast<META_NS::IStackProperty>(p)) {
            sp->SetDefaultValue(value);
        }
        return true;
    }
    return false;
}

template<typename T, typename U>
bool InitializeSamplerValue(META_NS::IProperty* p, const U& value, const U& defaultValue, bool setValue)
{
    auto success = SetSamplerValue<T, U>(p, defaultValue, true); // Set default
    if (setValue) {
        success &= SetSamplerValue<T, U>(p, value, false); // Set value if required
    }
    return success;
}

template<typename T, typename U>
bool GetSamplerValue(const META_NS::IProperty* p, U& value)
{
    if (p && p->IsValueSet()) {
        // Value has been set
        T v {};
        p->GetValue().GetValue(v);
        if (value != static_cast<U>(v)) {
            value = static_cast<U>(v);
            return true;
        }
    }
    return false;
}

bool ResetSamplerProperty(META_NS::IProperty* p)
{
    if (p && p->IsValueSet()) {
        p->ResetValue();
        return true;
    }
    return false;
}

META_NS::IProperty* Sampler::GetExistingProperty(BASE_NS::string_view name)
{
    return GetProperty(name, META_NS::MetadataQuery::EXISTING).get();
}

const META_NS::IProperty* Sampler::GetExistingProperty(BASE_NS::string_view name) const
{
    return GetProperty(name, META_NS::MetadataQuery::EXISTING).get();
}

void Sampler::NotifyChanged()
{
    // Only notify if we don't have a set operation in progress
    if (CanNotifyChanged()) {
        // Since some property changed our sampler handle is not valid anymore, we need to create a new one next time
        // GetHandleFromSampler is called
        sampler_ = {};
        if (auto ev = EventOnResourceChanged(META_NS::MetadataQuery::EXISTING)) {
            META_NS::Invoke<META_NS::IOnChanged>(ev);
        }
    }
}

bool Sampler::CanNotifyChanged()
{
    CORE_NS::UniqueLock lock(transaction_);
    if (!setting_) {
        return true; // No transaction ongoing so can notify
    }
    // Set flag that we had a changed event during transaction
    changedDuringTransaction_ = true;
    return false;
}

void Sampler::StartTransaction()
{
    CORE_NS::UniqueLock lock(transaction_);
    setting_++;
}

void Sampler::EndTransaction(bool changed)
{
    bool shouldNotify = false;
    {
        CORE_NS::UniqueLock lock(transaction_);
        setting_--;
        if (!setting_) {
            // Transaction ended
            shouldNotify = changed && changedDuringTransaction_;
            changedDuringTransaction_ = false;
        }
    }
    if (shouldNotify) {
        NotifyChanged();
    }
}

void Sampler::ResetObject()
{
    auto task = [&]() -> bool {
        StartTransaction();
        bool changed = ResetSamplerProperty(GetExistingProperty(MAG_FILTER_PROPERTY));
        changed |= ResetSamplerProperty(GetExistingProperty(MIN_FILTER_PROPERTY));
        changed |= ResetSamplerProperty(GetExistingProperty(MIP_MAP_MODE_PROPERTY));
        changed |= ResetSamplerProperty(GetExistingProperty(ADDRESS_MODE_U_PROPERTY));
        changed |= ResetSamplerProperty(GetExistingProperty(ADDRESS_MODE_V_PROPERTY));
        changed |= ResetSamplerProperty(GetExistingProperty(ADDRESS_MODE_W_PROPERTY));
        EndTransaction(false); // Delay notification about change
        return changed;
    };
    auto scene = scene_.lock();
    auto changed = scene ? scene->RunDirectlyOrInTask(task) : task();
    if (changed) {
        NotifyChanged();
    }
}

void Sampler::SetScene(const IInternalScene::Ptr& scene)
{
    scene_ = scene;
}

void Sampler::OnPropertyChanged(const META_NS::IProperty&)
{
    NotifyChanged();
}

void Sampler::OnMetadataConstructed(const META_NS::StaticMetadata& m, CORE_NS::IInterface& i)
{
    if (!sampler_.handle || !scene_.lock()) {
        // Don't even try to set the values if we are in non-initialized state
        return;
    }
    if (auto p = interface_cast<META_NS::IProperty>(&i)) {
        const auto& desc = sampler_.descriptor;
        const auto& defaultDesc = sampler_.isDefault ? sampler_.descriptor : GetDefaultSampler().descriptor;
        const auto usingDefault = sampler_.isDefault;
        BASE_NS::string_view name(m.name);
        if (name == MAG_FILTER_PROPERTY) {
            InitializeSamplerValue<SamplerFilter>(p, desc.magFilter, defaultDesc.magFilter, !usingDefault);
        } else if (name == MIN_FILTER_PROPERTY) {
            InitializeSamplerValue<SamplerFilter>(p, desc.minFilter, defaultDesc.minFilter, !usingDefault);
        } else if (name == MIP_MAP_MODE_PROPERTY) {
            InitializeSamplerValue<SamplerFilter>(p, desc.mipMapMode, defaultDesc.mipMapMode, !usingDefault);
        } else if (name == ADDRESS_MODE_U_PROPERTY) {
            InitializeSamplerValue<SamplerAddressMode>(p, desc.addressModeU, defaultDesc.addressModeU, !usingDefault);
        } else if (name == ADDRESS_MODE_V_PROPERTY) {
            InitializeSamplerValue<SamplerAddressMode>(p, desc.addressModeV, defaultDesc.addressModeV, !usingDefault);
        } else if (name == ADDRESS_MODE_W_PROPERTY) {
            InitializeSamplerValue<SamplerAddressMode>(p, desc.addressModeW, defaultDesc.addressModeW, !usingDefault);
        }
    }
}

Sampler::SamplerInfo Sampler::GetDefaultSampler() const
{
    SamplerInfo info;
    if (auto scene = scene_.lock()) {
        auto& mgr = scene->GetRenderContext().GetDevice().GetGpuResourceManager();
        info.handle = mgr.GetSamplerHandle(DEFAULT_SAMPLER_NAME);
        info.descriptor = mgr.GetSamplerDescriptor(info.handle);
        info.isDefault = true;
    }
    return info;
}

RENDER_NS::RenderHandleReference Sampler::GetSamplerHandle(const SamplerInfo& info) const noexcept
{
    return info.isDefault ? RENDER_NS::RenderHandleReference {} : info.handle;
}

bool Sampler::UpdateSamplerFromHandle(
    RENDER_NS::IRenderContext& context, const RENDER_NS::RenderHandleReference& handle)
{
    if (sampler_.handle && handle.GetHandle() == sampler_.handle.GetHandle()) {
        // Same
        return false;
    }
    if (handle) {
        // Valid handle
        if (handle.GetHandleType() != RENDER_NS::RenderHandleType::GPU_SAMPLER) {
            CORE_LOG_E("Sampler requires GPU_SAMPLER handle");
            return false;
        }
        sampler_.handle = handle;
        sampler_.descriptor = context.GetDevice().GetGpuResourceManager().GetSamplerDescriptor(handle);
        sampler_.isDefault = false;
    } else {
        // Null handle, initialize from engine default sampler
        sampler_ = GetDefaultSampler();
    }
    auto& desc = sampler_.descriptor;

    // Update properties we have initialized
    StartTransaction();
    bool changed = SetSamplerValue<SamplerFilter>(GetExistingProperty(MAG_FILTER_PROPERTY), desc.magFilter);
    changed |= SetSamplerValue<SamplerFilter>(GetExistingProperty(MIN_FILTER_PROPERTY), desc.minFilter);
    changed |= SetSamplerValue<SamplerFilter>(GetExistingProperty(MIP_MAP_MODE_PROPERTY), desc.mipMapMode);
    changed |= SetSamplerValue<SamplerAddressMode>(GetExistingProperty(ADDRESS_MODE_U_PROPERTY), desc.addressModeU);
    changed |= SetSamplerValue<SamplerAddressMode>(GetExistingProperty(ADDRESS_MODE_V_PROPERTY), desc.addressModeV);
    changed |= SetSamplerValue<SamplerAddressMode>(GetExistingProperty(ADDRESS_MODE_W_PROPERTY), desc.addressModeW);
    EndTransaction(changed);
    return false;
}

RENDER_NS::RenderHandleReference Sampler::GetHandleFromSampler(RENDER_NS::IRenderContext& context) const
{
    if (sampler_.handle) {
        // We already have a handle
        return GetSamplerHandle(sampler_);
    }
    // No handle, create a new sampler based on our own property values, using the default sampler as base, replacing
    // values from it from our internal property values
    auto info = GetDefaultSampler();
    auto& desc = info.descriptor;
    auto isset = GetSamplerValue<SamplerFilter>(GetExistingProperty(MAG_FILTER_PROPERTY), desc.magFilter);
    isset |= GetSamplerValue<SamplerFilter>(GetExistingProperty(MIN_FILTER_PROPERTY), desc.minFilter);
    isset |= GetSamplerValue<SamplerFilter>(GetExistingProperty(MIP_MAP_MODE_PROPERTY), desc.mipMapMode);
    isset |= GetSamplerValue<SamplerAddressMode>(GetExistingProperty(ADDRESS_MODE_U_PROPERTY), desc.addressModeU);
    isset |= GetSamplerValue<SamplerAddressMode>(GetExistingProperty(ADDRESS_MODE_V_PROPERTY), desc.addressModeV);
    isset |= GetSamplerValue<SamplerAddressMode>(GetExistingProperty(ADDRESS_MODE_W_PROPERTY), desc.addressModeW);

    // If nothing changed from the default sampler's descriptor just use the default, otherwise create a new one
    sampler_.isDefault = !isset;
    sampler_.handle = sampler_.isDefault ? info.handle : context.GetDevice().GetGpuResourceManager().Create(desc);
    sampler_.descriptor = BASE_NS::move(desc);
    return GetSamplerHandle(sampler_);
}

SCENE_END_NAMESPACE()
