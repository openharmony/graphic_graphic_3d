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

#ifndef SCENE_SRC_POSTPROCESS_TONEMAP_H
#define SCENE_SRC_POSTPROCESS_TONEMAP_H

#include <scene/interface/postprocess/intf_tonemap.h>

#include "postprocess_effect.h"

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(Tonemap, "56c1fc6e-90aa-486d-846e-c9a5c780a90a", META_NS::ObjectCategoryBits::NO_CATEGORY)

class Tonemap : public Internal::PostProcessEffect<ITonemap, CORE3D_NS::PostProcessComponent::TONEMAP_BIT> {
    META_OBJECT(Tonemap, ClassId::Tonemap, PostProcessEffect)
public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcessEffect, bool, Enabled, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(ITonemap, TonemapType, Type, "tonemapType")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(ITonemap, float, Exposure, "exposure")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(bool, Enabled)
    META_IMPLEMENT_PROPERTY(TonemapType, Type)
    META_IMPLEMENT_PROPERTY(float, Exposure)

private:
    BASE_NS::string_view GetComponentPath() const override;
};

SCENE_END_NAMESPACE()

#endif
