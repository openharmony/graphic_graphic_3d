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

#ifndef META_SRC_SERIALIZATION_REFURI_BUILDER_H
#define META_SRC_SERIALIZATION_REFURI_BUILDER_H

#include <meta/interface/serialization/intf_refuri_builder.h>

#include "../base_object.h"

META_BEGIN_NAMESPACE()
namespace Serialization {

class RefUriBuilder : public IntroduceInterfaces<BaseObject, IRefUriBuilder, IRefUriBuilderAnchorType> {
    META_OBJECT(RefUriBuilder, ClassId::RefUriBuilder, IntroduceInterfaces)
public:
    RefUri BuildRefUri(IExporterState&, const IObject::ConstPtr& object) override;
    void AddAnchorType(const ObjectId& id) override
    {
        anchorTypes_.push_back(id);
    }

private:
    IObject::Ptr ResolveUriSegment(const IObject::ConstPtr& ptr, RefUri& uri, const IObject::ConstPtr& previous) const;
    bool IsAnchorType(const ObjectId& id) const;
    IObject::Ptr ResolveProperty(const IProperty* property, RefUri& uri, const IObject::ConstPtr& previous) const;
    IObject::Ptr ResolveObjectInstance(const IObject::ConstPtr& ptr, const IObjectInstance& instance, RefUri& uri,
        const IObject::ConstPtr& previous) const;

private:
    BASE_NS::vector<ObjectId> anchorTypes_;
};

} // namespace Serialization
META_END_NAMESPACE()

#endif
