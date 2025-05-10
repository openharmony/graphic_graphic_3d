/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Callback helpers
 * Author: Mikael Kilpeläinen
 */
#ifndef META_API_MAKE_CALLBACK_H
#define META_API_MAKE_CALLBACK_H

#include <meta/base/capture.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_lifecycle.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE();

namespace {

template<typename Callable, typename Func, typename Signature = typename Callable::FunctionType>
class Callback;

template<typename Callable, typename Func, typename R, typename... ARG>
class Callback<Callable, Func, R(ARG...)> : public IntroduceInterfaces<Callable> {
public:
    Callback(Func f) : func_(BASE_NS::move(f)) {}

protected:
    Func func_;
    R Invoke(ARG... args) override
    {
        return func_(args...);
    }
};

} // namespace

/**
 * @brief MakeCallable creates a generic callable from callable entity (e.g. lambda).
 * @param CallableType Type that defines the callable interface, e.g. ITaskQueueTask
 */
template<typename CallableType, typename Func>
auto MakeCallback(Func f)
{
    return typename CallableType::Ptr(new Callback<CallableType, Func>(BASE_NS::move(f)));
}

/**
 * @brief As above MakeCallable but using capture helper. @see Capture.
 */
template<typename CallableType, typename Func, typename... Args>
auto MakeCallback(Func f, Args&&... args)
{
    return MakeCallback<CallableType>(Capture(BASE_NS::move(f), BASE_NS::forward<Args>(args)...));
}

/**
 * @brief Creates a generic callable from a class method.
 * Note: User needs to ensure that the lifetime of the class is longer than the created callback.
 */
template<typename CallableType, typename o, typename R, typename... ARG>
auto MakeCallback(o* instance, R (o::*func)(ARG...))
{
    return MakeCallback<CallableType>([instance, func](ARG... args) { return (instance->*func)(args...); });
}

/**
 * @brief Creates a generic callable from callable entity (e.g. lambda). The callable entity is executed only if
 * all captured shared pointers are valid.
 * @param BaseClass Interface that defines event, e.g. IOnChanged.
 * @param args Capture helper support.
 * @see Capture
 */
template<typename CallableType, typename Func, typename... Args>
auto MakeCallbackSafe(Func f, Args&&... args)
{
    return MakeCallback<CallableType>(CaptureSafe(BASE_NS::move(f), BASE_NS::forward<Args>(args)...));
}

META_END_NAMESPACE();
#endif
