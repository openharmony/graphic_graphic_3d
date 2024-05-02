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

#ifndef META_INTERFACE_ICONTAINER_H
#define META_INTERFACE_ICONTAINER_H

#include <meta/api/iteration.h>
#include <meta/base/ids.h>
#include <meta/base/interface_utils.h>
#include <meta/base/object_traits.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/intf_iterable.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/property/intf_property.h>
#include <meta/interface/simple_event.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IContainer, "55ce4604-91f4-4378-9c7d-9ab0fe97b3ce")
META_REGISTER_INTERFACE(IContainerProxyParent, "d57c03fc-23ac-4cba-8f9f-0363ca781bfa")

META_REGISTER_INTERFACE(IOnChildChanged, "686e62cc-66b1-409d-821f-e5ac69e33863")
META_REGISTER_INTERFACE(IOnChildMoved, "5890fe47-4829-42bb-bdb1-03f8a399ba0e")

struct IOnChildChangedInfo {
    constexpr static BASE_NS::Uid UID { META_NS::InterfaceId::IOnChildChanged };
    constexpr static char const* NAME { "OnChildChanged" };
};

struct IOnMovedInfo {
    constexpr static BASE_NS::Uid UID { META_NS::InterfaceId::IOnChildMoved };
    constexpr static char const *NAME { "OnChildMoved" };
};

class IContainer;

/**
 * @brief The ChildChangedInfo struct defines the parameters for an event which is invoked when
 *        an object is added to or removed from an IContainer.
 */
struct ChildChangedInfo {
    IObject::Ptr object;                        ///< The object
    size_t index;                               ///< IContainer index where the change happened
    BASE_NS::weak_ptr<const IContainer> parent; ///< The parent IContainer for the change
};

/**
 * @brief The ChildChangedInfo struct defines the parameters for an event which is invoked when
 *        an object is moved in an IContainer.
 */
struct ChildMovedInfo {
    IObject::Ptr object;                        ///< The object
    size_t from;                                ///< IContainer index where the the object was moved from
    size_t to;                                  ///< IContainer index where the object was moved to
    BASE_NS::weak_ptr<const IContainer> parent; ///< The parent IContainer for the change
};

using IOnChildChanged = META_NS::SimpleEvent<IOnChildChangedInfo, void(const ChildChangedInfo&)>;
using IOnChildMoved = META_NS::SimpleEvent<IOnMovedInfo, void(const ChildMovedInfo&)>;

/**
 * @brief An interface implemented by objects that may contain other objects.
 * @note  The implementer shall use IObjectFlags::SERIALIZE_HIERARCHY flag to determine
 *        if the children should be serialized when exporting the object. By default
 *        IContainer implementations should serialize their child objects.
 */
