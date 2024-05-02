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

#ifndef META_SRC_CONNECTOR_H
#define META_SRC_CONNECTOR_H

#include <meta/ext/attachment/attachment.h>
#include <meta/interface/intf_connector.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/serialization/intf_serializable.h>

META_BEGIN_NAMESPACE()

class Connector : public META_NS::AttachmentFwd<Connector, ClassId::Connector, IConnector, ISerializable> {
public:
    bool AttachTo(const META_NS::IAttach::Ptr& target, const META_NS::IObject::Ptr& dataContext) override;
    bool DetachFrom(const META_NS::IAttach::Ptr& target) override;

    bool Connect(const IObject::Ptr& source, const BASE_NS::string& event, const BASE_NS::string& func) override;
    IObject::Ptr GetSource() const override;
    BASE_NS::string GetEventName() const override;
    BASE_NS::string GetFunctionName() const override;

    ReturnError Export(IExportContext&) const override;
    ReturnError Import(IImportContext&) override;

private:
    IEvent::Token handle_ {};
    RefUri uri_;
    IMetadata::WeakPtr source_;
    BASE_NS::string eventName_;
    BASE_NS::string funcName_;
};

META_END_NAMESPACE()

#endif
