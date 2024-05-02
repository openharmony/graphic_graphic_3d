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
#include "startable_object_controller.h"

#include <meta/api/future.h>
#include <meta/api/iteration.h>
#include <meta/api/make_callback.h>
#include <meta/api/task.h>
#include <meta/api/util.h>
#include <meta/interface/intf_content.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/property/property_events.h>

META_BEGIN_NAMESPACE()

bool StartableObjectController::Build(const IMetadata::Ptr& data)
{
    auto& reg = GetObjectRegistry();
    observer_ = reg.Create<IObjectHierarchyObserver>(ClassId::ObjectHierarchyObserver);
    CORE_ASSERT(observer_);
    clock_ = reg.Create<IClock>(ClassId::SystemClock);
    CORE_ASSERT(clock_);

    observer_->OnHierarchyChanged()->AddHandler(
        MakeCallback<IOnHierarchyChanged>(this, &StartableObjectController::HierarchyChanged));

    META_ACCESS_PROPERTY(StartBehavior)->OnChanged()->AddHandler(MakeCallback<IOnChanged>([this]() {
        if (META_ACCESS_PROPERTY_VALUE(StartBehavior) == StartBehavior::AUTOMATIC) {
            // If StartBehavior changes to AUTOMATIC, start all AUTOMATIC startables
            StartAll(ControlBehavior::CONTROL_AUTOMATIC);
        }
    }));

    defaultTickerQueue_ = META_NS::GetObjectRegistry().Create<ITaskQueue>(ClassId::ThreadedTaskQueue);
    CORE_ASSERT(defaultTickerQueue_);
    tickerTask_ = META_NS::MakeCallback<ITaskQueueTask>([this]() {
        TickAll(clock_->GetTime());
        return true; // Recurring
    });
    CORE_ASSERT(tickerTask_);

    META_ACCESS_PROPERTY(TickInterval)
        ->OnChanged()
        ->AddHandler(MakeCallback<IOnChanged>(this, &StartableObjectController::UpdateTicker));
    META_ACCESS_PROPERTY(TickOrder)->OnChanged()->AddHandler(
        MakeCallback<IOnChanged>(this, &StartableObjectController::InvalidateTickables));
    UpdateTicker();
    return true;
}

void StartableObjectController::Destroy()
{
    if (tickerQueue_ && tickerToken_) {
        tickerQueue_->CancelTask(tickerToken_);
    }
    InvalidateTickables();
    SetTarget({}, {});
    observer_.reset();
}

bool StartableObjectController::SetStartableQueueId(
    const BASE_NS::Uid& startStartableQueueId, const BASE_NS::Uid& stopStartableQueueId)
{
    startQueueId_ = startStartableQueueId;
    stopQueueId_ = stopStartableQueueId;
    return true;
}

void StartableObjectController::SetTarget(const IObject::Ptr& hierarchyRoot, HierarchyChangeModeValue mode)
{
    if (!observer_) {
        return;
    }
    InvalidateTickables();
    target_ = hierarchyRoot;
    bool automatic = META_ACCESS_PROPERTY_VALUE(StartBehavior) == StartBehavior::AUTOMATIC;
    if (automatic && !hierarchyRoot) {
        StopAll(ControlBehavior::CONTROL_AUTOMATIC);
    }
    observer_->SetTarget(hierarchyRoot, mode);
    if (automatic && hierarchyRoot) {
        StartAll(ControlBehavior::CONTROL_AUTOMATIC);
    }
}

IObject::Ptr StartableObjectController::GetTarget() const
{
    return observer_->GetTarget();
}

BASE_NS::vector<IObject::Ptr> StartableObjectController::GetAllObserved() const
{
    return observer_->GetAllObserved();
}

bool StartableObjectController::StartAll(ControlBehavior behavior)
{
    if (const auto root = target_.lock()) {
        return AddOperation({ StartableOperation::START, target_ }, startQueueId_);
    }
    return false;
}

