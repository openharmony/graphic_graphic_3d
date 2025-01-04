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

#ifndef META_ENGINE_INTERFACE_ENGINE_TYPE_H
#define META_ENGINE_INTERFACE_ENGINE_TYPE_H

#include <core/property/property.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Interface to access engine type info
 */
class IEngineType : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEngineType, "887149a1-8612-43de-98c0-1ed0053acb7e")
public:
    /// Get Core type declaration for associated type
    virtual CORE_NS::PropertyTypeDecl GetTypeDecl() const = 0;
};

META_END_NAMESPACE()

#endif
