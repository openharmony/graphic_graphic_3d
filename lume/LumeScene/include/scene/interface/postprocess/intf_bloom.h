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
#include <scene/interface/intf_image.h>
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
     * @brief Bloom type.
     */
    META_PROPERTY(BloomType, Type)
    /**
     * @brief Bloom quality.
     */
    META_PROPERTY(EffectQualityType, Quality)
    /**
     * @brief Bloom threshold hard.
     */
    META_PROPERTY(float, ThresholdHard)
    /**
     * @brief Bloom threshold soft.
     */
    META_PROPERTY(float, ThresholdSoft)
    /**
     * @brief Bloom amount coefficient
     */
    META_PROPERTY(float, AmountCoefficient)
    /**
     * @brief Bloom dirt mask coefficient.
     */
    META_PROPERTY(float, DirtMaskCoefficient)
    /**
     * @brief Bloom dirt mask image.
     */
    META_PROPERTY(IImage::Ptr, DirtMaskImage)
    /**
     * @brief If true, use compute dispatches for bloom.
     */
    META_PROPERTY(bool, UseCompute)
    /**
     * @brief Scatter (amount of bloom spread). (1.0 full spread / default)
     */
    META_PROPERTY(float, Scatter)
    /** @brief Scaling factor. Controls the amount of scaling and bloom spread.
     *         Reduces the downscale and upscale steps.
     *         Values 0 - 1. Value of 0.5 halves the scale steps.
     */
    META_PROPERTY(float, ScaleFactor)
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::BloomType)
META_INTERFACE_TYPE(SCENE_NS::IBloom)

#endif
