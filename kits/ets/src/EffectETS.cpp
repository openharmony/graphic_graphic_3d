/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "EffectETS.h"

#include "RenderContextETS.h"
#include <scene/ext/util.h>

namespace OHOS::Render3D {
EffectsContainerETS::EffectsContainerETS(META_NS::ArrayProperty<SCENE_NS::IEffect::Ptr> effects) : effects_(effects)
{}

EffectsContainerETS::~EffectsContainerETS()
{}

size_t EffectsContainerETS::GetCount()
{
    return effects_ ? static_cast<uint32_t>(effects_->GetSize()) : 0u;
}

std::shared_ptr<EffectETS> EffectsContainerETS::GetChild(const uint32_t index)
{
    if (effects_) {
        if (auto effect = effects_->GetValueAt(index)) {
            return std::make_shared<EffectETS>(effect);
        }
    }
    return nullptr;
}

void EffectsContainerETS::ClearChildren()
{
    if (effects_) {
        effects_->Reset();
    }
}

void EffectsContainerETS::InsertChildAfter(
    const std::shared_ptr<EffectETS> &childEffect, const std::shared_ptr<EffectETS> &siblingEffect)
{
    if (!childEffect) {
        return;
    }
    if (!effects_) {
        return;
    }
    SCENE_NS::IEffect::Ptr child = childEffect->GetInternalEffect();
    SCENE_NS::IEffect::Ptr sibling = (siblingEffect == nullptr ? nullptr : siblingEffect->GetInternalEffect());
    size_t index = effects_->FindFirstValueOf(sibling);
    if (index == size_t(-1)) {
        return;
    }
    effects_->InsertValueAt(index + 1, child);
}

void EffectsContainerETS::AppendChild(const std::shared_ptr<EffectETS> &childEffect)
{
    if (!childEffect) {
        return;
    }
    if (!effects_) {
        return;
    }
    SCENE_NS::IEffect::Ptr child = childEffect->GetInternalEffect();
    effects_->AddValue(child);
}

void EffectsContainerETS::RemoveChild(const std::shared_ptr<EffectETS> &childEffect)
{
    if (!childEffect) {
        return;
    }
    if (!effects_) {
        return;
    }

    SCENE_NS::IEffect::Ptr child = childEffect->GetInternalEffect();
    effects_->RemoveAt(effects_->FindFirstValueOf(child));
}

EffectETS::EffectETS(const SCENE_NS::IEffect::Ptr effect)
    : SceneResourceETS(SceneResourceETS::SceneResourceType::EFFECT), effect_(effect)
{
    AddProperties();
}

EffectETS::~EffectETS()
{
    effect_.reset();
}

void EffectETS::Destroy()
{
    effect_.reset();
}

META_NS::IObject::Ptr EffectETS::GetNativeObj() const
{
    return interface_pointer_cast<META_NS::IObject>(effect_);
}

bool EffectETS::GetEnabled()
{
    if (!effect_) {
        return false;
    }
    bool enabled = META_NS::GetValue(effect_->Enabled());
    return enabled;
}

void EffectETS::SetEnabled(bool enabled)
{
    if (!effect_) {
        return;
    }
    META_NS::SetValue(effect_->Enabled(), enabled);
}

BASE_NS::string EffectETS::GetEffectId()
{
    if (!effect_) {
        return "";
    }
    BASE_NS::string id = effect_->GetEffectClassId().ToString();
    return id;
}

void EffectETS::AddProperties()
{
    auto meta =  interface_cast<META_NS::IMetadata>(effect_);
    if (!effect_ || !meta) {
        return;
    }

    keys_.clear();
    proxies_.clear();

    for (auto &&p : meta->GetProperties()) {
        if (auto proxy = PropertyToProxy(p)) {
            proxies_.insert_or_assign(SCENE_NS::PropertyName(p->GetName()).data(), std::move(proxy));
        }
    }
    keys_.reserve(proxies_.size());
    for (auto &pair : proxies_) {
        keys_.push_back(pair.first);
    }
}

std::shared_ptr<IPropertyProxy> EffectETS::GetProperty(const std::string &key)
{
    return proxies_[key];
}
} // namespace OHOS::Render3D