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

#ifndef META_INTERFACE_SERIALIZATION_ISER_TRANSFORMATION_H
#define META_INTERFACE_SERIALIZATION_ISER_TRANSFORMATION_H

#include <meta/interface/serialization/intf_ser_node.h>

META_BEGIN_NAMESPACE()

/// Transformation for serialisation tree
class ISerTransformation : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISerTransformation, "dea20f12-e257-4204-9f38-3cb8eb4f4ced")
public:
    /// Get name of this transformation
    virtual BASE_NS::string GetName() const = 0;
    /// Get lowest version you should not apply this, that is, apply for everything less than this version.
    virtual Version ApplyForLessThanVersion() const = 0;
    /// Apply transformation to serialisation tree (might create totally new tree)
    virtual ISerNode::Ptr Process(ISerNode::Ptr) = 0;
};

META_INTERFACE_TYPE(META_NS::ISerTransformation)

META_END_NAMESPACE()

#endif
