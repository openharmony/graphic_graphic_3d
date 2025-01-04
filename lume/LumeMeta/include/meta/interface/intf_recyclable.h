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

#ifndef META_INTERFACE_IRECYCLABLE_H
#define META_INTERFACE_IRECYCLABLE_H

#include <meta/interface/intf_metadata.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IRecyclable, "47ded8f2-b790-47af-a576-88e5b157a179")

/**
 * @brief The IRecyclable interface to indicate entity can be recycled
 */
class IRecyclable : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IRecyclable)
public:
    /**
     * @brief Build the object again when recycling (see ILifecycle for Build).
     */
    virtual bool ReBuild(const IMetadata::Ptr&) = 0;
    /**
     * @brief Dispose the object before it can be re-used.
     */
    virtual void Dispose() = 0;
};

META_END_NAMESPACE()

#endif
