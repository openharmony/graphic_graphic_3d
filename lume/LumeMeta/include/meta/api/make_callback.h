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
