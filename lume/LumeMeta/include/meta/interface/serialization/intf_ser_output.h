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

#ifndef META_INTERFACE_SERIALIZATION_ISER_OUTPUT_H
#define META_INTERFACE_SERIALIZATION_ISER_OUTPUT_H

#include <meta/interface/serialization/intf_ser_node.h>

META_BEGIN_NAMESPACE()

/// Serialisation output interface
class ISerOutput : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISerOutput, "6ec3b784-34a1-4fd6-964e-4af37de4186b")
public:
    // todo: have sink interface instead of returning string
    /// Process serialisation tree to string (e.g. to json string)
    virtual BASE_NS::string Process(const ISerNode::Ptr& tree) = 0;
};

META_INTERFACE_TYPE(META_NS::ISerOutput)

META_END_NAMESPACE()

#endif
