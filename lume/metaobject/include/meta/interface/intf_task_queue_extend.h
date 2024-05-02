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
#ifndef META_INTERFACE_ITASKQUEUE_EXTEND_H
#define META_INTERFACE_ITASKQUEUE_EXTEND_H

#include <meta/interface/intf_task_queue.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(ITaskQueueExtend, "952a030f-4b93-45b1-a2f3-ba352608d512")

class ITaskQueueExtend : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITaskQueueExtend);

public:
    virtual void SetExtend(ITaskQueueExtend* extend) = 0;
    virtual bool InvokeTask(const ITaskQueueTask::Ptr&) = 0;
    virtual void Shutdown() = 0;
};

META_END_NAMESPACE()

#endif // META_INTERFACE_ITASKQUEUE_H
