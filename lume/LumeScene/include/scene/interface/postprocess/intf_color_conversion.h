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

#ifndef SCENE_INTERFACE_POSTPROCESS_ICOLOR_CONVERSION_H
#define SCENE_INTERFACE_POSTPROCESS_ICOLOR_CONVERSION_H

#include <scene/base/types.h>
#include <scene/interface/postprocess/intf_postprocess_effect.h>

SCENE_BEGIN_NAMESPACE()

/** Color conversion function type */
enum ColorConversionFunctionType : uint32_t {
    /** Linear -> no conversion in normal situation. */
    LINEAR = 0,
    /** Linear to sRGB conversion */
    LINEAR_TO_SRGB = 1,
};

class IColorConversion : public IPostProcessEffect {
    META_INTERFACE(IPostProcessEffect, IColorConversion, "ef36e89e-2379-4297-9cac-10943f1c605c")
public:
    /**
     * @brief Color conversion function.
     */
    META_PROPERTY(ColorConversionFunctionType, Function)
    /**
     * @brief If true, multiply rgb with alpha.
     */
    META_PROPERTY(bool, MultiplyWithAlpha)
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::ColorConversionFunctionType)
META_INTERFACE_TYPE(SCENE_NS::IColorConversion)

#endif // SCENE_INTERFACE_POSTPROCESS_ICOLOR_CONVERSION_H
