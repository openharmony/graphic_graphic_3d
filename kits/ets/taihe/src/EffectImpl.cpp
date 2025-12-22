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

#include "EffectImpl.h"

#include "ColorImpl.h"
#include "SceneResourceImpl.h"

namespace OHOS::Render3D::KITETS {
EffectsContainerImpl::EffectsContainerImpl(const std::shared_ptr<EffectsContainerETS> effectsContainerETS)
    : effectsContainerETS_(effectsContainerETS)
{}

EffectsContainerImpl::~EffectsContainerImpl()
{
    effectsContainerETS_.reset();
}

void EffectsContainerImpl::append(::SceneResources::weak::Effect item)
{
    if (!effectsContainerETS_) {
        return;
    }
    if (item.is_error()) {
        return;
    }
    auto effectOptional = static_cast<::SceneResources::weak::SceneResource>(item)->getImpl();
    if (!effectOptional.has_value()) {
        WIDGET_LOGE("invalid effect in taihe object");
        return;
    }
    auto effectImpl = reinterpret_cast<EffectImpl*>(effectOptional.value());
    if (effectImpl == nullptr) {
        return;
    }
    std::shared_ptr<EffectETS> itemEffect = effectImpl->GetInternalEffect();
    if (!itemEffect) {
        return;
    }
    effectsContainerETS_->AppendChild(itemEffect);
}

void EffectsContainerImpl::insertAfter(::SceneResources::weak::Effect item, ::SceneNodes::EffectOrNull const &sibling)
{
    if (!effectsContainerETS_) {
        return;
    }
    if (item.is_error()) {
        return;
    }
    auto effectOptional = static_cast<::SceneResources::weak::SceneResource>(item)->getImpl();
    if (!effectOptional.has_value()) {
        WIDGET_LOGE("invalid effect in taihe object");
        return;
    }
    auto effectImpl = reinterpret_cast<EffectImpl*>(effectOptional.value());
    if (effectImpl == nullptr) {
        return;
    }
    std::shared_ptr<EffectETS> itemEffect = effectImpl->GetInternalEffect();
    if (!itemEffect) {
        return;
    }
    std::shared_ptr<EffectETS> siblingEffect = nullptr;
    if (sibling.holds_effect()) {
        ::SceneResources::Effect effect = sibling.get_effect_ref();
        if (effect.is_error()) {
            return;
        }
        auto siblingEffectOptional = static_cast<::SceneResources::SceneResource>(effect)->getImpl();
        if (!siblingEffectOptional.has_value()) {
            WIDGET_LOGE("invalid effect in taihe object");
            return;
        }
        auto siblingEffectImpl = reinterpret_cast<EffectImpl*>(siblingEffectOptional.value());
        if (siblingEffectImpl != nullptr) {
            siblingEffect = siblingEffectImpl->GetInternalEffect();
        }
    }
    effectsContainerETS_->InsertChildAfter(itemEffect, siblingEffect);
}

void EffectsContainerImpl::remove(::SceneResources::weak::Effect item)
{
    if (!effectsContainerETS_) {
        return;
    }
    if (item.is_error()) {
        return;
    }
    auto effectOptional = static_cast<::SceneResources::weak::SceneResource>(item)->getImpl();
    if (!effectOptional.has_value()) {
        WIDGET_LOGE("invalid effect in taihe object");
        return;
    }
    auto effectImpl = reinterpret_cast<EffectImpl*>(effectOptional.value());
    if (effectImpl == nullptr) {
        WIDGET_LOGE("cast EffectImpl fail");
        return;
    }
    std::shared_ptr<EffectETS> itemEffect = effectImpl->GetInternalEffect();
    if (!itemEffect) {
        WIDGET_LOGE("GetInternalEffect fail");
        return;
    }
    effectsContainerETS_->RemoveChild(itemEffect);
}

::SceneNodes::EffectOrNull EffectsContainerImpl::get(int32_t index)
{
    if (!effectsContainerETS_) {
        return SceneNodes::EffectOrNull::make_nValue();
    }
    std::shared_ptr<EffectETS> effect = effectsContainerETS_->GetChild(index);
    return EffectImpl::MakeEffectOrNull(effect);
}

void EffectsContainerImpl::clear()
{
    if (effectsContainerETS_) {
        effectsContainerETS_->ClearChildren();
    }
}

int32_t EffectsContainerImpl::count()
{
    if (effectsContainerETS_) {
        return effectsContainerETS_->GetCount();
    }
    return 0;
}

::SceneNodes::EffectOrNull EffectImpl::MakeEffectOrNull(const std::shared_ptr<EffectETS> &effectETS)
{
    if (!effectETS) {
        return ::SceneNodes::EffectOrNull::make_nValue();
    }
    auto effect = taihe::make_holder<EffectImpl, ::SceneResources::Effect>(effectETS);
    return ::SceneNodes::EffectOrNull::make_effect(effect);
}

EffectImpl::EffectImpl(const std::shared_ptr<EffectETS> effectETS)
    : SceneResourceImpl(SceneResources::SceneResourceType::key_t::EFFECT, effectETS), effectETS_(effectETS)
{
    WIDGET_LOGD("EffectImpl ++");
}

EffectImpl::~EffectImpl()
{
    WIDGET_LOGD("EffectImpl --");
    effectETS_.reset();
}

bool EffectImpl::getEnabled()
{
    if (!effectETS_) {
        WIDGET_LOGE("empty EffectETS");
        return false;
    }
    return effectETS_->GetEnabled();
}

void EffectImpl::setEnabled(bool enabled)
{
    if (!effectETS_) {
        WIDGET_LOGE("empty EffectETS");
        return;
    }
    effectETS_->SetEnabled(enabled);
}

::taihe::string EffectImpl::getEffectId()
{
    if (!effectETS_) {
        WIDGET_LOGE("empty EffectETS");
        return "";
    }
    return effectETS_->GetEffectId().c_str();
}

::SceneResources::EffectPropertyOutputValue EffectImpl::getPropertyValue(::taihe::string_view key)
{
    if (!effectETS_) {
        WIDGET_LOGE("empty EffectETS");
        return ::SceneResources::EffectPropertyOutputValue::make_nValue();
    }
    std::shared_ptr<IPropertyProxy> propProxy = effectETS_->GetProperty(std::string(key));
    if (!propProxy) {
        WIDGET_LOGE("empty propProxy");
        return ::SceneResources::EffectPropertyOutputValue::make_nValue();
    }
    META_NS::IProperty::Ptr prop = propProxy->GetPropertyPtr();
    if (!prop) {
        WIDGET_LOGE("propProxy->GetPropertyPtr is null");
        return ::SceneResources::EffectPropertyOutputValue::make_nValue();
    }
    if (META_NS::IsCompatibleWith<bool>(prop)) {
        auto proxy = static_pointer_cast<PropertyProxy<bool>>(propProxy);
        ani_object obj = WrapBoolAsObj(proxy->GetValue());
        return ::SceneResources::EffectPropertyOutputValue::make_t_obj((uintptr_t)obj);
    }
    if (META_NS::IsCompatibleWith<int32_t>(prop)) {
        auto proxy = static_pointer_cast<PropertyProxy<int32_t>>(propProxy);
        ani_object obj = WrapIntAsObj(proxy->GetValue());
        return ::SceneResources::EffectPropertyOutputValue::make_t_obj((uintptr_t)obj);
    }
    if (META_NS::IsCompatibleWith<uint32_t>(prop)) {
        auto proxy = static_pointer_cast<PropertyProxy<uint32_t>>(propProxy);
        ani_object obj = WrapIntAsObj(proxy->GetValue());
        return ::SceneResources::EffectPropertyOutputValue::make_t_obj((uintptr_t)obj);
    }
    if (META_NS::IsCompatibleWith<float>(prop)) {
        auto proxy = static_pointer_cast<PropertyProxy<float>>(propProxy);
        ani_object obj = WrapDoubleAsObj(proxy->GetValue());
        return ::SceneResources::EffectPropertyOutputValue::make_t_obj((uintptr_t)obj);
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Color>(prop)) {
        BASE_NS::Color value = META_NS::GetValue(META_NS::Property<BASE_NS::Color>(prop));
        ::SceneTypes::Color result = ::taihe::make_holder<ColorImpl, ::SceneTypes::Color>(value);
        ani_object obj = WrapColorAsObj(result);
        return ::SceneResources::EffectPropertyOutputValue::make_t_obj((uintptr_t)obj);
    }
    auto any = META_NS::GetInternalAny(prop);
    WIDGET_LOGE("Unsupported property type [%{public}s] [%{public}s]",
        any ? any->GetTypeIdString().c_str() : "<Unknown>", key.data());
    return ::SceneResources::EffectPropertyOutputValue::make_nValue();
}

bool EffectImpl::setPropertyValue(::taihe::string_view key, ::SceneResources::EffectPropertyInputValue const &value)
{
    if (!effectETS_) {
        WIDGET_LOGE("empty EffectETS");
        return false;
    }
    const std::string name = std::string(key);
    if (value.holds_t_obj()) {
        ani_object obj = reinterpret_cast<ani_object>(value.get_t_obj_ref());
        AniObjectType type = HandleAniObject(obj);
        switch (type) {
            case AniObjectType::TYPE_INT:
                return effectETS_->SetProperty(name, static_cast<int32_t>(ParseObjToInt(obj)));
            case AniObjectType::TYPE_DOUBLE:
                return effectETS_->SetProperty(name, static_cast<float>(ParseObjToDouble(obj)));
            case AniObjectType::TYPE_BOOLEAN:
                return effectETS_->SetProperty(name, static_cast<bool>(ParseObjToBool(obj)));
            case AniObjectType::TYPE_COLOR:
                return effectETS_->SetProperty(name, ParseObjToColor(obj));
            default:
                WIDGET_LOGE("the property type is unavaliable");
                return false;
        }
    }
    return false;
}
} // namespace OHOS::Render3D::KITETS