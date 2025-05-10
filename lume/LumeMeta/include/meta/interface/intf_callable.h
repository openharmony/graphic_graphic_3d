/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Interface for callable types.
 */
#ifndef META_INTERFACE_ICALLABLE_H
#define META_INTERFACE_ICALLABLE_H

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(ICallable, "161005ee-c4e6-4f1c-aab6-7d0dc31e2778")
/**
 * @brief Generic callable that can be bound to IEvent
 *        (typeless, need to use Uid to cast to proper callable interface).
 */
class ICallable : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ICallable)

public:
};

META_END_NAMESPACE()
#endif
