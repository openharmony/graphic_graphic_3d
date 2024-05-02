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

#ifndef META_INTERFACE_IOBJECT_CONTEXT_H
#define META_INTERFACE_IOBJECT_CONTEXT_H

#include <meta/base/namespace.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IObjectContext, "1f8bbc62-3d3e-4193-b3bf-8a4f65e55625")
META_REGISTER_INTERFACE(IObjectContextProvider, "940331af-d8b7-4865-bb69-a15cba347218")

/**
 * @brief The IObjectContext interface defines the base interface for
 *        and object-specific object context.
 */
class IObjectContext : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IObjectContext)
public:
    /**
     * @brief Returns the ObjectRegistry instance which should be returned
     *        by this context.
     */
    virtual IObjectRegistry& GetObjectRegistry() = 0;
};

META_END_NAMESPACE()

META_INTERFACE_TYPE(META_NS::IObjectContext);

META_BEGIN_NAMESPACE()

/**
 * @brief The IObjectContextProvider interface defines an interface which can
 *        be implemented by objects that have an object context.
 */
class IObjectContextProvider : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IObjectContextProvider)
public:
    /**
     * @brief Returns the ObjectContext property.
     */
    META_PROPERTY(IObjectContext::Ptr, ObjectContext);
    /**
     * @brief Resets the object context to a default value,
     * @note Usually the default value is the object context returned
     *       by META_NS::GetDefaultObjectContext
     */
    virtual void ResetObjectContext() = 0;
};

META_END_NAMESPACE()

#endif
