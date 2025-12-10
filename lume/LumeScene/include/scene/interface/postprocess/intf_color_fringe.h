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

#ifndef SCENE_INTERFACE_POSTPROCESS_ICOLOR_FRINGE_H
#define SCENE_INTERFACE_POSTPROCESS_ICOLOR_FRINGE_H

#include <scene/base/types.h>
#include <scene/interface/postprocess/intf_postprocess_effect.h>

SCENE_BEGIN_NAMESPACE()

class IColorFringe : public IPostProcessEffect {
    META_INTERFACE(IPostProcessEffect, IColorFringe, "8b95f17a-6c31-4fc5-b63e-f0034c1ecc7e")
public:
    /**
     * @brief Color fringe distance coefficient.
     */
    META_PROPERTY(float, DistanceCoefficient)
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IColorFringe)

#endif // SCENE_INTERFACE_POSTPROCESS_ICOLOR_FRINGE_H
