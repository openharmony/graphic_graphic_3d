/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SCENE_SRC_POSTPROCESS_WHITE_BALANCE_H
#define SCENE_SRC_POSTPROCESS_WHITE_BALANCE_H

#include <scene/interface/postprocess/intf_white_balance.h>

#include "postprocess_effect.h"

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(WhiteBalance, "b8c7d6e5-f4a3-b2c1-d0e9-f8a7b6c5d4e3", META_NS::ObjectCategoryBits::NO_CATEGORY)

class WhiteBalance
    : public Internal::PostProcessEffect<IWhiteBalance, CORE3D_NS::PostProcessComponent::WHITE_BALANCE_BIT> {
    META_OBJECT(WhiteBalance, ClassId::WhiteBalance, PostProcessEffect)
public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcessEffect, bool, Enabled, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IWhiteBalance, float, Temperature, "temperature")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IWhiteBalance, float, Tint, "tint")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(bool, Enabled)
    META_IMPLEMENT_PROPERTY(float, Temperature)
    META_IMPLEMENT_PROPERTY(float, Tint)

private:
    BASE_NS::string_view GetComponentPath() const override;
};

SCENE_END_NAMESPACE()

#endif
