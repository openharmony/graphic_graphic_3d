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

#include <scene/interface/postprocess/intf_bloom.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

class Bloom : public META_NS::IntroduceInterfaces<META_NS::MetaObject, IBloom> {
    META_OBJECT(Bloom, SCENE_NS::ClassId::Bloom, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(IPostProcessEffect, bool, Enabled)
    META_STATIC_PROPERTY_DATA(IBloom, BloomType, Type, BloomType::NORMAL)
    META_STATIC_PROPERTY_DATA(IBloom, EffectQualityType, Quality, EffectQualityType::NORMAL)
    META_STATIC_PROPERTY_DATA(IBloom, float, ThresholdHard, 1.f)
    META_STATIC_PROPERTY_DATA(IBloom, float, ThresholdSoft, 2.f)
    META_STATIC_PROPERTY_DATA(IBloom, float, AmountCoefficient, 0.25f)
    META_STATIC_PROPERTY_DATA(IBloom, float, DirtMaskCoefficient, 0.f)
    META_STATIC_PROPERTY_DATA(IBloom, IBitmap::Ptr, DirtMaskImage)
    META_STATIC_PROPERTY_DATA(IBloom, bool, UseCompute)
    META_STATIC_PROPERTY_DATA(IBloom, float, Scatter)
    META_STATIC_PROPERTY_DATA(IBloom, float, ScaleFactor)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(bool, Enabled)
    META_IMPLEMENT_PROPERTY(BloomType, Type)
    META_IMPLEMENT_PROPERTY(EffectQualityType, Quality)
    META_IMPLEMENT_PROPERTY(float, ThresholdHard)
    META_IMPLEMENT_PROPERTY(float, ThresholdSoft)
    META_IMPLEMENT_PROPERTY(float, AmountCoefficient)
    META_IMPLEMENT_PROPERTY(float, DirtMaskCoefficient)
    META_IMPLEMENT_PROPERTY(IBitmap::Ptr, DirtMaskImage)
    META_IMPLEMENT_PROPERTY(bool, UseCompute)
    META_IMPLEMENT_PROPERTY(float, Scatter)
    META_IMPLEMENT_PROPERTY(float, ScaleFactor)
};

SCENE_END_NAMESPACE()

#endif