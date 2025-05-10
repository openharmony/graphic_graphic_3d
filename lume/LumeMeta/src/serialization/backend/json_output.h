/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2024. All rights reserved.
 * Description: JSON output for serialization
 * Author: Mikael Kilpeläinen
 * Create: 2024-01-08
 */

#ifndef META_SRC_SERIALIZATION_BACKEND_JSON_OUTPUT_H
#define META_SRC_SERIALIZATION_BACKEND_JSON_OUTPUT_H

#include <core/json/json.h>

#include <meta/base/namespace.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/serialization/intf_ser_output.h>

#include "../../base_object.h"

META_BEGIN_NAMESPACE()

namespace Serialization {

namespace json = CORE_NS::json;
using json_value = CORE_NS::json::standalone_value;

class JsonOutput : public IntroduceInterfaces<BaseObject, ISerOutput> {
    META_OBJECT(JsonOutput, ClassId::JsonOutput, IntroduceInterfaces)
public:
    BASE_NS::string Process(const ISerNode::Ptr& tree) override;
};

} // namespace Serialization

META_END_NAMESPACE()

#endif
