/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "graphics_task.h"
#include <mutex>
#include <utility>
#include <pthread.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/time.h>

#include "3d_widget_adapter_log.h"

namespace OHOS::Render3D {
GraphicsTask::Message::Message(const std::function<Task>& task)
    : task_(std::move(task))
{}

GraphicsTask::Message::Message(GraphicsTask::Message&& msg)
    : task_(std::move(msg.task_)), pms_(std::move(msg.pms_)), ftr_(std::move(msg.ftr_))
{}

GraphicsTask::Message& GraphicsTask::Message::operator=(GraphicsTask::Message&& msg)
{
    task_ = std::move(msg.task_);
    pms_ = std::move(msg.pms_);
    ftr_ = std::move(msg.ftr_);
    return *this;
}

void GraphicsTask::Message::Execute()
{
    task_();
    Finish();
}

void GraphicsTask::Message::Finish()
{
    pms_.set_value();
}

std::shared_future<void> GraphicsTask::Message::GetFuture()
{
    return std::move(ftr_);
}

GraphicsTask& GraphicsTask::GetInstance()
{
    static GraphicsTask gfxTask;
    return gfxTask;
}

void GraphicsTask::PushSyncMessage(const std::function<Task>& task)
{
    std::shared_future<void> ftr;
    {
        std::lock_guard<std::mutex> lk(messageQueueMut_);
        ftr = messageQueue_.emplace(std::move(task)).GetFuture();
        messageQueueCnd_.notify_one();
    }

    if (ftr.valid()) {
        ftr.get();
    }
}

std::shared_future<void> GraphicsTask::PushAsyncMessage(const std::function<Task>& task)
{
    std::lock_guard<std::mutex> lk(messageQueueMut_);

    Message& msg = messageQueue_.emplace(std::move(task));
    messageQueueCnd_.notify_one();

    return msg.GetFuture();
}

GraphicsTask::GraphicsTask()
{
    Start();
}

GraphicsTask::~GraphicsTask()
{
    Stop();
    {
        std::lock_guard<std::mutex> lk(messageQueueMut_);
        while (!messageQueue_.empty()) {
            messageQueue_.front().Finish();
            messageQueue_.pop();
        }
    }

    if (loop_.joinable()) {
        loop_.join();
    }
}

void GraphicsTask::Start()
{
    WIDGET_LOGD("GraphicsTask::Start start");

    if (!exit_) {
        return;
    }
    exit_ = false;
    loop_ = std::thread(std::bind(&GraphicsTask::EngineThread, this));
    PushAsyncMessage(std::bind(&GraphicsTask::SetName, this));
    WIDGET_LOGD("GraphicsTask::Start end");
}

void GraphicsTask::Stop()
{
    exit_ = true;
}

void GraphicsTask::EngineThread()
{
    WIDGET_LOGD("GraphicsTask::EngineThread execute start");
    do {
        std::unique_lock<std::mutex> lk(messageQueueMut_);
        messageQueueCnd_.wait(lk, [this] { return !messageQueue_.empty(); });

        Message msg(std::move(messageQueue_.front()));
        messageQueue_.pop();
        lk.unlock();

        msg.Execute();
    } while (!exit_);

    WIDGET_LOGD("GraphicsTask::EngineThread execute exit");
}

void GraphicsTask::SetName()
{
    WIDGET_LOGD("GraphicsTask::SetName start");
    prctl(PR_SET_NAME, "Engine Service Lume", 0, 0, 0);
}
} // namespace OHOS::Render3D