class IContainer : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IContainer)
public:
    using SizeType = size_t;

    /**
     * @brief The FindOptions struct defines a set of options that can be used to find objects from a container.
     */
    struct FindOptions {
        /** Name of the object to find. If name is empty, only the uids are taken into account. */
        BASE_NS::string name;
        /** Search mode */
        TraversalType behavior { TraversalType::BREADTH_FIRST_ORDER };
        /** List of interface ids that the object must implement. If empty, no check is made */
        BASE_NS::vector<TypeId> uids;
        /** If true, the found object must implement all of the uids.
         *  If false, any uid is enough to consider the object as a match */
        bool strict { false };
    };
    /**
     * @brief Returns the contained children.
     */
    virtual BASE_NS::vector<IObject::Ptr> GetAll() const = 0;
    /**
     * @brief Returns the child at given index.
     * @param index Index of the child to return.
     * @return The child at given index or nullptr in case of invalid index.
     */
    virtual IObject::Ptr GetAt(SizeType index) const = 0;
    /**
     * @brief Returns the number of children in the container. Equal to GetAll().size().
     */
    virtual SizeType GetSize() const = 0;
    /**
     * @brief Find all objects matching a search criteria from the container.
     * @param options The options to use for finding.
     * @return The child objects that match the search criteria.
     */
    virtual BASE_NS::vector<IObject::Ptr> FindAll(const FindOptions& options) const = 0;
    /**
     * @brief Returns the first found object matching a search criteria from the container.
     * @note If the user is interested in finding some object that matches the criteria, it
     *       is more efficient to call FindAny rather than FindAll as the implementation
     *       can do an early exit when the first suitable object is found.
     * @param options The options to use for finding.
     * @return The first object which matches the search criteria.
     */
    virtual IObject::Ptr FindAny(const FindOptions& options) const = 0;
    /**
     * @brief Get child by name from the immediate children of the container.
     * @param name Name of the child object to find.
     * @note  Use FindAny to find from the hierarchy.
     * @return The first child object with given name or null if non found.
     */
    virtual IObject::Ptr FindByName(BASE_NS::string_view name) const = 0;
    /**
     * @brief Add child object.
     * @param object The object to add.
     * @note An object can be added to the container only once. Subsequent add operations
     *       for the same object will succeed but do not add anything to the container
     *       (since the object was already there).
     * @note Adding may fail due to e.g. the object not fulfilling the constraints set to
     *       the interfaces each object in the container should implement (defined by calling
     *       SetRequiredInterfaces).
     * @return True if the object is in the container after the operation, false otherwise.
     */
    virtual bool Add(const IObject::Ptr& object) = 0;
    /**
     * @brief Inserts a child object at given index.
     * @param index The index where the object should be added. If index >= container size, the
     *              object is added at the end of the container.
     * @param object The object to add.
     * @note  If the object is already in the container, the function will move the object
     *        to the given index.
     * @return True if the object is in the container after the operation, false otherwise.
     */
    virtual bool Insert(SizeType index, const IObject::Ptr& object) = 0;
    /**
     * @brief Remove child object.
     * @param index The index of child object to remove.
     * @return True if the object was removed from the container, false otherwise.
     */
    virtual bool Remove(SizeType index) = 0;
    /**
     * @brief Remove child object.
     * @param child The child object to remove.
     * @return True if the object was removed from the container, false otherwise.
     */
    virtual bool Remove(const IObject::Ptr& child) = 0;
    /**
     * @brief Move a child object from one index to another.
     * @note OnMoved event will be invoked only for the moved item, but not for other items in the
     *       container whose index may change as a side effect of the move operation.
     * @param fromIndex The index to move from.
     * @param toIndex The index to move the object to. The target position is the position in the
     *                container before the the object has been removed from its original position.
     * @return True if the object was successfully moved, false otherwise.
     */
    virtual bool Move(SizeType fromIndex, SizeType toIndex) = 0;
    /**
     * @brief Move a child object to a new index.
     * @note OnMoved event will be invoked only for the moved item, but not for other items in the
     *       container whose index may change as a side effect of the move operation.
     * @param child The child to move.
     * @param toIndex The index to move the object to. The target position is the position in the
     *                container before the the object has been removed from its original position.
     * @return True if the object was successfully moved, false otherwise.
     */
    virtual bool Move(const IObject::Ptr& child, SizeType toIndex) = 0;
    /**
     * @brief Replace a child item with another one. The function will fail if child is not found.
     * @note  Does nothing if the parameters point to the same object.
     * @param child The child to replace. If the child is not found from the container, does nothing or
     *        adds replaceWith to the container, depending on addAlways.
     * @param replaceWith The item to replace the child with. If null, the child is removed.
     * @return True if any changes were made in the container as a result of this call, or if child == replaceWith
     *         and child is found from the container. In other cases returns false.
     */
    bool Replace(const IObject::Ptr& child, const IObject::Ptr& replaceWith)
    {
        return Replace(child, replaceWith, false);
    }
    /**
     * @brief Replace a child item with another one.
     * @note  Does nothing if the parameters point to the same object.
     * @note  If both child and replaceWith are already in the container, child is removed and replaceWith
     *        is moved to its location.
     * @param child The child to replace. If the child is not found from the container, does nothing or
     *        adds replaceWith to the container, depending on addAlways.
     * @param replaceWith The item to replace the child with. If null, the child is removed.
     * @param addAlways If true, and if child is not found from the container, adds replaceWith to
     *        the end of the container. If false, the function will fail if child is not found.
     * @return True if any changes were made in the container as a result of this call, or if child == replaceWith
     *         and child is found from the container. In other cases returns false.
     */
    virtual bool Replace(const IObject::Ptr& child, const IObject::Ptr& replaceWith, bool addAlways) = 0;
    /**
     * @brief Removes all children from container.
     */
    virtual void RemoveAll() = 0;
    /**
     * @brief Invoked after an object was added to the container.
     */
    META_EVENT(IOnChildChanged, OnAdded)
    /**
     * @brief Invoked after an object was removed from the container.
     */
    META_EVENT(IOnChildChanged, OnRemoved)
    /**
     * @brief Invoked after an object was moved from one index to another in the container.
     */
    META_EVENT(IOnChildMoved, OnMoved)
    /**
     * @brief A helper template for finding a child item in this container that matches the name and implements
     *        the interface given as a template parameter.
     *
     *        The template parameter interface is implicitly given as a parameter to the IContainer::FindAny function.
     */
    template<class T>
    typename T::Ptr FindAny(BASE_NS::string_view name, TraversalType order) const
    {
        return interface_pointer_cast<T>(FindAny({ BASE_NS::string(name), order, { T::UID }, false }));
    }
    /**
     * @brief A helper template for finding a child item in this container that matches the name.
     */
    template<class T>
    typename T::Ptr FindByName(BASE_NS::string_view name) const
    {
        return interface_pointer_cast<T>(FindByName(name));
    }
    /**
     * @brief A helper template for finding a child item in this container that matches the name and implements
     *        the interface given as a template parameter.
     */
    template<class T>
    typename T::Ptr FindAnyFromHierarchy(BASE_NS::string_view name) const
    {
        return FindAny<T>(name, TraversalType::BREADTH_FIRST_ORDER);
    }
    /**
     * @brief Typed helper for IContainer::GetAll, returns all children which implement T.
     */
    template<class T>
    BASE_NS::vector<typename T::Ptr> GetAll() const
    {
        return PtrArrayCast<T>(GetAll());
    }
    /**
     * @brief Checks if object is part of this container's hierarchy.
     * @param object The object to check.
     * @return True if object can be found from this container's hierarchy.
     */
    virtual bool IsAncestorOf(const IObject::ConstPtr& object) const = 0;

