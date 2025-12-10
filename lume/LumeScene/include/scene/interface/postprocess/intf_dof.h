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

#ifndef SCENE_INTERFACE_POSTPROCESS_IDOF_H
#define SCENE_INTERFACE_POSTPROCESS_IDOF_H

#include <scene/base/types.h>
#include <scene/interface/postprocess/intf_postprocess_effect.h>

SCENE_BEGIN_NAMESPACE()

class IDepthOfField : public IPostProcessEffect {
    META_INTERFACE(IPostProcessEffect, IDepthOfField, "c578779f-7f70-466b-8a61-92d90cb284e5")
public:
    /**
     * @brief Distance to point of focus.
     */
    META_PROPERTY(float, FocusPoint)
    /**
     * @brief Range around focusPoint which is in focus.
     */
    META_PROPERTY(float, FocusRange)
    /**
     * @brief Range before focusRange where the view transitions from blurred to focused.
     */
    META_PROPERTY(float, NearTransitionRange)
    /**
     * @brief Range after focusRange where the view transitions from focused to blurred.
     */
    META_PROPERTY(float, FarTransitionRange)
    /**
     * @brief Blur level used close to the viewer.
     */
    META_PROPERTY(float, NearBlur)
    /**
     * @brief Blur level used far away from the viewer
     */
    META_PROPERTY(float, FarBlur)
    /**
     * @brief View near plane.
     */
    META_PROPERTY(float, NearPlane)
    /**
     * @brief View far plane.
     */
    META_PROPERTY(float, FarPlane)
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IDepthOfField)

#endif // SCENE_INTERFACE_POSTPROCESS_IDOF_H
