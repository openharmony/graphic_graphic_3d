/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: Event <-> Meta function connector
 * Author: Mikael Kilpeläinen
 * Create: 2023-04-24
 */

#ifndef META_SRC_CONNECTOR_H
#define META_SRC_CONNECTOR_H

#include <meta/base/namespace.h>
#include <meta/ext/attachment/attachment.h>
#include <meta/interface/intf_connector.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/serialization/intf_serializable.h>

META_BEGIN_NAMESPACE()

class Connector : public IntroduceInterfaces<META_NS::AttachmentFwd, IConnector, ISerializable> {
    META_OBJECT(Connector, ClassId::Connector, IntroduceInterfaces)
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
