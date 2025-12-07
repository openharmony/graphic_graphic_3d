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

#ifndef SCENE_SRC_POSTPROCESS_UPSCALE_H
#define SCENE_SRC_POSTPROCESS_UPSCALE_H

#include <scene/interface/postprocess/intf_upscale.h>

#include "postprocess_effect.h"

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(Upscale, "4bb7b52d-9ccc-40a5-8f47-510946410fb5", META_NS::ObjectCategoryBits::NO_CATEGORY)

class Upscale : public Internal::PostProcessEffect<IUpscale, CORE3D_NS::PostProcessComponent::UPSCALE_BIT> {
    META_OBJECT(Upscale, ClassId::Upscale, PostProcessEffect)
public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcessEffect, bool, Enabled, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IUpscale, float, SmoothScale, "smoothScale")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IUpscale, float, StructureSensitivity, "structureSensitivity")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IUpscale, float, EdgeSharpness, "edgeSharpness")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IUpscale, float, Ratio, "ratio")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(bool, Enabled)
    META_IMPLEMENT_PROPERTY(float, SmoothScale)
    META_IMPLEMENT_PROPERTY(float, StructureSensitivity)
    META_IMPLEMENT_PROPERTY(float, EdgeSharpness)
    META_IMPLEMENT_PROPERTY(float, Ratio)

private:
    BASE_NS::string_view GetComponentPath() const override;
};

SCENE_END_NAMESPACE()

#endif // SCENE_SRC_POSTPROCESS_UPSCALE_H
