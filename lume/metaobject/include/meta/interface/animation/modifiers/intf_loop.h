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

#ifndef META_INTERFACE_ANIMATION_MODIFIERS_INTF_LOOP_H
#define META_INTERFACE_ANIMATION_MODIFIERS_INTF_LOOP_H

#include <meta/base/meta_types.h>
#include <meta/interface/interface_macros.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(ILoop, "08f22131-c65d-4c12-b351-5fc784aa5278")

namespace AnimationModifiers {

/**
 * @brief Defines interface for the loop animation modifier
 */
class ILoop : public CORE_NS::IInterface {
    META_INTERFACE(IInterface, ILoop)
public:
    /**
     * @brief The number of times the animation should loop. Default 1.
     *        *  0: Animation is not run at all.
     *        * >0: Loop the animation this many times.
     *        * <0: Loop the animation indefinitely.
     */
    META_PROPERTY(int32_t, LoopCount)
};

} // namespace AnimationModifiers

META_END_NAMESPACE()

META_TYPE(META_NS::AnimationModifiers::ILoop::Ptr)

#endif // META_INTERFACE_ANIMATION_MODIFIERS_INTF_LOOP_H
