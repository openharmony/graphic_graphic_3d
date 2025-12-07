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

#ifndef SCENE_SRC_EFFECT_H
#define SCENE_SRC_EFFECT_H

#include <scene/ext/intf_internal_scene.h>
#include <scene/interface/intf_effect.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/resource/types.h>

#include <meta/ext/object.h>
#include <meta/ext/resource/resource.h>
#include <meta/interface/engine/intf_engine_value_manager.h>

SCENE_BEGIN_NAMESPACE()

class Effect : public META_NS::IntroduceInterfaces<META_NS::MetaObject, IEffect, META_NS::IEnginePropertySync> {
    META_OBJECT(Effect, SCENE_NS::ClassId::Effect, IntroduceInterfaces)

public:
    bool Build(const META_NS::IMetadata::Ptr& data) override;
    void Destroy() override;

    BASE_NS::string GetName() const override;

public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(IEffect, bool, Enabled, true)
    META_END_STATIC_DATA()

public: // IEffect
    META_IMPLEMENT_PROPERTY(bool, Enabled)

    Future<bool> InitializeEffect(const BASE_NS::shared_ptr<IScene>& scene, META_NS::ObjectId effectClassId) override;
    META_NS::ObjectId GetEffectClassId() const override;
    RENDER_NS::IRenderPostProcess::Ptr GetEffect() const override;

public: // IEnginePropertySync
    void SyncProperties() override;

private:
    bool CreateEffect();
    void PopulateProperties();
    void AddProperties(BASE_NS::array_view<const META_NS::IEngineValue::Ptr> values);
    void AddPropertyUpdateHook(const META_NS::IProperty::Ptr& prop);

    BASE_NS::Uid effectClassId_ {};
    mutable RENDER_NS::IRenderPostProcess::Ptr pp_;
    mutable META_NS::IEngineValueManager::Ptr valueManager_;
    META_NS::IOnChanged::InterfaceTypePtr syncPropertiesCallable_;
    mutable IInternalScene::WeakPtr scene_;
    bool initialized_ {};
};

SCENE_END_NAMESPACE()

#endif // SCENE_SRC_EFFECT_H
