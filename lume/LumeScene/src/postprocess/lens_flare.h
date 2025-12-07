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

#ifndef SCENE_SRC_POSTPROCESS_LENS_FLARE_H
#define SCENE_SRC_POSTPROCESS_LENS_FLARE_H

#include <scene/interface/postprocess/intf_lens_flare.h>

#include "postprocess_effect.h"

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(LensFlare, "72693514-26dc-4df4-86a7-cdaaa2d023f9", META_NS::ObjectCategoryBits::NO_CATEGORY)

class LensFlare : public Internal::PostProcessEffect<ILensFlare, CORE3D_NS::PostProcessComponent::LENS_FLARE_BIT> {
    META_OBJECT(LensFlare, ClassId::LensFlare, PostProcessEffect)
public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcessEffect, bool, Enabled, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(ILensFlare, EffectQualityType, Quality, "quality")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(ILensFlare, float, Intensity, "intensity")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(ILensFlare, BASE_NS::Math::Vec3, FlarePosition, "flarePosition")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(bool, Enabled)
    META_IMPLEMENT_PROPERTY(EffectQualityType, Quality)
    META_IMPLEMENT_PROPERTY(float, Intensity)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec3, FlarePosition)

private:
    BASE_NS::string_view GetComponentPath() const override;
};

SCENE_END_NAMESPACE()

#endif // SCENE_SRC_POSTPROCESS_LENS_FLARE_H
