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
#ifndef META_SRC_OBJECT_CONTAINER_H
#define META_SRC_OBJECT_CONTAINER_H

#include <meta/base/namespace.h>

#include "container.h"

META_BEGIN_INTERNAL_NAMESPACE()

class ObjectContainer : public IntroduceInterfaces<MetaObject, Container, IContainerPreTransaction> {
    META_OBJECT(ObjectContainer, ClassId::ObjectContainer, IntroduceInterfaces)
protected:
    bool Build(const IMetadata::Ptr& data) override;
    void Destroy() override;

    bool Add(const IObject::Ptr& object) override;
    bool Insert(SizeType index, const IObject::Ptr& object) override;
    bool Replace(const IObject::Ptr& child, const IObject::Ptr& replaceWith, bool addAlways) override;

    bool Remove(SizeType index) override;
    bool Remove(const IObject::Ptr& object) override;
    void RemoveAll() override;

private:
    IEvent::Ptr GetAboutToChange();
    bool CallAboutToChange(ContainerChangeType type, const IObject::Ptr& object);

public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_EVENT_DATA(IContainer, IOnChildChanged, OnContainerChanged)
    META_STATIC_EVENT_DATA(IContainerPreTransaction, IOnChildChanging, ContainerAboutToChange)
    META_END_STATIC_DATA()
    META_IMPLEMENT_EVENT(IOnChildChanged, OnContainerChanged)
    META_IMPLEMENT_EVENT(IOnChildChanging, ContainerAboutToChange)
};

META_END_INTERNAL_NAMESPACE()

#endif // META_SRC_OBJECT_CONTAINER_H
