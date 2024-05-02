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
#ifndef META_SRC_CONTAINER_H
#define META_SRC_CONTAINER_H

#include "container_base.h"

META_BEGIN_NAMESPACE()

class Container : public ContainerBase {
public: // IContainer
    IObject::Ptr FindAny(const META_NS::IContainer::FindOptions& options) const override;
    BASE_NS::vector<IObject::Ptr> FindAll(const META_NS::IContainer::FindOptions& options) const override;
    bool Add(const META_NS::IObject::Ptr& object) override;
    bool Insert(SizeType index, const IObject::Ptr& object) override;
    bool Replace(const META_NS::IObject::Ptr& child, const META_NS::IObject::Ptr& replaceWith, bool addAlways) override;

private:
    bool CheckLoop(const IObject::Ptr& object) const;
    void SetObjectParent(const IObject::Ptr& object, const IObject::Ptr& parent) const override;
};

META_END_NAMESPACE()

#endif // META_SRC_OBJECT_CONTAINER_H
