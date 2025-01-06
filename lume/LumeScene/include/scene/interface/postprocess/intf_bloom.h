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

#ifndef SCENE_INTERFACE_POSTPROCESS_IBLOOM_H
#define SCENE_INTERFACE_POSTPROCESS_IBLOOM_H

#include <scene/base/types.h>
#include <scene/interface/intf_bitmap.h>
#include <scene/interface/postprocess/intf_postprocess_effect.h>

SCENE_BEGIN_NAMESPACE()

/** Bloom type enum */
enum class BloomType {
    /** Normal, smooth to every direction */
    NORMAL = 0,
    /** Blurred/Blooms more in horizontal direction */
    HORIZONTAL = 1,
    /** Blurred/Blooms more in vertical direction */
    VERTICAL = 2,
    /** Bilateral filter, uses depth if available */
    BILATERAL = 3,
};

class IBloom : public IPostProcessEffect {
    META_INTERFACE(IPostProcessEffect, IBloom, "8020311e-8724-4a20-be99-e46cf667b505")
public:
    /**
     * @brief Camera postprocessing settings, bloom type
     * @return
     */
    META_PROPERTY(BloomType, Type)
    /**
     * @brief Camera postprocessing settings, bloom quality type
     * @return
     */
    META_PROPERTY(EffectQualityType, Quality)
    /**
     * @brief Camera postprocessing settings, bloom threshold hard
     * @return
     */
    META_PROPERTY(float, ThresholdHard)
    /**
     * @brief Camera postprocessing settings, bloom threshold soft
     * @return
     */
    META_PROPERTY(float, ThresholdSoft)
    /**
     * @brief Camera postprocessing settings, bloom amount coefficient
     * @return
     */
    META_PROPERTY(float, AmountCoefficient)
    /**
     * @brief Camera postprocessing settings
     * @return
     */
    META_PROPERTY(float, DirtMaskCoefficient)
    /**
     * @brief Camera postprocessing settings
     * @return
     */
    META_PROPERTY(IBitmap::Ptr, DirtMaskImage)
    /**
     * @brief Camera postprocessing settings
     * @return
     */
    META_PROPERTY(bool, UseCompute)
    /**
     * @brief Scatter (amount of bloom spread). (1.0 full spread / default)
     * @return
     */
    META_PROPERTY(float, Scatter)
    /** @brief Scaling factor. Controls the amount of scaling and bloom spread
     * Reduces the downscale and upscale steps
     * Values 0 - 1. Value of 0.5 halves the scale steps
     * @return
     */
    META_PROPERTY(float, ScaleFactor)
};

META_REGISTER_CLASS(Bloom, "6718b07d-c3d1-4036-bd0f-88d1380b846a", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::BloomType)
META_INTERFACE_TYPE(SCENE_NS::IBloom)

#endif
