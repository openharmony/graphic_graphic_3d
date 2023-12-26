/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_RENDER_3D_GRAPHICS_TASK_H
#define OHOS_RENDER_3D_GRAPHICS_TASK_H

#include <shared_mutex>
#include <future>
#include <queue>
#include <functional>
#include <thread>

namespace OHOS::Render3D {
class __attribute__((visibility("default"))) GraphicsTask {
public:
    using Task = void();
    class Message {
    public:
        explicit Message(const std::function<Task>& task);
        Message(Message&& msg);
        Message& operator=(Message&& msg);
        ~Message() = default;

        void Execute();
        void Finish();

    private:
        friend GraphicsTask;
        std::function<Task> task_;
        std::promise<void> pms_;
        std::future<void> ftr_ = pms_.get_future();
        std::shared_future<void> GetFuture();
    };

    static GraphicsTask& GetInstance();
    void PushSyncMessage(const std::function<Task>& task);
    std::shared_future<void> PushAsyncMessage(const std::function<Task>& task);

private:
    GraphicsTask(const GraphicsTask&) = delete;
    GraphicsTask& operator=(const GraphicsTask&) = delete;
    GraphicsTask();
    ~GraphicsTask();

    void Start();
    void Stop();
    void EngineThread();
    void SetName();

    std::queue<Message> messageQueue_;
    std::condition_variable messageQueueCnd_;
    std::mutex messageQueueMut_;

    std::thread loop_;
    std::atomic_bool exit_ = true;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_GRAPHIC_TASK_H
