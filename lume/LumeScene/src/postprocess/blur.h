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

#ifndef SCENE_SRC_POSTPROCESS_BLUR_H
#define SCENE_SRC_POSTPROCESS_BLUR_H

#include <scene/interface/postprocess/intf_blur.h>

#include <3d/ecs/components/post_process_component.h>

#include "postprocess_effect.h"

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(Blur, "cf09372e-223a-4272-9063-38c0f07b2f4b", META_NS::ObjectCategoryBits::NO_CATEGORY)

class Blur : public Internal::PostProcessEffect<IBlur, CORE3D_NS::PostProcessComponent::BLUR_BIT> {
    META_OBJECT(Blur, ClassId::Blur, PostProcessEffect)
public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcessEffect, bool, Enabled, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IBlur, BlurType, Type, "blurType")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IBlur, EffectQualityType, Quality, "blurQualityType")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IBlur, float, FilterSize, "filterSize")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IBlur, uint32_t, MaxMipmapLevel, "maxMipLevel")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(bool, Enabled)
    META_IMPLEMENT_PROPERTY(BlurType, Type)
    META_IMPLEMENT_PROPERTY(EffectQualityType, Quality)
    META_IMPLEMENT_PROPERTY(float, FilterSize)
    META_IMPLEMENT_PROPERTY(uint32_t, MaxMipmapLevel)

private:
    BASE_NS::string_view GetComponentPath() const override;
};

SCENE_END_NAMESPACE()

#endif // SCENE_SRC_POSTPROCESS_BLUR_H
