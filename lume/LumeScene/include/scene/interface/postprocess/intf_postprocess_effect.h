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

#ifndef SCENE_INTERFACE_POSTPROCESS_IPOSTPROCESS_EFFECT_H
#define SCENE_INTERFACE_POSTPROCESS_IPOSTPROCESS_EFFECT_H

#include <scene/base/types.h>

SCENE_BEGIN_NAMESPACE()

class IPostProcessEffect : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IPostProcessEffect, "7e8243b4-b6c8-482d-9ef4-0ad2828618e2")
public:
    META_PROPERTY(bool, Enabled)
};

enum class EffectQualityType {
    /** Low quality */
    LOW = 0,
    /** Normal quality */
    NORMAL = 1,
    /** High quality */
    HIGH = 2,
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::EffectQualityType)
META_INTERFACE_TYPE(SCENE_NS::IPostProcessEffect)

#endif
