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

#ifndef OHOS_3D_EFFECT_IMPL_H
#define OHOS_3D_EFFECT_IMPL_H

#include "stdexcept"
#include "taihe/runtime.hpp"

#include "EffectETS.h"
#include "SceneResourceImpl.h"

namespace OHOS::Render3D::KITETS {
class EffectsContainerImpl {
public:
    explicit EffectsContainerImpl(const std::shared_ptr<EffectsContainerETS> EffectsContainerETS);
    ~EffectsContainerImpl();
    void append(::SceneResources::weak::Effect item);
    void insertAfter(::SceneResources::weak::Effect item, ::SceneNodes::EffectOrNull const &sibling);
    void remove(::SceneResources::weak::Effect item);
    ::SceneNodes::EffectOrNull get(int32_t index);
    void clear();
    int32_t count();
private:
    std::shared_ptr<EffectsContainerETS> effectsContainerETS_{nullptr};
};

class EffectImpl : public SceneResourceImpl {
public:
    static ::SceneNodes::EffectOrNull MakeEffectOrNull(const std::shared_ptr<EffectETS> &effectETS);

    explicit EffectImpl(const std::shared_ptr<EffectETS> effectETS);
    ~EffectImpl();
    bool getEnabled();
    void setEnabled(bool enabled);
    ::taihe::string getEffectId();
    ::SceneResources::EffectPropertyOutputValue getPropertyValue(::taihe::string_view key);
    bool setPropertyValue(::taihe::string_view key, ::SceneResources::EffectPropertyInputValue const &value);

    std::shared_ptr<EffectETS> GetInternalEffect()
    {
        return effectETS_;
    }

private:
    std::shared_ptr<EffectETS> effectETS_{nullptr};
};
} // namespace OHOS::Render3D::KITETS
#endif // OHOS_3D_EFFECT_IMPL_H