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

#ifndef SCENE_SRC_POSTPROCESS_BLOOM_H
#define SCENE_SRC_POSTPROCESS_BLOOM_H

#include <scene/ext/ecs_lazy_property.h>
#include <scene/ext/scene_property.h>
#include <scene/interface/postprocess/intf_bloom.h>
#include <scene/interface/postprocess/intf_postprocess_effect.h>

#include <meta/ext/implementation_macros.h>

#include "util.h"

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(Bloom, "6718b07d-c3d1-4036-bd0f-88d1380b846a", META_NS::ObjectCategoryBits::NO_CATEGORY)

class Bloom : public META_NS::IntroduceInterfaces<EcsLazyProperty, IBloom, IPPEffectInit> {
    META_OBJECT(Bloom, ClassId::Bloom, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcessEffect, bool, Enabled, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IBloom, BloomType, Type, "bloomType")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IBloom, EffectQualityType, Quality, "bloomQualityType")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IBloom, float, ThresholdHard, "thresholdHard")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IBloom, float, ThresholdSoft, "thresholdSoft")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IBloom, float, AmountCoefficient, "amountCoefficient")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IBloom, float, DirtMaskCoefficient, "dirtMaskCoefficient")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IBloom, IImage::Ptr, DirtMaskImage, "dirtMaskImage")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IBloom, bool, UseCompute, "useCompute")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IBloom, float, Scatter, "scatter")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IBloom, float, ScaleFactor, "scaleFactor")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(bool, Enabled)
    META_IMPLEMENT_PROPERTY(BloomType, Type)
    META_IMPLEMENT_PROPERTY(EffectQualityType, Quality)
    META_IMPLEMENT_PROPERTY(float, ThresholdHard)
    META_IMPLEMENT_PROPERTY(float, ThresholdSoft)
    META_IMPLEMENT_PROPERTY(float, AmountCoefficient)
    META_IMPLEMENT_PROPERTY(float, DirtMaskCoefficient)
    META_IMPLEMENT_PROPERTY(IImage::Ptr, DirtMaskImage)
    META_IMPLEMENT_PROPERTY(bool, UseCompute)
    META_IMPLEMENT_PROPERTY(float, Scatter)
    META_IMPLEMENT_PROPERTY(float, ScaleFactor)

    bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override;
    bool Init(const META_NS::IProperty::Ptr& flags) override
    {
        flags_ = flags;
        return static_cast<bool>(flags_);
    }

private:
    META_NS::Property<uint32_t> flags_;
};

SCENE_END_NAMESPACE()

#endif