bool StartableObjectController::StopAll(ControlBehavior behavior)
{
    if (auto root = target_.lock()) {
        return AddOperation({ StartableOperation::STOP, target_ }, stopQueueId_);
    }
    return false;
}

template<class T, class Callback>
void IterateChildren(const BASE_NS::vector<T>& children, bool reverse, Callback&& callback)
{
    if (reverse) {
        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            callback(*it);
        }
    } else {
        for (auto&& child : children) {
            callback(child);
        }
    }
}

template<class Callback>
void IterateHierarchy(const IObject::Ptr& root, bool reverse, Callback&& callback)
{
    if (const auto container = interface_cast<IContainer>(root)) {
        IterateChildren(container->GetAll(), reverse, callback);
    }
    if (const auto content = interface_cast<IContent>(root)) {
        if (auto object = GetValue(content->Content())) {
            callback(object);
        }
    }
}

template<class ObjectType, class Callback>
void IterateAttachments(const IObject::Ptr& object, bool reverse, Callback&& callback)
{
    if (const auto attach = interface_cast<IAttach>(object)) {
        if (auto container = attach->GetAttachmentContainer(false)) {
            IterateChildren(container->GetAll<ObjectType>(), reverse, BASE_NS::forward<Callback>(callback));
        }
    }
}

template<class Callback>
void IterateStartables(const IObject::Ptr& object, bool reverse, Callback&& callback)
{
    IterateAttachments<IStartable, Callback>(object, reverse, BASE_NS::forward<Callback>(callback));
}

template<class Callback>
void IterateTickables(const IObject::Ptr& object, TraversalType order, Callback&& callback)
{
    if (!object) {
        return;
    }
    bool rootFirst = order != TraversalType::DEPTH_FIRST_POST_ORDER;
    if (rootFirst) {
        IterateAttachments<ITickable, Callback>(object, false, BASE_NS::forward<Callback>(callback));
    }
    IterateShared(
        object,
        [&callback](const IObject::Ptr& object) {
            IterateAttachments<ITickable, Callback>(object, false, callback);
            return true;
        },
        order);
    if (!rootFirst) {
        IterateAttachments<ITickable, Callback>(object, false, BASE_NS::forward<Callback>(callback));
    }
}

void StartableObjectController::HierarchyChanged(const HierarchyChangedInfo& info)
{
    if (info.change == HierarchyChangeType::ADDED || info.change == HierarchyChangeType::REMOVING ||
        info.change == HierarchyChangeType::MOVED) {
        // Any hierarchy change (add/remove/move) invalidates the tick order
        InvalidateTickables();
        if (info.change == HierarchyChangeType::ADDED) {
            AddOperation({ StartableOperation::START, info.object }, startQueueId_);
        } else if (info.change == HierarchyChangeType::REMOVING) {
            AddOperation({ StartableOperation::STOP, info.object }, stopQueueId_);
        }
    }
}

BASE_NS::vector<IStartable::Ptr> StartableObjectController::GetAllStartables() const
{
    BASE_NS::vector<IStartable::Ptr> startables;
    auto add = [&startables](const IStartable::Ptr& startable) { startables.push_back(startable); };
    if (const auto root = target_.lock()) {
        IterateStartables(root, false, add);
        IterateShared(
            root,
            [&add](const IObject::Ptr& object) {
                IterateStartables(object, false, add);
                return true;
            },
            TraversalType::DEPTH_FIRST_POST_ORDER);
    }
    return startables;
}

