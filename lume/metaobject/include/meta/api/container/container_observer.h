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

#ifndef META_API_CONTAINER_OBSERVER_H
#define META_API_CONTAINER_OBSERVER_H

#include <meta/api/internal/object_api.h>
#include <meta/api/make_callback.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_container_observer.h>

META_BEGIN_NAMESPACE()

/**
 * @brief The ContainerObserver class provides a wrapper for META_NS::ClassId::ContainerObserver.
 */
class ContainerObserver final
    : public Internal::ObjectInterfaceAPI<ContainerObserver, META_NS::ClassId::ContainerObserver> {
    META_API(ContainerObserver)
    META_API_CACHE_INTERFACE(META_NS::IContainerObserver, Observer)
public:
    /**
     * @brief See IContainerObserver::SetContainer.
     */
    auto& Container(const IContainer::Ptr& container)
    {
        META_API_CACHED_INTERFACE(Observer)->SetContainer(container);
        return *this;
    }
    /**
     * @brief See IContainerObserver::OnDescendantAdded.
     */
    template<class Callback>
    auto& OnDescendantAdded(Callback&& callback)
    {
        META_API_CACHED_INTERFACE(Observer)->OnDescendantAdded()->AddHandler(
            META_NS::MakeCallback<META_NS::IOnChildChanged>(BASE_NS::forward<Callback>(callback)));
        return *this;
    }
    /**
     * @brief See IContainerObserver::OnDescendantRemoved.
     */
    template<class Callback>
    auto& OnDescendantRemoved(Callback&& callback)
    {
        META_API_CACHED_INTERFACE(Observer)->OnDescendantRemoved()->AddHandler(
            META_NS::MakeCallback<META_NS::IOnChildChanged>(BASE_NS::forward<Callback>(callback)));
        return *this;
    }
    /**
     * @brief See IContainerObserver::OnDescendantMoved.
     */
    template<class Callback>
    auto& OnDescendantMoved(Callback&& callback)
    {
        META_API_CACHED_INTERFACE(Observer)->OnDescendantMoved()->AddHandler(
            META_NS::MakeCallback<META_NS::IOnChildMoved>(BASE_NS::forward<Callback>(callback)));
        return *this;
    }
};

META_END_NAMESPACE()

#endif // META_API_CONTAINER_OBSERVER_H
