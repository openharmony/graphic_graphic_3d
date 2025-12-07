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

#ifndef SCENE_INTERFACE_POSTPROCESS_IFXAA_H
#define SCENE_INTERFACE_POSTPROCESS_IFXAA_H

#include <scene/base/types.h>
#include <scene/interface/postprocess/intf_postprocess_effect.h>

SCENE_BEGIN_NAMESPACE()

class IFxaa : public IPostProcessEffect {
    META_INTERFACE(IPostProcessEffect, IFxaa, "9a270f3f-f3d6-4bb2-b005-9ee33c74a726")
public:
    /**
     * @brief FXAA overall quality.
     */
    META_PROPERTY(EffectQualityType, Quality)
    /**
     * @brief FXAA edge sharpness.
     */
    META_PROPERTY(EffectSharpnessType, Sharpness)
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IFxaa)

#endif // SCENE_INTERFACE_POSTPROCESS_IFXAA_H
