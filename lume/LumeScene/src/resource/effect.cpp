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

#include "effect.h"

#include <scene/ext/intf_internal_scene.h>
#include <set>

#include <3d/implementation_uids.h>
#include <3d/render/intf_render_node_scene_util.h>
#include <core/plugin/intf_class_factory.h>
#include <render/datastore/render_data_store_render_pods.h>

#include <meta/api/engine/util.h>
#include <meta/api/make_callback.h>
#include <meta/interface/engine/intf_engine_data.h>

constexpr auto BLOOM_TYPE = PROPERTYTYPE(RENDER_NS::BloomConfiguration);
constexpr auto VIGNETTE_TYPE = PROPERTYTYPE(RENDER_NS::VignetteConfiguration);
constexpr auto COLOR_FRINGE_TYPE = PROPERTYTYPE(RENDER_NS::ColorFringeConfiguration);
constexpr auto DITHER_TYPE = PROPERTYTYPE(RENDER_NS::DitherConfiguration);
constexpr auto BLUR_TYPE = PROPERTYTYPE(RENDER_NS::BlurConfiguration);
constexpr auto TONEMAP_TYPE = PROPERTYTYPE(RENDER_NS::TonemapConfiguration);
constexpr auto COLOR_CONVERSION_TYPE = PROPERTYTYPE(RENDER_NS::ColorConversionConfiguration);
constexpr auto FXAA_TYPE = PROPERTYTYPE(RENDER_NS::FxaaConfiguration);
constexpr auto TAA_TYPE = PROPERTYTYPE(RENDER_NS::TaaConfiguration);
constexpr auto DOF_TYPE = PROPERTYTYPE(RENDER_NS::DofConfiguration);
constexpr auto MOTION_BLUR_TYPE = PROPERTYTYPE(RENDER_NS::MotionBlurConfiguration);
constexpr auto LENS_FLARE_TYPE = PROPERTYTYPE(RENDER_NS::LensFlareConfiguration);
constexpr auto UPSCALE_TYPE = PROPERTYTYPE(RENDER_NS::UpscaleConfiguration);

SCENE_BEGIN_NAMESPACE()

bool Effect::Build(const META_NS::IMetadata::Ptr& data)
{
    if (Super::Build(data)) {
        return true;
    }
    return false;
}

void Effect::Destroy()
{
    pp_.reset();
    scene_.reset();
    syncPropertiesCallable_.reset();
    Super::Destroy();
}

BASE_NS::string Effect::GetName() const
{
    return Super::GetName();
}

Future<bool> Effect::InitializeEffect(const BASE_NS::shared_ptr<IScene>& scene, META_NS::ObjectId effectClassId)
{
    if (!scene) {
        return {};
    }
    auto is = scene->GetInternalScene();
    return is->AddTaskOrRunDirectly([=, self = BASE_NS::weak_ptr { GetSelf() }]() {
        auto me = self.lock();
        if (me) {
            if (pp_) {
                return effectClassId_ == effectClassId.ToUid() && scene->GetInternalScene() == scene_.lock();
            }
            scene_ = is;
            effectClassId_ = effectClassId.ToUid();
            initialized_ = CreateEffect();
            return initialized_;
        }
        return false;
    });
}

META_NS::ObjectId Effect::GetEffectClassId() const
{
    return effectClassId_;
}

bool Effect::CreateEffect()
{
    if (!pp_) {
        if (auto scene = scene_.lock()) {
            valueManager_ =
                META_NS::GetObjectRegistry().Create<META_NS::IEngineValueManager>(META_NS::ClassId::EngineValueManager);
            if (auto fac = scene->GetRenderContext().GetInterface<CORE_NS::IClassFactory>()) {
                if (pp_ = CORE_NS::CreateInstance<RENDER_NS::IRenderPostProcess>(*fac, effectClassId_); pp_) {
                    PopulateProperties();
                }
            }
        }
    }
    return pp_ != nullptr;
}

