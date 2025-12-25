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

#ifndef OHOS_3D_EFFECT_ETS_H
#define OHOS_3D_EFFECT_ETS_H

#include <string>

#include "SceneResourceETS.h"
#include "PropertyProxy.h"

namespace OHOS::Render3D {
class EffectETS : public SceneResourceETS {
public:
    explicit EffectETS(const SCENE_NS::IEffect::Ptr effect);
    ~EffectETS() override;
    void Destroy() override;
    META_NS::IObject::Ptr GetNativeObj() const override;

    bool GetEnabled();
    void SetEnabled(bool enabled);
    BASE_NS::string GetEffectId();

    SCENE_NS::IEffect::Ptr GetInternalEffect()
    {
        return effect_;
    }

    std::shared_ptr<IPropertyProxy> GetProperty(const std::string &key);

    template<typename Type>
    bool SetProperty(const std::string &key, const Type &value)
    {
        return ProxySetProperty(proxies_[key], value, key);
    }

protected:
    void AddProperties();

private:
    SCENE_NS::IEffect::Ptr effect_{nullptr};
    std::unordered_map<std::string, std::shared_ptr<IPropertyProxy>> proxies_;
    std::vector<std::string> keys_;
};

class EffectsContainerETS {
public:
    explicit EffectsContainerETS(META_NS::ArrayProperty<SCENE_NS::IEffect::Ptr> effects);
    ~EffectsContainerETS();

    size_t GetCount();
    std::shared_ptr<EffectETS> GetChild(const uint32_t index);

    void ClearChildren();
    void InsertChildAfter(
        const std::shared_ptr<EffectETS> &childEffect, const std::shared_ptr<EffectETS> &siblingEffect);
    void AppendChild(const std::shared_ptr<EffectETS> &childEffect);
    void RemoveChild(const std::shared_ptr<EffectETS> &childEffect);

private:
    META_NS::ArrayProperty<SCENE_NS::IEffect::Ptr> effects_;
};
} // namespace OHOS::Render3D
#endif  // OHOS_3D_EFFECT_ETS_H