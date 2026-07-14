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

#ifndef META_SRC_SERIALIZATION_BACKEND_JSON_INPUT_H
#define META_SRC_SERIALIZATION_BACKEND_JSON_INPUT_H

#include <core/json/json2.h>

#include <meta/base/namespace.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/serialization/intf_ser_input.h>

#include "../../base_object.h"

META_BEGIN_NAMESPACE()

namespace Serialization {

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

    ISerNode::Ptr ImportRef(const CORE_NS::json2::view& ref);
    ISerNode::Ptr ImportObject(const CORE_NS::json2::view& value);
    ISerNode::Ptr ImportArray(const CORE_NS::json2::view::array_t& arr);
    ISerNode::Ptr Import(const CORE_NS::json2::view& value);
    bool ReadMetadata(const CORE_NS::json2::view& value);
    ISerNode::Ptr ImportRootObject(const CORE_NS::json2::view& value);

    Version GetVersion() const
    {
        return metaVersion_;
    }

private:
    void SetMetaV1Compatibility();

private:
    Version metaVersion_;
    SerMetadata metadata_;
    uint32_t depth_{};
};

}  // namespace Serialization

META_END_NAMESPACE()

#endif