void Effect::PopulateProperties()
{
    if (!(valueManager_ && pp_)) {
        return;
    }

    META_NS::EnginePropertyHandle h;
    h.handle = pp_->GetProperties();

    // List of know PostProcess property types that we should traverse into (even if they are known types by our meta
    // system)
    static std::set<uint64_t> knownPostProcesses = { BLOOM_TYPE, VIGNETTE_TYPE, COLOR_FRINGE_TYPE, DITHER_TYPE,
        BLUR_TYPE, TONEMAP_TYPE, COLOR_CONVERSION_TYPE, FXAA_TYPE, TAA_TYPE, DOF_TYPE, MOTION_BLUR_TYPE,
        LENS_FLARE_TYPE, UPSCALE_TYPE };

    BASE_NS::vector<META_NS::IEngineValue::Ptr> valid;
    BASE_NS::vector<META_NS::IEngineValue::Ptr> values;
    META_NS::EngineValueOptions opt;
    opt.values = &values;
    if (valueManager_->ConstructValues(h, opt)) {
        for (auto&& v : values) {
            if (auto internal = interface_cast<META_NS::IEngineValueInternal>(v)) {
                // If we know how to handle this property type, just add it directly to list
                auto p = internal->GetPropertyParams();
                // First check if the property type is on our list of known post process struct types (we should
                // traverse inside it)
                if (knownPostProcesses.find(p.property.type) == knownPostProcesses.end()) {
                    if (auto access =
                            META_NS::GetObjectRegistry().GetEngineData().GetInternalValueAccess(p.property.type)) {
                        valid.push_back(v);
                        continue;
                    }
                }
            }
            // Try to traverse to members
            BASE_NS::vector<META_NS::IEngineValue::Ptr> members;
            META_NS::EngineValueOptions memberOpt;
            memberOpt.values = &members;
            valueManager_->ConstructValues(v, memberOpt);
            if (members.empty()) {
                valid.push_back(v);
            } else {
                valid.insert(valid.end(), members.begin(), members.end());
            }
        }
    }
    valueManager_->Sync(META_NS::EngineSyncDirection::FROM_ENGINE);
    AddProperties(valid);
}

void Effect::AddPropertyUpdateHook(const META_NS::IProperty::Ptr& prop)
{
    if (prop) {
        if (!syncPropertiesCallable_) {
            syncPropertiesCallable_ = META_NS::MakeCallback<META_NS::IOnChanged>(
                [this, self = BASE_NS::weak_ptr(GetSelf<META_NS::IEnginePropertySync>())] {
                    if (auto me = self.lock()) {
                        if (auto scene = scene_.lock()) {
                            // Scene will call Effect::SyncProperties()
                            scene->SchedulePropertyUpdate(me);
                        }
                    }
                });
        }
        prop->OnChanged()->AddHandler(syncPropertiesCallable_);
    }
}

void Effect::SyncProperties()
{
    if (valueManager_) {
        valueManager_->Sync(META_NS::EngineSyncDirection::TO_ENGINE);
    }
}

void Effect::AddProperties(BASE_NS::array_view<const META_NS::IEngineValue::Ptr> values)
{
    auto meta = GetSelf<META_NS::IMetadata>();
    if (!meta) {
        return;
    }
    for (auto&& v : values) {
        auto name = v->GetName();
        if (name == "enabled") {
            // Special handling for "enabled" from effect metadata. Bind that to our Enabled property.
            auto p = META_ACCESS_PROPERTY(Enabled);
            if (auto i = interface_cast<META_NS::IStackProperty>(p)) {
                i->PushValue(v);
                i->SetDefaultValue(v->GetValue());
                AddPropertyUpdateHook(p);
            }
        } else {
            // Rest of the properties we just expose as is (without the possible struct name, e.g.
            // "lensFlareConfiguration.intensity" becomes "intensity"
            name = name.substr(name.find_last_of('.') + 1);
            if (auto internal = interface_cast<META_NS::IEngineValueInternal>(v)) {
                const auto params = internal->GetPropertyParams();
                if (params.property.type !=
                    CORE_NS::PropertySystem::PropertyTypeDeclFromType<CORE_NS::IPropertyHandle*>()) {
                    auto p = META_NS::PropertyFromEngineValue(name, v);
                    meta->AddProperty(p);
                    AddPropertyUpdateHook(p);
                }
            }
        }
    }
}

RENDER_NS::IRenderPostProcess::Ptr Effect::GetEffect() const
{
    return pp_;
}

SCENE_END_NAMESPACE()
