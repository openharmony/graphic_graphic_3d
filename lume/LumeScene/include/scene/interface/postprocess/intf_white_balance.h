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

#ifndef SCENE_INTERFACE_POSTPROCESS_IWHITE_BALANCE_H
#define SCENE_INTERFACE_POSTPROCESS_IWHITE_BALANCE_H

#include <scene/base/types.h>
#include <scene/interface/postprocess/intf_postprocess_effect.h>

SCENE_BEGIN_NAMESPACE()

class IWhiteBalance : public IPostProcessEffect {
    META_INTERFACE(IPostProcessEffect, IWhiteBalance, "7a3b9c1f-2d4e-5f6a-7b8c-9d0e1f2a3b4c")
public:
    /**
     * @brief White balance temperature adjustment (-100 to 100, default 0).
     * Positive values warm the image, negative values cool it.
     * Based on chromaticity coordinate adjustment in LMS color space.
     */
    META_PROPERTY(float, Temperature)
    /**
     * @brief White balance tint adjustment (-100 to 100, default 0).
     * Positive values add green tint, negative values add magenta tint.
     * Adjusts chromaticity Y coordinate for color cast correction.
     */
    META_PROPERTY(float, Tint)
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IWhiteBalance)

#endif
