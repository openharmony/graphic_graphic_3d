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

#ifndef META_INTERFACE_ITICKABLE_H
#define META_INTERFACE_ITICKABLE_H

#include <meta/base/interface_macros.h>
#include <meta/base/time_span.h>

META_BEGIN_NAMESPACE()

REGISTER_INTERFACE(ITickable, "4ff70521-002a-4f64-8b42-20a82a11947d")

/**
 * @brief The ITickable interface can be implemented by startable objects who want notifcation
 *        from the startable object controller of a UI hierarchy during runtime.
 */
class ITickable : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITickable)
public:
    /**
     * @brief Called by a startable object controller.
     * @param sinceLastTick Time since previous call.
     */
    virtual void Tick(const TimeSpan& time, const TimeSpan& sinceLastTick) = 0;
};

META_END_NAMESPACE()

#endif // META_INTERFACE_ITICKABLE_H
