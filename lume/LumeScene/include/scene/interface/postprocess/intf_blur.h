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

#ifndef SCENE_INTERFACE_POSTPROCESS_IBLUR_H
#define SCENE_INTERFACE_POSTPROCESS_IBLUR_H

#include <scene/base/types.h>
#include <scene/interface/postprocess/intf_postprocess_effect.h>

SCENE_BEGIN_NAMESPACE()

/** Blur type enum */
enum BlurType : uint32_t {
    /** Normal, smooth to every direction */
    NORMAL = 0,
    /** Blurred more in horizontal direction */
    HORIZONTAL = 1,
    /** Blurred more in vertical direction */
    VERTICAL = 2,
};

class IBlur : public IPostProcessEffect {
    META_INTERFACE(IPostProcessEffect, IBlur, "282c0b68-ce16-4e66-9ec1-02fd935a4c4d")
public:
    /**
     * @brief Blur type.
     */
    META_PROPERTY(BlurType, Type)
    /**
     * @brief Blur quality.
     */
    META_PROPERTY(EffectQualityType, Quality)
    /**
     * @brief Blur filter size in pixels.
     */
    META_PROPERTY(float, FilterSize)
    /**
     * @brief Blur quality max mip. If mips available. Only applicable to to first (few) mips.
     */
    META_PROPERTY(uint32_t, MaxMipmapLevel)
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::BlurType)
META_INTERFACE_TYPE(SCENE_NS::IBlur)

#endif // SCENE_INTERFACE_POSTPROCESS_IBLUR_H
