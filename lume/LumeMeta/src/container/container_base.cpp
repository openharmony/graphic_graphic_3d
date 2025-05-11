
#include "container_base.h"

#include <algorithm>
#include <mutex>

#include <base/math/mathf.h>

#include <meta/api/iteration.h>
#include <meta/base/interface_utils.h>
#include <meta/interface/intf_containable.h>

META_BEGIN_NAMESPACE()

void ContainerBase::SetImplementingIContainer(IObject* me, IContainer* c)
{
    me_ = me;
    impl_ = c;
    CORE_ASSERT_MSG(impl_, "ContainerBase with null implementation");
}

void ContainerBase::LockShared() const
{
    mutex_.lock_shared();
}

void ContainerBase::UnlockShared() const
{
    mutex_.unlock_shared();
}

void ContainerBase::Lock() const
{
    mutex_.lock();
}

void ContainerBase::Unlock() const
{
    mutex_.unlock();
}

bool ContainerBase::SetProxyParent(const IContainer::Ptr& parent)
{
    parent_ = parent;
    return true;
}

IObject::Ptr ContainerBase::FindAnyImpl(const IContainer::FindOptions& options, bool isFlat) const
{
    IObject::Ptr res;
    Internal::ConstIterate(
        this,
        [&](const IObject::Ptr& obj) {
            if (MatchCriteria(options, obj)) {
                res = obj;
            }
            return !res;
        },
        IterateStrategy { isFlat ? TraversalType::NO_HIERARCHY : options.behavior, LockType::SHARED_LOCK });
    return res;
}

BASE_NS::vector<IObject::Ptr> ContainerBase::FindAllImpl(const IContainer::FindOptions& options, bool isFlat) const
{
    BASE_NS::vector<IObject::Ptr> res;
    Internal::ConstIterate(
        this,
        [&](const IObject::Ptr& obj) {
            if (options.name.empty() || obj->GetName() == options.name) {
                if (CheckInterfaces(obj, options.uids, options.strict)) {
                    res.push_back(obj);
                }
            }
            return true;
        },
        IterateStrategy { isFlat ? TraversalType::NO_HIERARCHY : options.behavior, LockType::SHARED_LOCK });
    return res;
}

BASE_NS::vector<IObject::Ptr> ContainerBase::GetAll() const
{
    std::shared_lock lock(mutex_);
    return children_;
}

IObject::Ptr ContainerBase::GetAt(SizeType index) const
{
    std::shared_lock lock(mutex_);
    if (index >= children_.size()) {
        return {};
    }
    return children_[index];
}

IContainer::SizeType ContainerBase::GetSize() const
{
    std::shared_lock lock(mutex_);
    return children_.size();
}

IObject::Ptr ContainerBase::FindByName(BASE_NS::string_view name) const
{
    std::shared_lock lock(mutex_);
    for (const auto& child : children_) {
        if (BASE_NS::string_view(child->GetName()) == name) {
            return child;
        }
    }
    return {};
}

bool ContainerBase::MatchCriteria(const IContainer::FindOptions& options, const IObject::Ptr& object) const
{
    return object && (options.name.empty() || object->GetName() == options.name) &&
           CheckInterfaces(object, options.uids, options.strict);
}

bool ContainerBase::Remove(SizeType index)
{
    IObject::Ptr child;
    {
        std::unique_lock lock(mutex_);
        if (children_.size() <= index) {
            return false;
        }
        auto it = children_.begin() + index;
        child = *it;
        children_.erase(it);
    }
    ChildChangedInfo info { ContainerChangeType::REMOVED, child, parent_, index };
    SetObjectParent(child, nullptr);
    Invoke<IOnChildChanged>(impl_->EventOnContainerChanged(MetadataQuery::EXISTING), info);
    return true;
}

bool ContainerBase::Remove(const IObject::Ptr& child)
{
    if (!child) {
        return false;
    }
    SizeType index = 0;
    {
        std::unique_lock lock(mutex_);
        auto it = children_.begin();
        for (; it != children_.end() && *it != child; ++it, ++index) {
        }
        if (it == children_.end()) {
            return false;
        }
        children_.erase(it);
    }
    ChildChangedInfo info { ContainerChangeType::REMOVED, child, parent_, index };
    SetObjectParent(child, nullptr);
    Invoke<IOnChildChanged>(impl_->EventOnContainerChanged(MetadataQuery::EXISTING), info);
    return true;
}

ChildChangedInfo ContainerBase::MoveInternal(SizeType fromIndex, SizeType toIndex)
{
    if (children_.empty()) {
        return {};
    }
    const auto size = children_.size();
    fromIndex = BASE_NS::Math::min(fromIndex, size - 1);
    toIndex = BASE_NS::Math::min(toIndex, size - 1);
    const IObject::Ptr child = children_[fromIndex];
    if (fromIndex > toIndex) {
        const auto first = children_.rbegin() + (size - fromIndex - 1);
        const auto last = children_.rbegin() + (size - toIndex);
        std::rotate(first, first + 1, last);
    } else if (fromIndex < toIndex) {
        const auto first = children_.begin() + fromIndex;
        const auto last = children_.begin() + toIndex + 1;
        std::rotate(first, first + 1, last);
    }
    return { ContainerChangeType::MOVED, child, parent_, fromIndex, toIndex };
}