void StartableObjectController::StartHierarchy(const IObject::Ptr& root, ControlBehavior behavior)
{
    const auto traversal = META_ACCESS_PROPERTY_VALUE(TraversalType);
    if (traversal != TraversalType::DEPTH_FIRST_POST_ORDER && traversal != TraversalType::FULL_HIERARCHY) {
        CORE_LOG_E("Only DEPTH_FIRST_POST_ORDER is supported");
    }

    if (!root) {
        return;
    }

    IterateHierarchy(root, false, [this, behavior](const IObject::Ptr& object) { StartHierarchy(object, behavior); });

    // Don't traverse hierarchy for attachments
    IterateStartables(
        root, false, [this, behavior](const IStartable::Ptr& startable) { StartStartable(startable.get(), behavior); });

    StartStartable(interface_cast<IStartable>(root), behavior);
}

void StartableObjectController::StartStartable(IStartable* const startable, ControlBehavior behavior)
{
    if (startable) {
        const auto state = GetValue(startable->StartableState());
        if (state == StartableState::ATTACHED) {
            const auto mode = GetValue(startable->StartableMode());
            if (behavior == ControlBehavior::CONTROL_ALL || mode == StartBehavior::AUTOMATIC) {
                startable->Start();
            }
        }
    }
}

void StartableObjectController::StopHierarchy(const IObject::Ptr& root, ControlBehavior behavior)
{
    const auto traversal = META_ACCESS_PROPERTY_VALUE(TraversalType);
    if (traversal != TraversalType::DEPTH_FIRST_POST_ORDER && traversal != TraversalType::FULL_HIERARCHY) {
        CORE_LOG_E("Only DEPTH_FIRST_POST_ORDER is supported");
    }
    if (!root) {
        return;
    }

    StopStartable(interface_cast<IStartable>(root), behavior);

    IterateStartables(
        root, true, [this, behavior](const IStartable::Ptr& startable) { StopStartable(startable.get(), behavior); });

    IterateHierarchy(root, true, [this, behavior](const IObject::Ptr& object) { StopHierarchy(object, behavior); });
}

void StartableObjectController::StopStartable(IStartable* const startable, ControlBehavior behavior)
{
    if (startable) {
        const auto state = GetValue(startable->StartableState());
        if (state == StartableState::STARTED) {
            const auto mode = GetValue(startable->StartableMode());
            if (behavior == ControlBehavior::CONTROL_ALL || mode == StartBehavior::AUTOMATIC) {
                startable->Stop();
            }
        }
    }
}

bool StartableObjectController::HasTasks(const BASE_NS::Uid& queueId) const
{
    std::shared_lock lock(mutex_);
    if (auto it = operations_.find(queueId); it != operations_.end()) {
        return !it->second.empty();
    }
    return false;
}

void StartableObjectController::RunTasks(const BASE_NS::Uid& queueId)
{
    BASE_NS::vector<StartableOperation> operations;
    {
        std::unique_lock lock(mutex_);
        // Take tasks for the given queue id
        if (auto it = operations_.find(queueId); it != operations_.end()) {
            operations.swap(it->second);
        }
    }
    for (auto&& op : operations) {
        // This may potentially end up calling Start/StopHierarchy several times
        // for the same subtrees, but we will accept that. Start/Stop will only
        // be called once since the functions check for current state.
        if (auto root = op.root_.lock()) {
            switch (op.operation_) {
                case StartableOperation::START:
                    ++executingStart_;
                    StartHierarchy(root, ControlBehavior::CONTROL_AUTOMATIC);
                    --executingStart_;
                    break;
                case StartableOperation::STOP:
                    StopHierarchy(root, ControlBehavior::CONTROL_AUTOMATIC);
                    break;
                default:
                    break;
            }
        }
    }
}

bool StartableObjectController::ProcessOps(const BASE_NS::Uid& queueId)
{
    if (!HasTasks(queueId)) {
        // No tasks for the given queue, bail out
        return true;
    }

    auto task = [queueId, internal = IStartableObjectControllerInternal::WeakPtr {
        GetSelf<IStartableObjectControllerInternal>() }]() {
            if (auto me = internal.lock()) {
                me->RunTasks(queueId);
            }
    };

    if (queueId != BASE_NS::Uid {} && !executingStart_) {
        if (auto queue = GetTaskQueueRegistry().GetTaskQueue(queueId)) {
            queue->AddWaitableTask(CreateWaitableTask(BASE_NS::move(task)));
            return true;
        }
        CORE_LOG_W("Cannot get task queue '%s'. Running the task synchronously.", BASE_NS::to_string(queueId).c_str());
    }
    // Just run the task immediately if we don't have a queue to defer it to
    task();
    return true;
}

