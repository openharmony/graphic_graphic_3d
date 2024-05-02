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

#include "container.h"

META_BEGIN_NAMESPACE()

class ObjectContainer : public Internal::ObjectFwd<ObjectContainer, ClassId::ObjectContainer, Container> {
    using Super = Internal::ObjectFwd<ObjectContainer, ClassId::ObjectContainer, Container>;

protected:
    bool Build(const IMetadata::Ptr& data) override;
    void Destroy() override;

protected:
    META_IMPLEMENT_INTERFACE_EVENT(IContainer, IOnChildChanged, OnAdded)
    META_IMPLEMENT_INTERFACE_EVENT(IContainer, IOnChildChanged, OnRemoved)
    META_IMPLEMENT_INTERFACE_EVENT(IContainer, IOnChildMoved, OnMoved)

    META_IMPLEMENT_INTERFACE_EVENT(IContainerPreTransaction, IOnChildChanging, OnAdding);
    META_IMPLEMENT_INTERFACE_EVENT(IContainerPreTransaction, IOnChildChanging, OnRemoving);
};

META_END_NAMESPACE()

#endif // META_SRC_OBJECT_CONTAINER_H
