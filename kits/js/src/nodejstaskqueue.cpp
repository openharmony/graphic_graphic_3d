/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "nodejstaskqueue.h"

#include <chrono>
#include <deque>
#include <meta/ext/object.h>
#include <meta/ext/task_queue.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <mutex>
#include <thread>

using namespace META_NS;
using Token = ITaskQueue::Token;

class NodeJSTaskQueue : public IntroduceInterfaces<MetaObject, ITaskQueue, INodeJSTaskQueue> {
    META_OBJECT(NodeJSTaskQueue, ::ClassId::NodeJSTaskQueue, IntroduceInterfaces)
public:
    META_NO_COPY_MOVE(NodeJSTaskQueue)

    NodeJSTaskQueue();
    ~NodeJSTaskQueue() override;

private:
    // ITaskQueue
    Token AddTask(ITaskQueueTask::Ptr p) override;
    Token AddTask(ITaskQueueTask::Ptr p, const TimeSpan& delay) override;
    IFuture::Ptr AddWaitableTask(ITaskQueueWaitableTask::Ptr p) override;
    void CancelTask(Token token) override;

    // INodeJSTaskQueue
    napi_env GetNapiEnv() const override;
    bool Acquire() override;
    bool Release() override;
    bool IsReleased() override;

    // ILifecycle
    bool Build(const IMetadata::Ptr& data) override;

private:
    static napi_value Run(napi_env env, napi_callback_info info);
    static void Cleanup(napi_env env, void* finalize_data, void* context);
    static void Invoke(napi_env env, napi_value js_callback, void* context, void* data);

    void SetTimeout(napi_env env, uint32_t trigger);
    void CancelTimeout(napi_env env);

    void Schedule(napi_env env, std::unique_lock<std::recursive_mutex>& lock);
    bool RescheduleTimer();

    bool IsInQueue();

    Token MakeToken(const ITaskQueueTask::Ptr p);
    void AddTaskImpl(Token ret, ITaskQueueTask::Ptr p, const TimeSpan& delay, const TimeSpan& excTime);
    Token AddTask(ITaskQueueTask::Ptr p, const TimeSpan& delay, const TimeSpan& excTime);

    std::chrono::high_resolution_clock::duration Now() const;
    TimeSpan TimeFloor() const;
    TimeSpan TimeCeil() const;

    struct Task {
        Task() = default;
        Task(Token t, TimeSpan d, TimeSpan e, const ITaskQueueTask::Ptr& p)
            : token(t), delay(d), executeTime(e), operation(p)
        {}

        Token token { 0 };
        TimeSpan delay;
        TimeSpan executeTime;
        ITaskQueueTask::Ptr operation { nullptr };
    };

    std::chrono::high_resolution_clock::duration epoch_ { 0 };
    napi_env env_ { nullptr };
    napi_threadsafe_function tsf_ { nullptr };
    NapiApi::StrongRef curTimeout_;
    NapiApi::StrongRef RunFunc_;

    std::recursive_mutex mutex_;
    std::thread::id execThread_;
    // currently running task..
    Token execToken_ { nullptr };
    std::deque<Task> tasks_;
    bool scheduled_ = false;
    uint32_t refcnt_ { 0 };
};

NodeJSTaskQueue::NodeJSTaskQueue() {}

NodeJSTaskQueue::~NodeJSTaskQueue()
{
    assert(tasks_.empty());
    assert(refcnt_ == 0);
    assert(curTimeout_.IsEmpty());
    assert(RunFunc_.IsEmpty());
    assert(tsf_ == nullptr);
    assert(!scheduled_);
}

bool NodeJSTaskQueue::Acquire()
{
    std::unique_lock lock { mutex_ };
    assert(execThread_ == std::this_thread::get_id());
    if (execThread_ != std::this_thread::get_id()) {
        // called from incorrect thread. do nothing.
        return false;
    }
    if (refcnt_ == 0) {
        if (tsf_ == nullptr) {
            napi_value func;
            napi_create_function(env_, "NodeJsTaskQueueRun", 0, Run, this, &func);
            RunFunc_ = { env_, func };

            napi_value name;
            napi_create_string_latin1(env_, "NodeJSTaskQueue", 1, &name);
            napi_create_threadsafe_function(env_, nullptr, nullptr, name, 0, 1, nullptr, Cleanup, this, Invoke, &tsf_);
        }
    }
    refcnt_++;
    return true;
}

