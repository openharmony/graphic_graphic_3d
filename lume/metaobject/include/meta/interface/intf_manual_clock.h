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

#ifndef META_INTERFACE_IMANUAL_CLOCK_H
#define META_INTERFACE_IMANUAL_CLOCK_H

#include <meta/base/namespace.h>
#include <meta/base/time_span.h>
#include <meta/interface/intf_clock.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IManualClock, "85b85ed8-014e-467f-bbd9-7a8829b340a2")

/**
 * @brief Clock for which you can explicitly set the current time.
 */
class IManualClock : public IClock {
    META_INTERFACE(IClock, IManualClock)
public:
    /**
     * @brief Get current time.
     */
    [[nodiscard]] META_NS::TimeSpan GetTime() const override = 0;
    /**
     * @brief Set current time.
     */
    virtual void SetTime(const META_NS::TimeSpan& time) = 0;
    /**
     * @brief Increment the current time.
     */
    virtual void IncrementTime(const META_NS::TimeSpan& time) = 0;
};

META_END_NAMESPACE()

#endif
