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

#ifndef META_INTERFACE_IOBJECT_HIERARCHY_OBSERVER_H
#define META_INTERFACE_IOBJECT_HIERARCHY_OBSERVER_H

#include <meta/base/bit_field.h>
#include <meta/interface/intf_container.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IObjectHierarchyObserver, "6ffa83ee-716a-4cff-aa09-b87f84c97056")
META_REGISTER_INTERFACE(IOnHierarchyChanged, "e7dfaaee-79fa-4dbe-9fb0-1e89a1025183")

struct IOnHierarchyChangedInfo {
    constexpr static BASE_NS::Uid UID { META_NS::InterfaceId::IOnHierarchyChanged };
    constexpr static char const* NAME { "OnHierarchyChanged" };
};

/**
 * @brief The HierarchyChangeType enum lists operations that IObjectHierarchyObserver can monitor.
 */
enum class HierarchyChangeType : uint32_t {
    /** An object is about to be added to the hierarchy being monitored */
    ADDING = 1,
    /** An object was added to the hierarchy being monitored */
    ADDED = 2,
    /** An object is about to be removed from the hierarchy being monitored */
    REMOVING = 3,
    /** An object was removed from the hierarchy being monitored */
    REMOVED = 4,
    /** An object was moved inside a container in the hierarchy being monitored */
    MOVED = 5,
    /** An object changed */
    CHANGED = 6
};

/**
 * @brief The HierarchyChangeObjectType enum lists different object types within the hierarchy.
 */
enum class HierarchyChangeObjectType : uint32_t {
    /** The root object */
    ROOT = 1,
    /** The object is a child of an IContainer */
    CHILD = 2,
    /** The object is the content of IContent::Content */
    CONTENT = 3,
    /** The object is an attachment */
    ATTACHMENT = 4,
};

enum class HierarchyChangeMode : uint8_t {
    /** Get notifications for IContainer changes */
    NOTIFY_CONTAINER = 1,
    /** Get notifications for IContent::Content changes */
    NOTIFY_CONTENT = 2,
    /** Get notifications for IAttachment changes */
    NOTIFY_ATTACHMENT = 4,
    /** Get Notifications for INotifyOnChange changes for the object itself */
    NOTIFY_OBJECT = 8,
    /** Default notify mode */
    NOTIFY_DEFAULT = NOTIFY_CONTAINER | NOTIFY_CONTENT | NOTIFY_ATTACHMENT
};

using HierarchyChangeModeValue = EnumBitField<HierarchyChangeMode>;

/**
 * @brief The ChildChangedInfo struct defines the parameters for an event which is invoked when
 *        an object is added to or removed from an IContainer.
 */
struct HierarchyChangedInfo {
    /** The object that the operation concerns */
    IObject::Ptr object;
    /** Type of change within the hierarchy */
    HierarchyChangeType change;
    /** Type of the object */
    HierarchyChangeObjectType objectType;
    /** Index of the object within the container where change happened if available, otherwise maximum value of size_t.
     *  Only valid for CHILD and ATTACHMENT object types */
    size_t index { size_t(-1) };
    /** Parent object in the hierarchy. */
    IObject::WeakPtr parent;
};

using IOnHierarchyChanged = META_NS::SimpleEvent<IOnHierarchyChangedInfo, void(const HierarchyChangedInfo&)>;

/**
 * @brief The IContainerObserver interface
 */
class IObjectHierarchyObserver : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IObjectHierarchyObserver)
public:
    /**
     * @brief Sets the root object of the hierarchy to observe.
     * @param root The root object of the hierarchy.
     * @param optional mode to use used, if not given, NOTIFY_DEFAULT is used.
     */
    virtual void SetTarget(const IObject::Ptr& root, HierarchyChangeModeValue mode) = 0;
    void SetTarget(const IObject::Ptr& root)
    {
        SetTarget(root, HierarchyChangeMode::NOTIFY_DEFAULT);
    }
    /**
     * @brief Returns the current target object.
     */
    virtual IObject::Ptr GetTarget() const = 0;
    /**
     * @brief Returns a list of all objects which are currently being monitored.
     */
    virtual BASE_NS::vector<IObject::Ptr> GetAllObserved() const = 0;
    /**
     * @brief Returns all currently monitored objects which implement a given interface.
     */
    template<class T>
    BASE_NS::vector<typename T::Ptr> GetAllObserved() const
    {
        return PtrArrayCast<T>(GetAllObserved());
    }
    /**
     * @brief Invoked when the hierarchy has changed in some way.
     */
    META_EVENT(IOnHierarchyChanged, OnHierarchyChanged)
};

META_END_NAMESPACE()

META_TYPE(META_NS::HierarchyChangedInfo)

#endif // META_INTERFACE_IOBJECT_HIERARCHY_OBSERVER_H
