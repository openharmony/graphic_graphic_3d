/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SCENE_INTERFACE_IIN_USE_H
#define SCENE_INTERFACE_IIN_USE_H

#include <scene/base/namespace.h>

#include <meta/interface/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief The IInUse interface is implemented by classes which can provide information about the object instance being
 *        in active use.
 */
class IInUse : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IInUse, "acf174cb-3f7b-46da-8a5f-b990e16cbd98")
public:
    /**
     * @brief Returns true if the object is in active use, false otherwise.
     */
    virtual bool IsInUse() const = 0;
    /**
     * @brief Set the in use state.
     * @param inUse The state to set.
     */
    virtual void SetInUse(bool inUse) = 0;
};

SCENE_END_NAMESPACE()

#endif  // SCENE_INTERFACE_IIN_USE_H