bool NodeJSTaskQueue::Release()
{
    std::unique_lock lock { mutex_ };
    assert(execThread_ == std::this_thread::get_id());
    if (execThread_ != std::this_thread::get_id()) {
        // called from incorrect thread. do nothing.
        return false;
    }
    if (refcnt_ == 0) {
        // already released.
        return false;
    }
    if (refcnt_ == 1) {
        refcnt_ = 0;
        // fully released, we may be allowed to release the resources
        if (tasks_.empty()) {
            // queue empty, we can finalize.
            auto tsf = tsf_;
            tsf_ = nullptr;
            RunFunc_.Reset();
            curTimeout_.Reset();
            lock.unlock();
            napi_release_threadsafe_function(tsf, napi_threadsafe_function_release_mode::napi_tsfn_abort);
        }
        return true;
    }
    refcnt_--;
    return true;
}

bool NodeJSTaskQueue::IsReleased()
{
    std::unique_lock lock { mutex_ };
    assert(execThread_ == std::this_thread::get_id());
    if (execThread_ != std::this_thread::get_id()) {
        // called from incorrect thread.
        return false;
    }
    if (refcnt_ > 0) {
        return false;
    }
    if (!tasks_.empty()) {
        return false;
    }
    if (tsf_) {
        return false;
    }
    if (scheduled_) {
        // still busy..
        return false;
    }
    return true;
}

bool NodeJSTaskQueue::Build(const IMetadata::Ptr& data)
{
    if (!Super::Build(data)) {
        return false;
    }
    if (auto prp = data->GetProperty<uintptr_t>("env")) {
        env_ = (napi_env)prp->GetValue();
    } else {
        return false;
    }
    execThread_ = std::this_thread::get_id();
    epoch_ = std::chrono::high_resolution_clock::now().time_since_epoch();

    // make the NodeJSTaskQueue current for this thread.
    // setting it here, makes sure that GetCurrentTaskQueue could be used to identify the thread.
    // unless someone runs other taskqueues in js thread.. hmm
    GetTaskQueueRegistry().SetCurrentTaskQueue(GetSelf<ITaskQueue>());

    return true;
}