bool ContainerBase::Move(SizeType fromIndex, SizeType toIndex)
{
    ChildChangedInfo info;
    {
        std::unique_lock lock(mutex_);
        info = MoveInternal(fromIndex, toIndex);
    }
    if (info.object) {
        if (info.from != info.to) {
            Invoke<IOnChildChanged>(impl_->EventOnContainerChanged(MetadataQuery::EXISTING), info);
        }
        return true;
    }
    return false;
}

bool ContainerBase::Move(const IObject::Ptr& child, SizeType toIndex)
{
    ChildChangedInfo info;
    {
        std::unique_lock lock(mutex_);
        SizeType fromIndex = 0;
        for (const auto& c : children_) {
            if (c == child) {
                info = MoveInternal(fromIndex, toIndex);
                break;
            }
            fromIndex++;
        }
    }
    if (info.object) {
        if (info.from != info.to) {
            Invoke<IOnChildChanged>(impl_->EventOnContainerChanged(MetadataQuery::EXISTING), info);
        }
        return true;
    }
    return false;
}

void ContainerBase::RemoveAll()
{
    BASE_NS::vector<IObject::Ptr> children;
    {
        std::unique_lock lock(mutex_);
        children_.swap(children);
    }
    SizeType index = 0;
    for (const auto& child : children) {
        ChildChangedInfo info { ContainerChangeType::REMOVED, child, parent_, index++ };
        SetObjectParent(child, nullptr);
        Invoke<IOnChildChanged>(impl_->EventOnContainerChanged(MetadataQuery::EXISTING), info);
    }
}

void ContainerBase::InternalRemoveAll()
{
    BASE_NS::vector<IObject::Ptr> children;
    {
        std::unique_lock lock(mutex_);
        children_.swap(children);
    }
    for (const auto& child : children) {
        if (auto c = interface_cast<IMutableContainable>(child)) {
            c->SetParent(nullptr);
        }
    }
}

bool ContainerBase::SetRequiredInterfaces(const BASE_NS::vector<TypeId>& interfaces)
{
    std::unique_lock lock(mutex_);
    required_ = interfaces;

    BASE_NS::vector<IObject::Ptr> compatible;
    compatible.reserve(children_.size());
    for (const auto& child : children_) {
        if (IsCompatible(child)) {
            compatible.push_back(child);
        }
    }
    children_.swap(compatible);
    return true;
}

BASE_NS::vector<TypeId> ContainerBase::GetRequiredInterfaces() const
{
    std::shared_lock lock(mutex_);
    return required_;
}

bool ContainerBase::IsCompatible(const IObject::Ptr& object) const
{
    return ObjectImplementsAll(object, required_);
}

bool ContainerBase::IsAncestorOf(const IObject::ConstPtr& object) const
{
    if (!object || !me_) {
        return false;
    }
    if (me_ == object.get()) {
        return true;
    }
    const auto containable = interface_pointer_cast<IContainable>(object);
    if (!containable) {
        return false;
    }
    auto parent = containable->GetParent();
    while (parent) {
        if (parent.get() == me_) {
            return true;
        }
        if (auto parentContainable = interface_cast<IContainable>(parent)) {
            parent = parentContainable->GetParent();
        } else {
            break;
        }
    }
    return false;
}

BASE_NS::shared_ptr<IEvent> ContainerBase::EventOnContainerChanged(MetadataQuery q) const
{
    return impl_->EventOnContainerChanged(q);
}

template<typename Cont, typename Func>
static IterationResult IterateImpl(Cont& cont, const Func& func)
{
    for (auto&& child : cont) {
        auto res = func->Invoke(child);
        if (!res.Continue()) {
            return res;
        }
    }
    return IterationResult::CONTINUE;
}

IterationResult ContainerBase::Iterate(const IterationParameters& params)
{
    auto f = params.function.GetInterface<IIterableCallable<IObject::Ptr>>();
    if (!f) {
        CORE_LOG_W("Incompatible function with Iterate");
        return IterationResult::FAILED;
    }
    return IterateImpl(children_, f);
}

IterationResult ContainerBase::Iterate(const IterationParameters& params) const
{
    auto f = params.function.GetInterface<IIterableConstCallable<IObject::Ptr>>();
    if (!f) {
        CORE_LOG_W("Incompatible function with Iterate");
        return IterationResult::FAILED;
    }
    return IterateImpl(children_, f);
}

META_END_NAMESPACE()
