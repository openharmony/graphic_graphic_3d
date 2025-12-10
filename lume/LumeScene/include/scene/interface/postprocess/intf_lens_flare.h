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

#ifndef SCENE_INTERFACE_POSTPROCESS_ILENS_FLARE_H
#define SCENE_INTERFACE_POSTPROCESS_ILENS_FLARE_H

#include <scene/base/types.h>
#include <scene/interface/postprocess/intf_postprocess_effect.h>

SCENE_BEGIN_NAMESPACE()

class ILensFlare : public IPostProcessEffect {
    META_INTERFACE(IPostProcessEffect, ILensFlare, "7a7f4353-8050-4a69-83c1-abdb8b8fe4d7")
public:
    /**
     * @brief Lens flare quality.
     */
    META_PROPERTY(EffectQualityType, Quality)
    /**
     * @brief Lens flare intensity.
     */
    META_PROPERTY(float, Intensity)
    /**
     * @brief Lens flare position.
     */
    META_PROPERTY(BASE_NS::Math::Vec3, FlarePosition)
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::ILensFlare)

#endif // SCENE_INTERFACE_POSTPROCESS_ILENS_FLARE_H
