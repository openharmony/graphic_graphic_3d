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

#ifndef META_SRC_OBJECT_CONTEXT_H
#define META_SRC_OBJECT_CONTEXT_H

#include <meta/base/namespace.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/object_fwd.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_object_context.h>
#include <meta/interface/intf_proxy_object.h>

#include "object.h"

META_BEGIN_NAMESPACE()

namespace Internal {

class ObjectContext final : public IntroduceInterfaces<ObjectFwd, IProxyObject, IObjectContext> {
    META_OBJECT(ObjectContext, META_NS::ClassId::ObjectContext, IntroduceInterfaces, META_NS::ClassId::ProxyObject)

public:
    ObjectContext() = default;
    ~ObjectContext() override = default;

public: // ILifecycle
    void SetSuperInstance(const META_NS::IObject::Ptr& aggr, const META_NS::IObject::Ptr& super) override;

public: // IProxyObject
    META_FORWARD_PROPERTY(ProxyModeBitsValue, Mode, proxy_->Mode())
    META_FORWARD_PROPERTY(bool, Dynamic, proxy_->Dynamic())
    const IObject::Ptr GetTarget() const override;
    bool SetTarget(const IObject::Ptr& target) override;
    BASE_NS::vector<IProperty::ConstPtr> GetOverrides() const override;
    IProperty::ConstPtr GetOverride(BASE_NS::string_view name) const override;
    IProperty::Ptr SetPropertyTarget(const IProperty::Ptr& property) override;
    IProperty::ConstPtr GetProxyProperty(BASE_NS::string_view name) const override;
    void AddInternalProxy(BASE_NS::string_view propertyPropertyName, BASE_NS::string_view sourcePropertyName) override;

public: // IMetadata
    using IMetadata::GetProperty;
    IProperty::Ptr GetProperty(BASE_NS::string_view name, MetadataQuery) override;
    IProperty::ConstPtr GetProperty(BASE_NS::string_view name, MetadataQuery) const override;

    BASE_NS::vector<IProperty::Ptr> GetProperties() override;
    BASE_NS::vector<IProperty::ConstPtr> GetProperties() const override;

public: // IObjectContext
    IObjectRegistry& GetObjectRegistry() override;

private:
    META_NS::IProxyObject* proxy_ { nullptr };
    META_NS::IMetadata* metadata_ { nullptr };
    IObject::WeakPtr target_;
};

} // namespace Internal

META_END_NAMESPACE()

#endif
