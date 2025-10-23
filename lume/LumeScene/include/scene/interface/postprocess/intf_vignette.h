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

#ifndef SCENE_INTERFACE_POSTPROCESS_IVIGNETTE_H
#define SCENE_INTERFACE_POSTPROCESS_IVIGNETTE_H

#include <scene/base/types.h>
#include <scene/interface/postprocess/intf_postprocess_effect.h>

SCENE_BEGIN_NAMESPACE()

class IVignette : public IPostProcessEffect {
    META_INTERFACE(IPostProcessEffect, IVignette, "bf3a09bd-cc53-48c7-8e5d-0a6e7bf327fd")
public:
    /**
     * @brief Vignette coefficient.
     */
    META_PROPERTY(float, Coefficient)
    /**
     * @brief Vignette power.
     */
    META_PROPERTY(float, Power)
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IVignette)

#endif // SCENE_INTERFACE_POSTPROCESS_IVIGNETTE_H