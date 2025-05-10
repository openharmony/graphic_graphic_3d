/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: Object name
 * Author: Mikael Kilpeläinen
 * Create: 2025-01-14
 */
#ifndef META_SRC_OBJECT_NAME_H
#define META_SRC_OBJECT_NAME_H

#include <meta/ext/base_object.h>
#include <meta/ext/serialization/serializer.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_named.h>
#include <meta/interface/serialization/intf_serializable.h>

META_BEGIN_NAMESPACE()
namespace Internal {
class ObjectName : public IntroduceInterfaces<BaseObject, IObjectName, ISerializable> {
    META_OBJECT(ObjectName, META_NS::ClassId::ObjectName, IntroduceInterfaces)
public:
    BASE_NS::string GetName() const override
    {
        return name_;
    }
    bool SetName(BASE_NS::string_view name) override
    {
        name_ = name;
        return true;
    }

    ReturnError Export(IExportContext& c) const override
    {
        return Serializer(c) & NamedValue("Name", name_);
    }
    ReturnError Import(IImportContext& c) override
    {
        return Serializer(c) & NamedValue("Name", name_);
    }

private:
    BASE_NS::string name_;
};
} // namespace Internal

META_END_NAMESPACE()

#endif
