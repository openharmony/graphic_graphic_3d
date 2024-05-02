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

#ifndef META_INTERFACE_ICLOCK_H
#define META_INTERFACE_ICLOCK_H

#include <cstdint>

#include <core/plugin/intf_interface.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/base/shared_ptr.h>
#include <meta/base/time_span.h>
#include <meta/base/types.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IClock, "a3021cdd-379e-45aa-ab0a-4d89e8657d0f")

/**
 * @brief Interface to get current time (current time of specific clock).
 */
class IClock : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IClock)
public:
    /**
     * @brief Get current time.
     */
    virtual TimeSpan GetTime() const = 0;
};

META_END_NAMESPACE()

#endif
