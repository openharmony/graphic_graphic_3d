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

#ifndef SCENE_INTERFACE_POSTPROCESS_ICOLOR_ADJUSTMENTS_H
#define SCENE_INTERFACE_POSTPROCESS_ICOLOR_ADJUSTMENTS_H

#include <scene/base/types.h>
#include <scene/interface/postprocess/intf_postprocess_effect.h>

SCENE_BEGIN_NAMESPACE()

class IColorAdjustments : public IPostProcessEffect {
    META_INTERFACE(IPostProcessEffect, IColorAdjustments, "4a8c9d2e-5f3a-47b6-8c9d-2e5f3a47b68c")
public:
    /**
     * @brief Color filter multiplied with output color (RGB used, Alpha ignored)
     */
    META_PROPERTY(BASE_NS::Math::Vec4, FilterColor)

    /**
     * @brief Hue shift angle in degrees
     */
    META_PROPERTY(float, HueShift)

    /**
     * @brief Saturation (0.0 = grayscale, 1.0 = original, >1.0 = oversaturated)
     */
    META_PROPERTY(float, Saturation)

    /**
     * @brief Brightness (-1.0 to 1.0)
     */
    META_PROPERTY(float, Brightness)

    /**
     * @brief Contrast (0.0 = grayscale, 1.0 = original, >1.0 = high contrast)
     */
    META_PROPERTY(float, Contrast)
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IColorAdjustments)

#endif // SCENE_INTERFACE_POSTPROCESS_ICOLOR_ADJUSTMENTS_H
