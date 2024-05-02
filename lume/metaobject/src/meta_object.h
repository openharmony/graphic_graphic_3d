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
#ifndef META_SRC_META_OBJECT_H
#define META_SRC_META_OBJECT_H

#include <base/containers/array_view.h>
#include <base/containers/vector.h>

#include <meta/base/types.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/metadata_helpers.h>
#include <meta/ext/object_factory.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_attachment.h>
#include <meta/interface/intf_attachment_container.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object_context.h>

#include "base_object.h"

META_BEGIN_NAMESPACE()

namespace Internal {

class MetaObject : public BaseObjectFwd<MetaObject, META_NS::ClassId::MetaObject, IObjectContextProvider, IMetadata,
                       IMetadataInternal> {
    using Super = BaseObjectFwd;

public:
    static BASE_NS::vector<BASE_NS::Uid> GetInterfacesVector()
    {
        return Super::GetInterfacesVector();
    }

protected:
    // ILifecycle
    bool Build(const IMetadata::Ptr& data) override;
    void Destroy() override;
    IObjectRegistry& GetObjectRegistry() const override;
    IObject::Ptr Resolve(const RefUri& uri) const override;

    // IObjectContextProvider

    IProperty::Ptr PropertyObjectContext() override;
    IProperty::ConstPtr PropertyObjectContext() const override;
    void ResetObjectContext() override;

    // IMetadata
    IMetadata::Ptr CloneMetadata() const override;
    IContainer::Ptr GetPropertyContainer() override;
    IContainer::ConstPtr GetPropertyContainer() const override;

    void AddFunction(const IFunction::Ptr&) override;
    void RemoveFunction(const IFunction::Ptr&) override;

    void AddProperty(const IProperty::Ptr&) override;
    void RemoveProperty(const IProperty::Ptr&) override;

    void AddEvent(const IEvent::Ptr&) override;
    void RemoveEvent(const IEvent::Ptr&) override;

    void SetProperties(const BASE_NS::vector<IProperty::Ptr>& vec) override;
    void Merge(const IMetadata::Ptr& data) override;

    BASE_NS::vector<IProperty::Ptr> GetAllProperties() override;
    BASE_NS::vector<IProperty::ConstPtr> GetAllProperties() const override;
    BASE_NS::vector<IFunction::Ptr> GetAllFunctions() override;
    BASE_NS::vector<IFunction::ConstPtr> GetAllFunctions() const override;
    BASE_NS::vector<IEvent::Ptr> GetAllEvents() override;
    BASE_NS::vector<IEvent::ConstPtr> GetAllEvents() const override;

    IProperty::Ptr GetPropertyByName(BASE_NS::string_view name) override;
    IProperty::ConstPtr GetPropertyByName(BASE_NS::string_view name) const override;
    IFunction::Ptr GetFunctionByName(BASE_NS::string_view name) override;
    IFunction::ConstPtr GetFunctionByName(BASE_NS::string_view name) const override;
    IEvent::ConstPtr GetEventByName(BASE_NS::string_view name) const override;
    IEvent::Ptr GetEventByName(BASE_NS::string_view name) override;

    // IMetadataInternal
    IMetadata::Ptr GetMetadata() const override;
    void SetMetadata(const IMetadata::Ptr& meta) override;
    const StaticObjectMetadata& GetStaticMetadata() const override;

private:
    Property<IObjectContext::Ptr> GetOrConstuctObjectContext() const;

    mutable Property<IObjectContext::Ptr> objectContext_;
    IMetadata::Ptr meta_;
};

template<class FinalClass, const META_NS::ClassInfo& ClassInfo, class... Interfaces>
class MetaObjectFwd : public ConcreteBaseFwd<FinalClass, ClassInfo, META_NS::Internal::MetaObject, Interfaces...> {
    using Super = ConcreteBaseFwd<FinalClass, ClassInfo, META_NS::Internal::MetaObject, Interfaces...>;

public:
    IObjectRegistry& GetObjectRegistry() const override
    {
        return MetaObject::GetObjectRegistry();
    }

    void SetMetadata(const META_NS::IMetadata::Ptr& meta) override
    {
        Super::SetMetadata(meta);
        META_NS::ConstructPropertiesFromMetadata(this, this->GetStaticObjectMetadata(), meta);
        META_NS::ConstructEventsFromMetadata(this, this->GetStaticObjectMetadata(), meta);
        META_NS::ConstructFunctionsFromMetadata(this, this->GetStaticObjectMetadata(), meta);
    }

protected:
    MetaObjectFwd() = default;
    ~MetaObjectFwd() override = default;
};

} // namespace Internal

class MetaObject : public Internal::MetaObjectFwd<MetaObject, ClassId::MetaObject> {};

META_END_NAMESPACE()

#endif
