
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
     * @brief Camera post-processing settings, bloom type
     */
    META_PROPERTY(BloomType, Type)
    /**
     * @brief Camera post-processing settings, bloom quality type
     */
    META_PROPERTY(EffectQualityType, Quality)
    /**
     * @brief Camera post-processing settings, bloom threshold hard
     */
    META_PROPERTY(float, ThresholdHard)
    /**
     * @brief Camera post-processing settings, bloom threshold soft
     */
    META_PROPERTY(float, ThresholdSoft)
    /**
     * @brief Camera post-processing settings, bloom amount coefficient
     */
    META_PROPERTY(float, AmountCoefficient)
    /**
     * @brief Camera post-processing settings
     */
    META_PROPERTY(float, DirtMaskCoefficient)
    /**
     * @brief Camera post-processing settings
     */
    META_PROPERTY(IImage::Ptr, DirtMaskImage)
    /**
     * @brief Camera post-processing settings
     */
    META_PROPERTY(bool, UseCompute)
    /**
     * @brief Scatter (amount of bloom spread). (1.0 full spread / default)
     */
    META_PROPERTY(float, Scatter)
    /** @brief Scaling factor. Controls the amount of scaling and bloom spread
     * Reduces the downscale and upscale steps
     * Values 0 - 1. Value of 0.5 halves the scale steps
     */
    META_PROPERTY(float, ScaleFactor)
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::BloomType)
META_INTERFACE_TYPE(SCENE_NS::IBloom)

#endif
