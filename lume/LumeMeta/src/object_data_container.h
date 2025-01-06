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
#ifndef META_SRC_OBJECT_DATA_CONTAINER_H
#define META_SRC_OBJECT_DATA_CONTAINER_H

#include <meta/interface/intf_metadata.h>

#include "attachment_container.h"

META_BEGIN_NAMESPACE()

namespace Internal {

class ObjectDataContainer : public IntroduceInterfaces<AttachmentContainer, IMetadata> {
    using Super = IntroduceInterfaces;
    META_IMPLEMENT_OBJECT_TYPE_INTERFACE(ClassId::ObjectDataContainer)

public: // IMetadata
    bool AddFunction(const IFunction::Ptr&) override;
    bool RemoveFunction(const IFunction::Ptr&) override;

    bool AddProperty(const IProperty::Ptr&) override;
    bool RemoveProperty(const IProperty::Ptr&) override;

    bool AddEvent(const IEvent::Ptr&) override;
    bool RemoveEvent(const IEvent::Ptr&) override;

    BASE_NS::vector<IProperty::Ptr> GetProperties() override;
    BASE_NS::vector<IProperty::ConstPtr> GetProperties() const override;
    BASE_NS::vector<IFunction::Ptr> GetFunctions() override;
    BASE_NS::vector<IFunction::ConstPtr> GetFunctions() const override;
    BASE_NS::vector<IEvent::Ptr> GetEvents() override;
    BASE_NS::vector<IEvent::ConstPtr> GetEvents() const override;

    BASE_NS::vector<MetadataInfo> GetAllMetadatas(MetadataType types) const override;

    using IMetadata::GetProperty;
    IProperty::Ptr GetProperty(BASE_NS::string_view name, MetadataQuery) override;
    IProperty::ConstPtr GetProperty(BASE_NS::string_view name, MetadataQuery) const override;

    using IMetadata::GetFunction;
    IFunction::Ptr GetFunction(BASE_NS::string_view name, MetadataQuery) override;
    IFunction::ConstPtr GetFunction(BASE_NS::string_view name, MetadataQuery) const override;

    using IMetadata::GetEvent;
    IEvent::ConstPtr GetEvent(BASE_NS::string_view name, MetadataQuery) const override;
    IEvent::Ptr GetEvent(BASE_NS::string_view name, MetadataQuery) override;
};

} // namespace Internal

META_END_NAMESPACE()

#endif