napi_env NodeJSTaskQueue::GetNapiEnv() const
{
    return env_;
}
/*static*/ napi_value NodeJSTaskQueue::Run(napi_env env, napi_callback_info info)
{
    NapiApi::FunctionContext fc(env, info);
    auto me = static_cast<NodeJSTaskQueue*>(fc.GetData());
    std::unique_lock lock { me->mutex_ };
    me->curTimeout_.Reset();
    me->Schedule(env, lock);
    return {};
}
void NodeJSTaskQueue::SetTimeout(napi_env env, uint32_t trigger)
{
    NapiApi::Env e(env);
    napi_value global;
    napi_get_global(env, &global);
    NapiApi::Object g(e, global);
    NapiApi::Function func = g.Get<NapiApi::Function>("setTimeout");

    napi_value args[2];
    args[0] = RunFunc_.GetValue();
    args[1] = e.GetNumber(trigger);
    napi_value res { nullptr };
    res = func.Invoke(g, 2, args);
    curTimeout_ = { e, res };
}
void NodeJSTaskQueue::CancelTimeout(napi_env env)
{
    if (curTimeout_.IsEmpty()) {
        return;
    }
    // cancel previous timeout
    NapiApi::Env e(env);
    napi_value global;
    napi_get_global(env, &global);
    NapiApi::Object g(e, global);
    NapiApi::Function func = g.Get<NapiApi::Function>("clearTimeout");

    napi_value args[1];
    args[0] = curTimeout_.GetValue();
    func.Invoke(g, 1, args);
    curTimeout_.Reset();
}
void NodeJSTaskQueue::Schedule(napi_env env, std::unique_lock<std::recursive_mutex>& lock)
{
    // NOTE: lock MUST be in locked state when entering here.
    assert(lock.owns_lock());

    // running in javascript side.
    BASE_NS::vector<Task> rearm;
    auto curTime = TimeFloor(); // round down the time..
    // 1. see how long until the first scheduled task. (looping through the queue ,executing tasks if needed)
    for (;;) {
        if (tasks_.empty()) {
            // no tasks to execute.
            break;
        }
        auto front = tasks_.back();
        // 2. if task should have executed already, invoke it.
        if (curTime >= front.executeTime) {
            // execute
            tasks_.pop_back();
            execToken_ = front.token;
            lock.unlock();
            auto q = GetTaskQueueRegistry().SetCurrentTaskQueue(GetSelf<ITaskQueue>());
            auto ret = front.operation->Invoke();
            GetTaskQueueRegistry().SetCurrentTaskQueue(q);
            if (!ret) {
                // if the task is to be terminated, do it here.
                front = {};
            }
            lock.lock();
            if (ret && execToken_) {
                rearm.emplace_back(front.token, front.delay, front.executeTime, BASE_NS::move(front.operation));
            }
            execToken_ = { 0 };
            continue;
        }
        break;
    }
    // cancel timeout if there is one
    CancelTimeout(env);

    if (!rearm.empty()) {
        // just add them..
        for (auto t : rearm) {
            // calculate the next executeTime in phase.. (ie. how many events missed)
            const auto dt = t.delay;
            const auto ct = curTime;
            if (dt > TimeSpan::Zero()) {
                auto et = t.executeTime;
                // calculate the next executeTime in phase..
                auto ticks = ((ct - et) + dt) / dt;
                // and based on the "ticks" we can now count the next execution time.
                et += (ticks * dt);
                AddTaskImpl(t.token, t.operation, dt, et);
            } else {
                // re-scheduling zero delay ones..
                // so zero delay, and now.
                AddTaskImpl(t.token, t.operation, TimeSpan::Zero(), ct);
            }
        }
    }

    if (!tasks_.empty()) {
        // 3. schedule a timer for remaining tasks. (using the front most time)
        const auto& front = tasks_.back();
        int64_t delayMS = (front.executeTime - curTime).ToMilliseconds();
        // clamp to at most 10 seconds of wait and at minimum 0 milliseconds.
        uint32_t trigger = BASE_NS::Math::max(int64_t(0), BASE_NS::Math::min(int64_t(10000), delayMS));
        SetTimeout(env, trigger);
    }

    if (tasks_.empty()) {
        // queue empty, we may be allowed to release resourcec.
        if (refcnt_ == 0) {
            if (tsf_) {
                // release the TSF, we have given permission to terminate.
                auto tsf = tsf_;
                tsf_ = nullptr;
                napi_release_threadsafe_function(tsf, napi_threadsafe_function_release_mode::napi_tsfn_abort);
            }
            RunFunc_.Reset();
        }
    }
}

bool NodeJSTaskQueue::RescheduleTimer()
{
    std::unique_lock lock { mutex_ };
    if (IsInQueue()) {
        // call directly as we are in JS thread.
        Schedule(env_, lock);
        return true;
    }
    if (!tsf_) {
        // tsf does not exist anymore, so stop scheduling.
        return false;
    }
    if (!scheduled_) {
        // TSF not queued yet.
        // use tsf to schedule
        auto res = napi_acquire_threadsafe_function(tsf_);
        if (res != napi_ok) {
            // could not acquire the function anymore.
            // (most likely cleanup already in progress)
            // fail.
            return false;
        }
        void* data = nullptr;
        scheduled_ = true;
        res = napi_call_threadsafe_function(tsf_, data, napi_threadsafe_function_call_mode::napi_tsfn_blocking);
        if (res == napi_closing) {
            // clean up in progress.
            return false;
        }
        res = napi_release_threadsafe_function(tsf_, napi_threadsafe_function_release_mode::napi_tsfn_release);
        if (res != napi_ok) {
            return false;
        }
    }
    return true;
}

