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

class JsonOutput : public Internal::BaseObjectFwd<JsonOutput, META_NS::ClassId::JsonOutput, ISerOutput> {
public:
    BASE_NS::string Process(const ISerNode::Ptr& tree) override;
};

} // namespace Serialization

META_END_NAMESPACE()

#endif
