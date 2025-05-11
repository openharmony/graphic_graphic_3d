/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Generic task/job queue interfaces
 * Author: Jani Kattelus
 * Create: 2022-02-10
 */
#ifndef META_INTERFACE_ITASKQUEUE_EXTEND_H
#define META_INTERFACE_ITASKQUEUE_EXTEND_H

#include <meta/interface/intf_task_queue.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(ITaskQueueExtend, "952a030f-4b93-45b1-a2f3-ba352608d512")

class ITaskQueueExtend : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITaskQueueExtend)

public:
    virtual void SetExtend(ITaskQueueExtend* extend) = 0;
    virtual bool InvokeTask(const ITaskQueueTask::Ptr&) = 0;
    virtual void Shutdown() = 0;
};

META_END_NAMESPACE()

#endif // META_INTERFACE_ITASKQUEUE_H
