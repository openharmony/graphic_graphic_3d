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

#ifndef SCENE_INTERFACE_POSTPROCESS_ITONEMAP_H
#define SCENE_INTERFACE_POSTPROCESS_ITONEMAP_H

#include <scene/base/types.h>
#include <scene/interface/postprocess/intf_postprocess_effect.h>

SCENE_BEGIN_NAMESPACE()

enum class TonemapType {
    /** Aces */
    ACES = 0,
    /** Aces 2020 */
    ACES_2020 = 1,
    /** Filmic */
    FILMIC = 2,
    /** PBRNeutral */
    TONEMAP_PBR_NEUTRAL = 3,
};

class ITonemap : public IPostProcessEffect {
    META_INTERFACE(IPostProcessEffect, ITonemap, "aec077b9-42bf-49e3-87f4-d3b7e821768f")
public:
    /**
     * @brief Tonemap type.
     */
    META_PROPERTY(TonemapType, Type)
    /**
     * @brief Tonemap exposure.
     */
    META_PROPERTY(float, Exposure)
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::TonemapType)
META_INTERFACE_TYPE(SCENE_NS::ITonemap)

#endif
