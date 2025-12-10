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

#ifndef META_INTERFACE_REFURI_BUILDER_H
#define META_INTERFACE_REFURI_BUILDER_H

#include <meta/base/meta_types.h>
#include <meta/base/ref_uri.h>
#include <meta/interface/interface_macros.h>

META_BEGIN_NAMESPACE()

class IExporterState;

/**
 * @brief Allows to parameterise how ref uri is built when exporting e.g. weak pointers
 */
class IRefUriBuilder : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IRefUriBuilder, "1ad0a4ab-737b-47c5-80ca-8259c34f62fe")
public:
    virtual RefUri BuildRefUri(IExporterState&, const IObject::ConstPtr& object) = 0;
};

/**
 * @brief Set anchor type for ref uri builder to stop when found such type
 */
class IRefUriBuilderAnchorType : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IRefUriBuilderAnchorType, "b28d63dc-f8fa-41c0-987c-656ffe5e1136")
public:
    virtual void AddAnchorType(const ObjectId& id) = 0;
};

META_INTERFACE_TYPE(META_NS::IRefUriBuilder)
META_INTERFACE_TYPE(META_NS::IRefUriBuilderAnchorType)

META_END_NAMESPACE()

#endif
