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
#include <scene_plugin/api/postprocess_uid.h>

#include <3d/ecs/components/post_process_component.h>
#include <render/datastore/render_data_store_render_pods.h>

#include <meta/ext/concrete_base_object.h>
#include <meta/interface/property/property.h>

#include "bind_templates.inl"
#include "intf_postprocess_private.h"
#include "node_impl.h"
#include "task_utils.h"

using SCENE_NS::MakeTask;

namespace {

class PostProcessImpl
    : public META_NS::ObjectFwd<PostProcessImpl, SCENE_NS::ClassId::PostProcess, META_NS::ClassId::Object,
          SCENE_NS::IPostProcess, IPostProcessPrivate, SCENE_NS::IProxyObject> {
    META_IMPLEMENT_INTERFACE_PROPERTY(META_NS::INamed, BASE_NS::string, Name)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IPostProcess, SCENE_NS::IBloom::Ptr, Bloom)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IPostProcess, SCENE_NS::IBlur::Ptr, Blur)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IPostProcess, SCENE_NS::IColorConversion::Ptr, ColorConversion)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IPostProcess, SCENE_NS::IColorFringe::Ptr, ColorFringe)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IPostProcess, SCENE_NS::IDepthOfField::Ptr, DepthOfField)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IPostProcess, SCENE_NS::IDither::Ptr, Dither)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IPostProcess, SCENE_NS::IFxaa::Ptr, Fxaa)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IPostProcess, SCENE_NS::IMotionBlur::Ptr, MotionBlur)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IPostProcess, SCENE_NS::ITaa::Ptr, Taa)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IPostProcess, SCENE_NS::ITonemap::Ptr, Tonemap)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IPostProcess, SCENE_NS::IVignette::Ptr, Vignette)

    static constexpr BASE_NS::string_view PP_ENABLE_FLAGS = "PostProcessComponent.enableFlags"; // uint32_t
    bool IsBitSet(META_NS::Property<uint32_t> property, uint32_t bit)
    {
        auto value = property->GetValue();
	return static_cast<bool>(value& bit);
    }

    void SetBit(META_NS::Property<uint32_t> property, uint32_t bit, bool set)
    {
        auto value = property->GetValue();
	if (set) {
	    value |= bit;
	} else {
	    value &= ~bit;
	}
	property->SetValue(value);
    }

    META_NS::PropertyChangedEventHandler handler_[11];

    ~PostProcessImpl()
    {
        SCENE_PLUGIN_VERBOSE_LOG("%s", __func__);
    }

    bool Build(const IMetadata::Ptr& data) override
    {
        auto& obr = META_NS::GetObjectRegistry();
        if (auto bloom = obr.Create<SCENE_NS::IBloom>(SCENE_NS::ClassId::Bloom)) {
            META_NS::SetValue(interface_cast<META_NS::INamed>(bloom)->Name(), "Bloom");
            META_NS::SetValue(Bloom(), bloom);
        }
        if (auto blur = obr.Create<SCENE_NS::IBlur>(SCENE_NS::ClassId::Blur)) {
            META_NS::SetValue(interface_cast<META_NS::INamed>(blur)->Name(), "Blur");
            META_NS::SetValue(Blur(), blur);
        }
        if (auto colorConversion =
                obr.Create<SCENE_NS::IColorConversion>(SCENE_NS::ClassId::ColorConversion)) {
            META_NS::SetValue(interface_cast<META_NS::INamed>(colorConversion)->Name(), "ColorConversion");
            META_NS::SetValue(ColorConversion(), colorConversion);
        }
        if (auto colorFringe =
                obr.Create<SCENE_NS::IColorFringe>(SCENE_NS::ClassId::ColorFringe)) {
            META_NS::SetValue(interface_cast<META_NS::INamed>(colorFringe)->Name(), "ColorFringe");
            META_NS::SetValue(ColorFringe(), colorFringe);
        }
        if (auto depthOfField =
                obr.Create<SCENE_NS::IDepthOfField>(SCENE_NS::ClassId::DepthOfField)) {
            META_NS::SetValue(interface_cast<META_NS::INamed>(depthOfField)->Name(), "DepthOfField");
            META_NS::SetValue(DepthOfField(), depthOfField);
        }
        if (auto dither = obr.Create<SCENE_NS::IDither>(SCENE_NS::ClassId::Dither)) {
            META_NS::SetValue(interface_cast<META_NS::INamed>(dither)->Name(), "Dither");
            META_NS::SetValue(Dither(), dither);
        }
        if (auto fxaaSettings = obr.Create<SCENE_NS::IFxaa>(SCENE_NS::ClassId::Fxaa)) {
            META_NS::SetValue(interface_cast<META_NS::INamed>(fxaaSettings)->Name(), "Fxaa");
            META_NS::SetValue(Fxaa(), fxaaSettings);
        }
        if (auto blur = obr.Create<SCENE_NS::IMotionBlur>(SCENE_NS::ClassId::MotionBlur)) {
            META_NS::SetValue(interface_cast<META_NS::INamed>(blur)->Name(), "MotionBlur");
            META_NS::SetValue(MotionBlur(), blur);
        }
        if (auto taaSettings = obr.Create<SCENE_NS::ITaa>(SCENE_NS::ClassId::Taa)) {
            META_NS::SetValue(interface_cast<META_NS::INamed>(taaSettings)->Name(), "Taa");
            META_NS::SetValue(Taa(), taaSettings);
        }
        if (auto tonemap = obr.Create<SCENE_NS::ITonemap>(SCENE_NS::ClassId::Tonemap)) {
            META_NS::SetValue(interface_cast<META_NS::INamed>(tonemap)->Name(), "Tonemap");
            META_NS::SetValue(Tonemap(), tonemap);
        }
        if (auto vignette = obr.Create<SCENE_NS::IVignette>(SCENE_NS::ClassId::Vignette)) {
            META_NS::SetValue(interface_cast<META_NS::INamed>(vignette)->Name(), "Vignette");
            META_NS::SetValue(Vignette(), vignette);
        }

        if (auto meta = GetSelf<META_NS::IMetadata>()) {
            for (auto& prop : meta->GetAllProperties()) {
		auto name = prop->GetName();
                if (auto iptr = interface_cast<IPostProcessEffectPrivate>(GetPointer(prop))) {
                    META_NS::PropertyLock p(prop);
                    p->OnChanged()->AddHandler(
                            META_NS::MakeCallback<META_NS::IOnChanged>([this, weak = BASE_NS::weak_ptr(prop)]() {
                        if (auto intPtr = weak.lock()) {
                                    if (auto valueObject =
                                            interface_cast<IPostProcessEffectPrivate>(GetPointer(intPtr))) {
                                // prefer values from toolkit
                                valueObject->Bind(ecsObject_, sh_, false);
                                    }
                        }
                            }),
                            reinterpret_cast<uint64_t>(this));
                }
            }
        }

        return true;
    }

    CORE_NS::Entity GetEntity() override
    {
        if (ecsObject_) {
            return ecsObject_->GetEntity();
        }
        return {};
    }
    // In principle, this implementation should be splitted to two parts which run the introspection
    // first on ecs queue and then bind the metaproperties on toolkit side
    void SetEntity(CORE_NS::Entity entity, SceneHolder::Ptr sh, bool preferEcsValues) override
    {
        if (ecsObject_ && ecsObject_->GetEntity() == entity) {
            return;
        }

        ecsObject_ = META_NS::GetObjectRegistry().Create<SCENE_NS::IEcsObject>(SCENE_NS::ClassId::EcsObject);
        sh_ = sh;

        if (auto proxyIf = interface_pointer_cast<SCENE_NS::IEcsProxyObject>(ecsObject_)) {
            proxyIf->SetCommonListener(sh->GetCommonEcsListener());
        }

        auto ecs = sh->GetEcs();
        ecsObject_->SetEntity(ecs, ecs->GetEntityManager().GetReferenceCounted(entity));

        if (auto tonemap = interface_cast<IPostProcessEffectPrivate>(META_NS::GetValue(Tonemap()))) {
            tonemap->Bind(ecsObject_, sh, preferEcsValues);
        }
        if (auto bloom = interface_cast<IPostProcessEffectPrivate>(META_NS::GetValue(Bloom()))) {
            bloom->Bind(ecsObject_, sh, preferEcsValues);
        }
        if (auto vignette = interface_cast<IPostProcessEffectPrivate>(META_NS::GetValue(Vignette()))) {
            vignette->Bind(ecsObject_, sh, preferEcsValues);
        }
        if (auto colorFringe = interface_cast<IPostProcessEffectPrivate>(META_NS::GetValue(ColorFringe()))) {
            colorFringe->Bind(ecsObject_, sh, preferEcsValues);
        }
        if (auto dither = interface_cast<IPostProcessEffectPrivate>(META_NS::GetValue(Dither()))) {
            dither->Bind(ecsObject_, sh, preferEcsValues);
        }
        if (auto fxaa = interface_cast<IPostProcessEffectPrivate>(META_NS::GetValue(Fxaa()))) {
            fxaa->Bind(ecsObject_, sh, preferEcsValues);
        }
        if (auto taa = interface_cast<IPostProcessEffectPrivate>(META_NS::GetValue(Taa()))) {
            taa->Bind(ecsObject_, sh, preferEcsValues);
        }
        if (auto dof = interface_cast<IPostProcessEffectPrivate>(META_NS::GetValue(DepthOfField()))) {
            dof->Bind(ecsObject_, sh, preferEcsValues);
        }
        if (auto mb = interface_cast<IPostProcessEffectPrivate>(META_NS::GetValue(MotionBlur()))) {
            mb->Bind(ecsObject_, sh, preferEcsValues);
        }
        propHandler_.Reset();

        propHandler_.SetUseEcsDefaults(preferEcsValues);
	
	auto meta = interface_pointer_cast<META_NS::IMetadata>(ecsObject_);
	if (!meta) {
	    return;
	}

	auto postState = meta->GetPropertyByName(PP_ENABLE_FLAGS);
	if (!postState) {
	    return;
	}
        using namespace META_NS;
        using namespace CORE3D_NS;

	auto func = [](PostProcessImpl* me, IProperty::Ptr fxState, IProperty::Ptr postprocessState, uint32_t bit) {
	    bool enabled = false;
	    if (fxState) {
	        fxState->GetValue().GetValue<bool>(enabled);
	    }
	    auto val = me->currentEnabled;
	    if (enabled) {
	        val |= bit;
	    } else {
	        val &= ~bit;
	    }
	    if (val != me->currentEnabled) {
                me->currentEnabled = val;
		META_NS::Property<uint32_t> pps(postprocessState);
		pps->SetValue(me->currentEnabled);
	    }
	};
#define ADD_HANDLER(Fx, Bit) \
    { \
        auto prop = Fx()->GetValue()->Enabled().GetProperty(); \
	handler_[idx++].Subscribe(prop, MakeCallback<IOnChanged>(func, this, prop, postState, (uint32_t)Bit)); \
	func(this, prop, postState, (uint32_t)Bit); \
    }
        uint32_t idx = 0;
	ADD_HANDLER(Bloom, PostProcessComponent::BLOOM_BIT);
	ADD_HANDLER(Blur, PostProcessComponent::BLUR_BIT);
	ADD_HANDLER(ColorConversion, PostProcessComponent::COLOR_CONVERSION_BIT);
	ADD_HANDLER(ColorFringe, PostProcessComponent::COLOR_FRINGE_BIT);
	ADD_HANDLER(DepthOfField, PostProcessComponent::DOF_BIT);
	ADD_HANDLER(Dither, PostProcessComponent::DITHER_BIT);
	ADD_HANDLER(Fxaa, PostProcessComponent::FXAA_BIT);
	ADD_HANDLER(MotionBlur, PostProcessComponent::MOTION_BLUR_BIT);
	ADD_HANDLER(Taa, PostProcessComponent::TAA_BIT);
	ADD_HANDLER(Tonemap, PostProcessComponent::TONEMAP_BIT);
	ADD_HANDLER(Vignette, PostProcessComponent::VIGNETTE_BIT);
    }

    BASE_NS::vector<SCENE_NS::IProxyObject::PropertyPair> ListBoundProperties() const override
    {
        return propHandler_.GetBoundProperties();
    }
    
    uint32_t currentEnabled { 0 };
    SCENE_NS::IEcsObject::Ptr ecsObject_;
    SceneHolder::Ptr sh_;
    PropertyHandlerArrayHolder propHandler_ {};
};

} // namespace

SCENE_BEGIN_NAMESPACE()

void RegisterPostprocessImpl()
{
    auto& registry = META_NS::GetObjectRegistry();
    registry.RegisterObjectType<PostProcessImpl>();
}

void UnregisterPostprocessImpl()
{
    auto& registry = META_NS::GetObjectRegistry();
    registry.UnregisterObjectType<PostProcessImpl>();
}

SCENE_END_NAMESPACE()
