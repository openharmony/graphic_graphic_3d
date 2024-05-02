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
#ifndef META_SRC_OBJECT_H
#define META_SRC_OBJECT_H

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
#include <meta/interface/intf_container_query.h>
#include <meta/interface/intf_derived.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_object_context.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/object_type_info.h>

#include "meta_object.h"

META_BEGIN_NAMESPACE()

namespace Internal {

class Object : public MetaObjectFwd<Object, META_NS::ClassId::Object, IAttach, IContainerQuery> {
    using Super = MetaObjectFwd;

public:
    bool Build(const IMetadata::Ptr& data) override;
    void Destroy() override;

    // IAttach
    bool Attach(const IObject::Ptr& attachment, const IObject::Ptr& dataContext) override;
    bool Detach(const IObject::Ptr& attachment) override;
    BASE_NS::vector<IObject::Ptr> GetAttachments(const BASE_NS::vector<TypeId>& uids, bool strict) const override;
    bool HasAttachments() const override;
    IContainer::Ptr GetAttachmentContainer(bool initializeAlways) const override;

    const StaticObjectMetadata& GetStaticMetadata() const override;

    BASE_NS::vector<IContainer::Ptr> FindAllContainers(const ContainerFindOptions& options) const override;

private:
    Property<IObjectContext::Ptr> GetOrConstuctObjectContext() const;
    void ValidateAttachmentContainer() const;

    mutable META_NS::IAttachmentContainer::Ptr attachments_;
};

template<class FinalClass, const META_NS::ClassInfo& ClassInfo, class... Interfaces>
class ObjectFwd : public ConcreteBaseFwd<FinalClass, ClassInfo, META_NS::Internal::Object, Interfaces...> {
    using Super = ConcreteBaseFwd<FinalClass, ClassInfo, META_NS::Internal::Object, Interfaces...>;

public:
    IObjectRegistry& GetObjectRegistry() const override
    {
        return Object::GetObjectRegistry();
    }

    void SetMetadata(const META_NS::IMetadata::Ptr& meta) override
    {
        Super::SetMetadata(meta);
        META_NS::ConstructPropertiesFromMetadata(this, this->GetStaticObjectMetadata(), meta);
        META_NS::ConstructEventsFromMetadata(this, this->GetStaticObjectMetadata(), meta);
        META_NS::ConstructFunctionsFromMetadata(this, this->GetStaticObjectMetadata(), meta);
    }

protected:
    ObjectFwd() = default;
    ~ObjectFwd() override = default;
};

} // namespace Internal

META_END_NAMESPACE()

#endif
