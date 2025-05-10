/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: IEvent Helpers
 * Author: Mikael Kilpeläinen
 */

#ifndef META_INTERFACE_SIMPLE_EVENT_H
#define META_INTERFACE_SIMPLE_EVENT_H

#include <meta/base/meta_types.h>
#include <meta/interface/intf_event.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Helper class to define event types easily.
 * Example:
 *     struct IOnEventInfo {
 *         constexpr static BASE_NS::Uid UID { "uid..." };
 *     };
 *     using IOnEvent = META_NS::SimpleEvent<IOnEventInfo, void(int myparameter)>;
 * IOnEvent is event type that can be used with META(_IMPLEMENT)_EVENT macros.
 */
template<typename MyEvent, typename Signature = void()>
class SimpleEvent;

template<typename MyEvent, typename... Args>
class SimpleEvent<MyEvent, void(Args...)> : public IEventCallable<MyEvent, void, Args...> {
public:
    constexpr static char const* NAME { MyEvent::NAME };
};

// NOLINTBEGIN(readability-identifier-naming)
/**
 * @brief Check if type is an event.
 */
template<typename Type>
constexpr bool IsEvent_v = BASE_NS::is_convertible_v<const Type*, const IEvent*>;
// NOLINTEND(readability-identifier-naming)

META_END_NAMESPACE()

#endif