bool StartableObjectController::AddOperation(StartableOperation&& operation, const BASE_NS::Uid& queueId)
{
    auto object = operation.root_.lock();
    if (!object) {
        return false;
    }
    // Note that queueId may be {}, but it is still a valid key for our queue map
    {
        std::unique_lock lock(mutex_);
        auto& ops = operations_[queueId];
        for (auto it = ops.begin(); it != ops.end(); ++it) {
            // If we already have an operation in queue for a given object, cancel the existing operation
            // and just add the new one
            if ((*it).root_.lock() == object) {
                ops.erase(it);
                break;
            }
        }
        ops.emplace_back(BASE_NS::move(operation));
    }
    return ProcessOps(queueId);
}

void StartableObjectController::InvalidateTickables()
{
    std::unique_lock lock(mutex_);
    tickables_.clear();
    tickablesValid_ = false;
}

BASE_NS::vector<ITickable::Ptr> StartableObjectController::GetTickables() const
{
    BASE_NS::vector<ITickable::WeakPtr> weaks;
    {
        std::unique_lock lock(tickMutex_);
        if (!tickablesValid_) {
            auto add = [this](const ITickable::Ptr& tickable) { tickables_.push_back(tickable); };
            IterateTickables(target_.lock(), META_ACCESS_PROPERTY_VALUE(TickOrder), add);
            tickablesValid_ = true;
        }
        weaks = tickables_;
    }
    BASE_NS::vector<ITickable::Ptr> tickables;
    tickables.reserve(weaks.size());
    for (auto&& t : weaks) {
        if (auto tick = t.lock()) {
            tickables.emplace_back(BASE_NS::move(tick));
        }
    }
    return tickables;
}

void StartableObjectController::UpdateTicker()
{
    auto queue = tickQueueId_ != BASE_NS::Uid {} ? META_NS::GetTaskQueueRegistry().GetTaskQueue(tickQueueId_)
                                                 : defaultTickerQueue_;
    if (tickerQueue_ && tickerToken_) {
        tickerQueue_->CancelTask(tickerToken_);
        tickerToken_ = {};
    }
    tickerQueue_ = queue;
    if (const auto interval = META_ACCESS_PROPERTY_VALUE(TickInterval); interval != TimeSpan::Infinite()) {
        if (tickerQueue_) {
            tickerToken_ = tickerQueue_->AddTask(tickerTask_, interval);
        } else {
            CORE_LOG_E("Invalid queue given for running ITickables: %s", BASE_NS::to_string(tickQueueId_).c_str());
        }
    }
}

bool StartableObjectController::SetTickableQueueuId(const BASE_NS::Uid& queueId)
{
    if (queueId != tickQueueId_) {
        tickQueueId_ = queueId;
        UpdateTicker();
    }
    return true;
}

void StartableObjectController::TickAll(const TimeSpan& time)
{
    const auto tickables = GetTickables();
    if (!tickables.empty()) {
        const auto sinceLast = lastTick_ != TimeSpan::Infinite() ? time - lastTick_ : TimeSpan::Zero();
        for (auto&& tickable : tickables) {
            bool shouldTick = true;
            if (const auto st = interface_cast<IStartable>(tickable)) {
                shouldTick = GetValue(st->StartableState()) == StartableState::STARTED;
            }
            if (shouldTick) {
                tickable->Tick(time, sinceLast);
            }
        }
    }
    lastTick_ = time;
}

META_END_NAMESPACE()