/*static*/ void NodeJSTaskQueue::Cleanup(napi_env env, void* finalize_data, void* context)
{
    auto* me = static_cast<NodeJSTaskQueue*>(context);
    // we actually do not need to do anything here now.
}
/*static*/ void NodeJSTaskQueue::Invoke(napi_env env, napi_value js_callback, void* context, void* data)
{
    auto* me = static_cast<NodeJSTaskQueue*>(context);
    if (me) {
        std::unique_lock lock { me->mutex_ };
        me->scheduled_ = false;
        me->Schedule(env, lock);
    }
}
bool NodeJSTaskQueue::IsInQueue()
{
    auto tmp = GetTaskQueueRegistry().GetCurrentTaskQueue();
    if (!tmp) {
        return false;
    }
    // this is the correct way to compare random objects.
    auto i1 = tmp->GetInterface(CORE_NS::IInterface::UID);
    auto i2 = GetInterface(CORE_NS::IInterface::UID);
    return (i1 == i2);
}
void NodeJSTaskQueue::CancelTask(Token token)
{
    if (!token) {
        // invalid parameter.
        return;
    }
    bool sameThread = false;
    auto curThread = std::this_thread::get_id();
    if (IsInQueue()) {
        // currently executing tasks in this thread.
        sameThread = true;
    }
    if (curThread != execThread_) {
        sameThread = true;
    }
    std::unique_lock lock { mutex_ };
    Token executingToken = execToken_;
    if (token == execToken_) {
        // Currently executing task is requested to cancel.
        // Tasks are temporarily removed from the queue while execution, so the currently running task is not in
        // the queue anymore. Setting execToken_ to null will cause the task to not be re-added.
        execToken_ = nullptr;
    }

    // If we are currently executing the task in different thread, wait for it to complete.
    // ie. called OUTSIDE the queue.
    if (!sameThread) {
        while (token == executingToken) {
            lock.unlock();
            // sleep a bit.
            std::this_thread::yield();
            lock.lock();
            executingToken = execToken_;
        }
    }

    // Remove all tasks from the queue, with the same token. (if any)
    // One can push the same task to the queue multiple times currently.
    // (ie. you "can" schedule the same task with different "delays")
    // So we remove all scheduled tasks with same token.
    // Also redo/rearm might have add the task back while we were waiting/yielding.
    for (auto it = tasks_.begin(); it != tasks_.end();) {
        if (it->token == token) {
            it = tasks_.erase(it);
        } else {
            it++;
        }
    }
    // trigger js side to schedule the tasks!
    RescheduleTimer();
}

void NodeJSTaskQueue::AddTaskImpl(Token ret, ITaskQueueTask::Ptr p, const TimeSpan& delay, const TimeSpan& excTime)
{
    if (auto i = interface_cast<ITaskScheduleInfo>(p)) {
        i->SetQueueAndToken(GetSelf<ITaskQueue>(), ret);
    }

    // insertion sort the tasks
    //
    // early out trivial cases (empty list, insert first, insert last..)
    if (tasks_.empty()) {
        tasks_.emplace_back(ret, delay, excTime, BASE_NS::move(p));
    } else if (excTime <= tasks_.back().executeTime) {
        // new task should execute before the current first one.
        tasks_.emplace_back(ret, delay, excTime, BASE_NS::move(p));
    } else if (excTime > tasks_.front().executeTime) {
        // new task should execute after the current last one.
        tasks_.emplace_front(ret, delay, excTime, BASE_NS::move(p));
    } else {
        // finally resort to linear search..
        // (could use a smarter search but expect that there are not too many tasks in queue)
        for (auto it = tasks_.begin(); it != tasks_.end(); ++it) {
            if (it->executeTime <= excTime) {
                // task in list should execute after us, so insert there.
                tasks_.insert(it, { ret, delay, excTime, BASE_NS::move(p) });
                break;
            }
        }
    }
}
Token NodeJSTaskQueue::MakeToken(const ITaskQueueTask::Ptr p)
{
    // use the old cheap "task raw pointer as token" method.
    return p.get();
}
Token NodeJSTaskQueue::AddTask(ITaskQueueTask::Ptr p, const TimeSpan& delay, const TimeSpan& excTime)
{
    if (!p) {
        return nullptr;
    }
    std::unique_lock lock { mutex_ };
    // unique
    Token result = MakeToken(p);
    AddTaskImpl(result, BASE_NS::move(p), delay, excTime);

    // trigger js side to schedule the tasks!
    if (!RescheduleTimer()) {
        // can not schedule tasks anymore.
        // remove the task we TRIED to add..
        for (auto it = tasks_.begin(); it != tasks_.end(); it++) {
            if ((it->token = result) && (it->executeTime == excTime) && (it->delay == delay)) {
                tasks_.erase(it);
                break;
            }
        }
        result = { nullptr };
    }
    return result;
}

