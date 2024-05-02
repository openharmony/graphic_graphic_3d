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
#ifndef META_INTERFACE_OBJECT_FACTORY_H
#define META_INTERFACE_OBJECT_FACTORY_H

#include <meta/interface/static_object_metadata.h>

#if defined(GetClassInfo)
#undef GetClassInfo
#endif

META_BEGIN_NAMESPACE()

class IObject;

using Interfaces = BASE_NS::vector<BASE_NS::Uid>;

META_REGISTER_INTERFACE(IClassInfo, "2f8dc0eb-2d1b-47d5-aacb-47b824d0339d")
META_REGISTER_INTERFACE(IObjectFactory, "57f8ae84-3ad1-4923-aede-ef7bab852186")

/**
 * @brief The IObjectInfo interface defines an interface for getting the metadata for a class.
 */
class IClassInfo : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IClassInfo)
public:
    /**
     * @brief Get class info for the constructed object type.
     */
    virtual const ClassInfo& GetClassInfo() const = 0;
    /**
     * @brief Get static meta data for class (properties, events, functions)
     */
    virtual const StaticObjectMetadata& GetClassStaticMetadata() const = 0;
    /**
     * @brief Get interfaces the constructed type implements.
     */
    virtual const Interfaces& GetClassInterfaces() const = 0;
};

/**
 * @brief The IObjectFactory interface defines a factory interface used by the object and class
 *        registry for managing and instantiating classes which implement IObject.
 */
class IObjectFactory : public IClassInfo {
    META_INTERFACE(IClassInfo, IObjectFactory)
public:
    /**
     * @brief Construct object. This is called by object registry when creating class instances,
     *        and should never be called by the user directly, as the returned object is not fully
     *        constructed yet.
     */
    virtual BASE_NS::shared_ptr<IObject> CreateInstance() const = 0;
};

META_END_NAMESPACE()

#endif