public:
    /**
     * @brief Typed helper for IContainer::GetAt, returns object at index converted to T::Ptr.
     */
    template<class T>
    typename T::Ptr GetAt(SizeType index) const
    {
        return interface_pointer_cast<T>(GetAt(index));
    }
    /**
     * @brief Typed helper for IContainer::Add
     */
    template<class T>
    bool Add(const T& object)
    {
        return Add(interface_pointer_cast<IObject>(object));
    }
    /**
     * @brief Typed helper for IContainer::Insert
     */
    template<class T>
    bool Insert(SizeType index, const T& object)
    {
        return Insert(index, interface_pointer_cast<IObject>(object));
    }
    /**
     * @brief Typed helper for IContainer::Remove
     */
    template<class T, class = BASE_NS::enable_if_t<IsInterfacePtr_v<T>>>
    bool Remove(const T& child)
    {
        return Remove(interface_pointer_cast<IObject>(child));
    }
    /**
     * @brief Typed helper for IContainer::Replace
     */
    template<class T1, class T2>
    bool Replace(const T1& child, const T2& replaceWith, bool addAlways = false)
    {
        return Replace(interface_pointer_cast<IObject>(child), interface_pointer_cast<IObject>(replaceWith), addAlways);
    }
};

/**
 * @brief Typed helper for IContainer::GetAll. Returns all children which implement T from container.
 */
template<class T>
BASE_NS::vector<typename T::Ptr> GetAll(const META_NS::IContainer::ConstPtr& container)
{
    if (!container) {
        return {};
    }
    if constexpr (IsIObjectPtr_v<typename T::Ptr>) {
        return container->GetAll();
    } else {
        return container->GetAll<T>();
    }
}

/**
 * @brief Returns true if a container contains a given child object.
 * @param container The container to check against.
 * @param child The child to find.
 * @return True if the child can be found in the container, false otherwise.
 */
template<class T, class = BASE_NS::enable_if_t<IsInterfacePtr_v<T>>>
bool ContainsObject(const META_NS::IContainer::ConstPtr& container, const T& child)
{
    bool found = false;
    if (const auto object = interface_pointer_cast<IObject>(child); container && object) {
        IterateShared(container, [&](const IObject::Ptr& o) {
            if (o == object) {
                found = true;
            }
            return !found;
        });
    }
    return found;
}

/**
 * @brief The IContainerProxyParent interface can be used to defined a proxied parent
 *        that an IContainer implementation sets to its child objects.
 * @note  Child objects must implement IMutableContainable for the proxy to have effect.
 */
class IContainerProxyParent : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IContainerProxyParent)
public:
    /**
     * @brief Sets the object that should be used as the parent object when an IContainer
     *        implementation calls IMutableContainable::SetParent on objects that are
     *        added to the container.
     * @note  This call only affects the parent for objects added to the container
     *        after SetProxyParent is called. Existing objects are not updated.
     * @param parent The parent container to set.
     * @return True if setting the parent was successful, false otherwise.
     */
    virtual bool SetProxyParent(const IContainer::Ptr& parent) = 0;
};

META_REGISTER_INTERFACE(IContainerPreTransaction, "1a1d3ba0-302c-442f-8a13-0701938a5f4c")
META_REGISTER_INTERFACE(IOnChildChanging, "385aec1c-0d4b-4ca1-85b5-6235e13ded3e")
META_REGISTER_INTERFACE(IOnChildChangingCallable, "7977c014-8181-465d-8d86-48d30c754e6c")

/**
 * @brief IOnChildChanging defines the interface for IContainerPreTransaction events.
 * @param info Information about the change that is about to happen
 * @param success The callee can request canceling the operation by setting the parameter to false.
 * @note Cancelling the operation may not always be possible.
 * @note Cancelling the operation does not work for deferred event callbacks.
 */
using IOnChildChanging = META_NS::SimpleEvent<IOnChildChangedInfo, void(const ChildChangedInfo& info, bool& success)>;

/**
 * @brief The IContainerPreTransaction interface can be implemenced by containers which support invoking events
 *        about transactions within the container that are about to happen.
 */
class IContainerPreTransaction : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IContainerPreTransaction)
public:
    /**
     * @brief Invoked when an object is about to be added to the container.
     */
    META_EVENT(IOnChildChanging, OnAdding)
    /**
     * @brief Invoked when an object is about to be removed from the container.
     */
    META_EVENT(IOnChildChanging, OnRemoving)
};

META_END_NAMESPACE()

META_TYPE(META_NS::IContainer::Ptr)
META_TYPE(META_NS::IContainer::WeakPtr)
META_TYPE(META_NS::TraversalType)

#endif
