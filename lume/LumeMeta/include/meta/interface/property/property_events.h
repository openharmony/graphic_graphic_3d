/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Property interface
 * Author: Mikael Kilpeläinen
 * Create: 2023-10-18
 */

#ifndef META_INTERFACE_PROPERTY_PROPERTY_EVENTS_H
#define META_INTERFACE_PROPERTY_PROPERTY_EVENTS_H

#include <meta/interface/simple_event.h>

META_BEGIN_NAMESPACE()

/**
 * @brief The IOnTypedChanged interface defines a user-bindable interface for reacting to property value changes with
 * parameter.
 */
struct IOnTypedChangedInfo {
    constexpr static BASE_NS::Uid UID { "4c6fc26c-1d8a-4319-b95f-e2219560c1c5" };
    constexpr static char const* NAME { "OnChanged" };
};
using IOnChanged = SimpleEvent<IOnTypedChangedInfo, void()>;

META_END_NAMESPACE()

#endif
