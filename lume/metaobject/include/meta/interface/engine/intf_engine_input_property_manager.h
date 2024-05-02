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

#ifndef META_ENGINE_INTERFACE_ENGINE_INPUT_PROPERTY_MANAGER_H
#define META_ENGINE_INTERFACE_ENGINE_INPUT_PROPERTY_MANAGER_H

#include <meta/interface/engine/intf_engine_value_manager.h>
#include <meta/interface/property/property.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IEngineInputPropertyManager, "db3adbf0-4caf-4b7b-8850-ee1f0936beab")

class IEngineInputPropertyManager : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEngineInputPropertyManager)
public:
    virtual bool Sync() = 0;

    virtual IProperty::Ptr ConstructProperty(BASE_NS::string_view name) = 0;
    virtual IProperty::Ptr TieProperty(const IProperty::Ptr&, BASE_NS::string valueName) = 0;
    virtual BASE_NS::vector<IProperty::Ptr> GetAllProperties() const = 0;
    virtual bool PopulateProperties(IMetadata&) = 0;
    virtual void RemoveProperty(BASE_NS::string_view name) = 0;

    virtual IEngineValueManager::Ptr GetValueManager() const = 0;

    template<typename Type>
    Property<Type> ConstructProperty(BASE_NS::string_view name)
    {
        return Property<Type>(ConstructProperty(name));
    }
};

META_INTERFACE_TYPE(IEngineInputPropertyManager);
META_END_NAMESPACE()

#endif
