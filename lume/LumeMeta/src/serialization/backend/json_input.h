/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2024. All rights reserved.
 * Description: JSON output for serialization
 * Author: Mikael Kilpeläinen
 * Create: 2024-01-08
 */

#ifndef META_SRC_SERIALIZATION_BACKEND_JSON_INPUT_H
#define META_SRC_SERIALIZATION_BACKEND_JSON_INPUT_H

#include <core/json/json.h>

#include <meta/base/namespace.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/serialization/intf_ser_input.h>

#include "../../base_object.h"

META_BEGIN_NAMESPACE()

namespace Serialization {

namespace json = CORE_NS::json;
using json_value = CORE_NS::json::standalone_value;

class JsonInput : public IntroduceInterfaces<BaseObject, ISerInput> {
    META_OBJECT(JsonInput, ClassId::JsonInput, IntroduceInterfaces)
public:
    const char* ClassNameName = "$className";
    const char* NameName = "$name";
    const char* ObjectIdName = "$classId";
    const char* InstanceIdName = "$instanceId";
    BASE_NS::string (*RewriteReservedName)(
        const BASE_NS::string_view& name) = [](auto name) { return BASE_NS::string(name); };

    ISerNode::Ptr Process(BASE_NS::string_view data) override;

    ISerNode::Ptr ImportRef(const json::value& ref);
    ISerNode::Ptr ImportObject(const json::value& value);
    ISerNode::Ptr ImportArray(const json::value::array& arr);
    ISerNode::Ptr Import(const json::value& value);
    bool ReadMetadata(const json::value& value);
    ISerNode::Ptr ImportRootObject(const json::value& value);

    Version GetVersion() const
    {
        return metaVersion_;
    }

private:
    void SetMetaV1Compatibility();

private:
    Version metaVersion_;
    SerMetadata metadata_;
};

} // namespace Serialization

META_END_NAMESPACE()

#endif
