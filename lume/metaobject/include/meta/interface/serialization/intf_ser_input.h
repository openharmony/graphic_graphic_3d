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

#ifndef META_INTERFACE_SERIALIZATION_ISER_INPUT_H
#define META_INTERFACE_SERIALIZATION_ISER_INPUT_H

#include <meta/interface/serialization/intf_ser_node.h>

META_BEGIN_NAMESPACE()

class ISerInput : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISerInput, "c33cdbb9-9913-41ab-a525-96876b66eb30")
public:
    // todo: have source interface instead of string
    virtual ISerNode::Ptr Process(BASE_NS::string_view) = 0;
};

META_INTERFACE_TYPE(ISerInput);

META_END_NAMESPACE()

#endif