Token NodeJSTaskQueue::AddTask(ITaskQueueTask::Ptr p)
{
    return AddTask(BASE_NS::move(p), TimeSpan::Milliseconds(0));
}

Token NodeJSTaskQueue::AddTask(ITaskQueueTask::Ptr p, const TimeSpan& delay)
{
    return AddTask(BASE_NS::move(p), delay, TimeCeil() + delay);
}

IFuture::Ptr NodeJSTaskQueue::AddWaitableTask(ITaskQueueWaitableTask::Ptr p)
{
    if (IsInQueue()) {
        // warning! this may cause a deadlock.
        LOG_W("Adding a waitable task to the same thread! Wait may never complete!");
    }

    auto promise = GetObjectRegistry().Create<IPromise>(META_NS::ClassId::Promise);
    BASE_NS::shared_ptr<PromisedQueueTask> task(new PromisedQueueTask(p, move(promise)));
    auto f = task->GetFuture();
    if (!AddTask(BASE_NS::move(task))) {
        // could not schedule the task.
        // so the promise is abandoned.
        promise->SetAbandoned();
    }
    return f;
}

std::chrono::high_resolution_clock::duration NodeJSTaskQueue::Now() const
{
    using namespace std::chrono;
    return high_resolution_clock::now().time_since_epoch() - epoch_;
}
TimeSpan NodeJSTaskQueue::TimeFloor() const
{
    using namespace std::chrono;
    return TimeSpan::Microseconds(floor<microseconds>(Now()).count());
}
TimeSpan NodeJSTaskQueue::TimeCeil() const
{
    using namespace std::chrono;
    return TimeSpan::Microseconds(ceil<microseconds>(Now()).count());
}

META_NS::ITaskQueue::Ptr GetOrCreateNodeTaskQueue(napi_env e)
{
    auto& tr = GetTaskQueueRegistry();
    if (const auto alreadyRegistered = tr.GetTaskQueue(JS_THREAD_DEP)) {
        return alreadyRegistered;
    }

    auto& obr = GetObjectRegistry();
    obr.RegisterObjectType<NodeJSTaskQueue>();

    auto params = obr.Create<IMetadata>(META_NS::ClassId::Object);
    if (!params) {
        return {};
    }
    auto p = ConstructProperty<uintptr_t>("env", uintptr_t(e), ObjectFlagBits::INTERNAL | ObjectFlagBits::NATIVE);
    if (!p) {
        return {};
    }
    params->AddProperty(p);
    const auto result = obr.Create<ITaskQueue>(::ClassId::NodeJSTaskQueue, params);
    if (result) {
        tr.RegisterTaskQueue(result, JS_THREAD_DEP);
    }
    return result;
}

bool DeinitNodeTaskQueue()
{
    auto& tr = GetTaskQueueRegistry();
    auto tq = tr.GetTaskQueue(JS_THREAD_DEP);
    if (!tq) {
        // already deinitialized
        return true;
    }
    if (!tq->GetInterface<INodeJSTaskQueue>()->IsReleased()) {
        // Unsafe deinitialize
        return false;
    }
    // can be safely unregistered.
    // expect the instance to be destroyed now.
    tr.UnregisterTaskQueue(JS_THREAD_DEP);
    GetTaskQueueRegistry().SetCurrentTaskQueue({});
    return true;
}
