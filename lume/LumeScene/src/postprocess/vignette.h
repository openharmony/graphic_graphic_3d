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

#ifndef SCENE_SRC_POSTPROCESS_VIGNETTE_H
#define SCENE_SRC_POSTPROCESS_VIGNETTE_H

#include <scene/interface/postprocess/intf_vignette.h>

#include "postprocess_effect.h"

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(Vignette, "af3eb8f6-b271-4fb5-b077-a4644942be89", META_NS::ObjectCategoryBits::NO_CATEGORY)

class Vignette : public Internal::PostProcessEffect<IVignette, CORE3D_NS::PostProcessComponent::VIGNETTE_BIT> {
    META_OBJECT(Vignette, ClassId::Vignette, PostProcessEffect)
public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcessEffect, bool, Enabled, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IVignette, float, Coefficient, "coefficient")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IVignette, float, Power, "power")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(bool, Enabled)
    META_IMPLEMENT_PROPERTY(float, Coefficient)
    META_IMPLEMENT_PROPERTY(float, Power)

private:
    BASE_NS::string_view GetComponentPath() const override;
};

SCENE_END_NAMESPACE()

#endif // SCENE_SRC_POSTPROCESS_VIGNETTE_H
