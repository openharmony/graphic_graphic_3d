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

#ifndef META_API_OBJECT_HIERARCHY_OBSERVER_H
#define META_API_OBJECT_HIERARCHY_OBSERVER_H

#include <meta/api/internal/object_api.h>
#include <meta/api/make_callback.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_object_hierarchy_observer.h>

META_BEGIN_NAMESPACE()

/**
 * @brief The ObjectHierarchyObserver class provides a wrapper for META_NS::ClassId::ObjectHierarchyObserver.
 */
class ObjectHierarchyObserver final
    : public Internal::ObjectInterfaceAPI<ObjectHierarchyObserver, META_NS::ClassId::ObjectHierarchyObserver> {
    META_API(ObjectHierarchyObserver)
    META_API_CACHE_INTERFACE(META_NS::IObjectHierarchyObserver, Observer)
public:
    /**
     * @brief See IObjectHierarchyObserver::SetTarget.
     */
    auto& Target(const IObject::Ptr& root)
    {
        META_API_CACHED_INTERFACE(Observer)->SetTarget(root);
        return *this;
    }
    auto& Target(const IObject::Ptr& root, HierarchyChangeModeValue mode)
    {
        META_API_CACHED_INTERFACE(Observer)->SetTarget(root, mode);
        return *this;
    }
    /**
     * @brief See IObjectHierarchyObserver::GetTarget.
     */
    IObject::Ptr GetTarget() const
    {
        return META_API_CACHED_INTERFACE(Observer)->GetTarget();
    }
    /**
     * @brief See IObjectHierarchyObserver::GetAllObserved.
     */
    BASE_NS::vector<IObject::Ptr> GetAllObserved() const
    {
        return META_API_CACHED_INTERFACE(Observer)->GetAllObserved();
    }
    template<class T>
    BASE_NS::vector<typename T::Ptr> GetAllObserved() const
    {
        return PtrArrayCast<T>(GetAllObserved());
    }

    /**
     * @brief See IContainerObserver::OnHierarchyChanged.
     */
    template<class Callback>
    auto& OnHierarchyChanged(Callback&& callback)
    {
        META_API_CACHED_INTERFACE(Observer)->OnHierarchyChanged()->AddHandler(
            META_NS::MakeCallback<META_NS::IOnHierarchyChanged>(BASE_NS::forward<Callback>(callback)));
        return *this;
    }
};

META_END_NAMESPACE()

#endif // META_API_OBJECT_HIERARCHY_OBSERVER_H
