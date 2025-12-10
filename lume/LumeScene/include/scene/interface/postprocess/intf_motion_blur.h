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

#ifndef SCENE_INTERFACE_POSTPROCESS_IMOTION_BLUR_H
#define SCENE_INTERFACE_POSTPROCESS_IMOTION_BLUR_H

#include <scene/base/types.h>
#include <scene/interface/postprocess/intf_postprocess_effect.h>

SCENE_BEGIN_NAMESPACE()

class IMotionBlur : public IPostProcessEffect {
    META_INTERFACE(IPostProcessEffect, IMotionBlur, "6fbeb47d-6a33-47d5-945e-9bc8088b321c")
public:
    /**
     * @brief Motion blur quality.
     */
    META_PROPERTY(EffectQualityType, Quality)
    /**
     * @brief Motion blur sharpness.
     */
    META_PROPERTY(EffectSharpnessType, Sharpness)
    /**
     * @brief Motion blur alpha blending. 1.0 -> fully motion blur sample.
     */
    META_PROPERTY(float, Alpha)
    /**
     * @brief Motion blur velocity coefficient.
     */
    META_PROPERTY(float, VelocityCoefficient)
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IMotionBlur)

#endif // SCENE_INTERFACE_POSTPROCESS_IMOTION_BLUR_H
