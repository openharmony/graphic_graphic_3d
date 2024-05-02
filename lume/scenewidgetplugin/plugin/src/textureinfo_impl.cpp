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
#include <scene_plugin/api/material_uid.h>

#include <meta/ext/concrete_base_object.h>

#include "bind_templates.inl"
#include "intf_proxy_object_holder.h"
#include <meta/api/property/array_property_event_handler.h> 

using CORE3D_NS::MaterialComponent;
META_BEGIN_NAMESPACE();
META_TYPE(MaterialComponent::TextureInfo);
META_END_NAMESPACE();

namespace {
using namespace SCENE_NS;
class TextureInfoImpl
    : public META_NS::ObjectFwd<TextureInfoImpl, ClassId::TextureInfo, META_NS::ClassId::Object,
          ITextureInfo, IProxyObject, IProxyObjectHolder> {
    META_IMPLEMENT_INTERFACE_PROPERTY(ITextureInfo, uint32_t, TextureSlotIndex, ~0u)
    META_IMPLEMENT_INTERFACE_PROPERTY(ITextureInfo, BASE_NS::string, Name, "")
    META_IMPLEMENT_INTERFACE_PROPERTY(ITextureInfo, IBitmap::Ptr, Image, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(ITextureInfo, uint8_t, Sampler, 0)
    META_IMPLEMENT_INTERFACE_PROPERTY(ITextureInfo, BASE_NS::Math::Vec4, Factor, Colors::TRANSPARENT)
    META_IMPLEMENT_INTERFACE_PROPERTY(
        ITextureInfo, BASE_NS::Math::Vec2, Translation, BASE_NS::Math::Vec2(0, 0))
    META_IMPLEMENT_INTERFACE_PROPERTY(ITextureInfo, float, Rotation, 0.f)
    META_IMPLEMENT_INTERFACE_PROPERTY(ITextureInfo, BASE_NS::Math::Vec2, Scale, BASE_NS::Math::Vec2(1.f, 1.f))

    BASE_NS::vector<PropertyPair> ListBoundProperties() const override
    {
        return holder_.GetBoundProperties();
    }

    PropertyHandlerArrayHolder& Holder() override
    {
        return holder_;
    }

    void SetTextureSlotIndex(uint32_t index) override
    {
        META_ACCESS_PROPERTY(TextureSlotIndex)->SetValue(index);
    }

    uint32_t GetTextureSlotIndex() const override
    {
        return GetValue(META_ACCESS_PROPERTY(TextureSlotIndex));
    }

    META_NS::ArrayProperty<CORE3D_NS::MaterialComponent::TextureInfo> array_;
    uint32_t slotIndex_;

    using ChangeInfo = META_NS::ArrayChanges<CORE3D_NS::MaterialComponent::TextureInfo>;
    META_NS::PropertyChangedEventHandler handler_;
    META_NS::PropertyChangedEventHandler handlers_[6];
    bool Build(const IMetadata::Ptr& params) override
    {
        // okay.. collect the propery that contains the data, And the our index..
	// slot names (unsure-what-they-were-before-but-matches-core3d-enum)
	auto sh = params->GetPropertyByName<uintptr_t>("sceneHolder")->GetValue();
	holder_.SetSceneHolder(*((SceneHolder::Ptr*)sh));

	auto ip = params->GetPropertyByName<META_NS::IProperty::Ptr>("textureInfoArray")->GetValue();
	array_= ip;
	slotIndex_ = params->GetPropertyByName<uint32_t>("textureSlotIndex")->GetValue();
	BASE_NS::string textureSlotName;
	const char* TextureIndexName[] = { "BASE_COLOR", "NORMAL", "MATERIAL", "EMISSIVE", "AO", "CLEARCOAT",
	    "CLEARCOAT_ROUGHNESS", "CLEARCOAT_NORMAL", "SHEEN", "TRANSMISSION", "SPECULAR" };
	if (slotIndex_ < BASE_NS::countof(TextureIndexName)) {
	    textureSlotName = TextureIndexName[slotIndex_];
	} else {
	    textureSlotName = "TextureSlot_" + BASE_NS::to_string(slotIndex_);
	}
	Name()->SetValue(textureSlotName);

	using namespace META_NS;
	handler_.Subscribe(array_, MakeCallback<IOnChanged>(this, &TextureInfoImpl::ArrayChanged));
	ArrayChanged();
	handlers_[0].Subscribe(Image(), MakeCallback<IOnChanged>(this, &TextureInfoImpl::ImageChanged));
	handlers_[1].Subscribe(Sampler(), MakeCallback<IOnChanged>(this, &TextureInfoImpl::SamplerChanged));
	handlers_[2].Subscribe(Factor(), MakeCallback<IOnChanged>(this, &TextureInfoImpl::FactorChanged)); // 2 index
    // 3 index
	handlers_[3].Subscribe(Translation(), MakeCallback<IOnChanged>(this, &TextureInfoImpl::TranslationChanged));
	handlers_[4].Subscribe(Rotation(), MakeCallback<IOnChanged>(this, &TextureInfoImpl::RotationChanged)); // 4 index
	handlers_[5].Subscribe(Scale(), MakeCallback<IOnChanged>(this, &TextureInfoImpl::ScaleChanged)); // 5 index
	return true;
    }

    void ArrayChanged()
    {
        // change from ecs?>
	const auto& curVal = array_->GetValueAt(slotIndex_);
	if (curVal.image != curImage_) {
	    curImage_ = curVal.image;
	    auto sh = holder_.GetSceneHolder();
	    auto bitmap = ImageConverter::ToUi(*sh, curVal.image);
	    if (bitmap != curBitmap_) {
	        Image()->SetValue(bitmap);
                curBitmap_ = bitmap;
	    }
	}

	Factor()->SetValue(curVal.factor);
	Translation()->SetValue(curVal.transform.translation);
	Rotation()->SetValue(curVal.transform.rotation);
	Scale()->SetValue(curVal.transform.scale);
    }

    void ImageChanged()
    {
	auto bitmap = Image()->GetValue();
	if (bitmap!= curBitmap_) {
	    curBitmap_ = bitmap;
	    auto curVal = array_->GetValueAt(slotIndex_);
	    auto sh = holder_.GetSceneHolder();
	    curVal.image = ImageConverter::ToEcs(*sh, Image()->GetValue());
	    curImage_ = curVal.image;
	    array_->SetValueAt(slotIndex_, curVal);
	}
    }

    void SamplerChanged() {}
    void FactorChanged()
    {
	auto curVal = array_->GetValueAt(slotIndex_);
	curVal.factor = Factor()->GetValue();
	array_->SetValueAt(slotIndex_, curVal);
    }
    void TranslationChanged() {}
    void RotationChanged() {}
    void ScaleChanged() {}
    void Update(const ChangeInfo& changes) {}

private:
    SCENE_NS::IBitmap::Ptr curBitmap_;
    CORE_NS::EntityReference curImage_;
    PropertyHandlerArrayHolder holder_;
};
} // namespace
SCENE_BEGIN_NAMESPACE()

void RegisterTextureInfoImpl()
{
    META_NS::GetObjectRegistry().RegisterObjectType<TextureInfoImpl>();
}
void UnregisterTextureInfoImpl()
{
    META_NS::GetObjectRegistry().UnregisterObjectType<TextureInfoImpl>();
}

SCENE_END_NAMESPACE()
