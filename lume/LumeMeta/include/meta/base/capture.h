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

#ifndef META_BASE_CAPTURE_HEADER
#define META_BASE_CAPTURE_HEADER

#include <meta/base/namespace.h>
#include <meta/base/shared_ptr.h>

META_BEGIN_NAMESPACE()

namespace Details {

/**
 * @brief Generic capture conversion.
 */
template<typename Type>
Type&& CaptureWrap(Type&& obj)
{
    return BASE_NS::forward<Type>(obj);
}

/**
 * @brief Helper to keep wrap type info.
 */
template<typename T>
struct CaptureWrapper {
    T value;
};

/**
 * @brief Capture conversion for shared_ptr.
 */
template<typename Type>
CaptureWrapper<BASE_NS::weak_ptr<Type>> CaptureWrap(BASE_NS::shared_ptr<Type> p)
{
    return { p };
}

/**
 * @brief Capture conversion for weak_ptr.
 */
template<typename Type>
CaptureWrapper<BASE_NS::weak_ptr<Type>> CaptureWrap(BASE_NS::weak_ptr<Type> p)
{
    return { BASE_NS::move(p) };
}

/**
 * @brief Helper to keep unwrapped type info.
 */
template<typename T>
struct CaptureUnWrapper {
    T value;
    bool valid { true };
};

/**
 * @brief Generic unwrap function that is called before the object is passed to the actual target function.
 * This should be overloaded for wrapped types.
 */
template<typename Type>
CaptureUnWrapper<Type> CaptureUnWrap(Type&& obj)
{
    return CaptureUnWrapper<Type> { BASE_NS::forward<Type>(obj) };
}

/**
 * @brief Capture conversion for wrapped type to shared_ptr.
 */
template<typename Type>
CaptureUnWrapper<BASE_NS::shared_ptr<Type>> CaptureUnWrap(const CaptureWrapper<BASE_NS::weak_ptr<Type>>& p)
{
    auto v = p.value.lock();
    return CaptureUnWrapper<BASE_NS::shared_ptr<Type>> { v, v != nullptr };
}

/**
 * @brief Invokes a captured function.
 * @tparam Check Decides if captured parameters should be valid before func is invoked.
 */
template<bool Check, typename... Other>
struct CaptureCallImpl {
    template<typename Func, typename... Args>
    static auto Call(Func& func, Other&&... other, Args&&... args)
    {
        if constexpr (Check) {
            if ((false || ... || !args.valid)) {
                using type = decltype(func(args.value..., BASE_NS::forward<decltype(other)>(other)...));
                if constexpr (BASE_NS::is_same_v<type, void>) {
                    return;
                } else {
                    return type {};
                }
            }
        }
        return func(args.value..., BASE_NS::forward<decltype(other)>(other)...);
    }
};

template<bool Check, typename Lambda, typename... Args>
auto CaptureImpl(Lambda func, Args&&... args)
{
    return [f = BASE_NS::move(func), args...](auto&&... other) {
        return CaptureCallImpl<Check, decltype(other)...>::Call(
            f, BASE_NS::forward<decltype(other)>(other)..., CaptureUnWrap(args)...);
    };
}

} // namespace Details

/**
 * @brief Replaces all shared pointers provided as args into weak pointers to avoid extending
 * lifetime of the pointed resources. The provided func is wrapped into capture call which first
 * locks back all weak pointers created from shared pointers and then calls the func.
 *
 * @param func Callable which will be wrapped into capture call.
 * @param args Arguments list which will be passed to the func as parameters when the capture call will have place.
 *             All shared pointers in it will be replaced by weak pointers.
 * @return Capture call which will not extend the lifetime of resources pointed by shared pointers provided as args.
 */
template<typename Lambda, typename... Args>
decltype(auto) Capture(Lambda func, Args&&... args)
{
    return Details::CaptureImpl<false>(BASE_NS::move(func), Details::CaptureWrap(BASE_NS::forward<Args>(args))...);
}

/**
 * @brief Wraps func into capture call which checks first if all shared pointers created from weak pointers points
 * to the valid resources, and if the condition is met executes the func.
 * If this condition is not met, capture call will return the default object.
 *
 * @param func Callable which will be wrapped into capture call.
 * @param args Arguments list which will be passed to the func as parameters when the capture call will have place.
 *             All shared pointers in it will be replaced by weak pointers.
 * @return Capture call which will not extend the lifetime of resources pointed by shared pointers provided as args,
 * and which will validate all resources before func call will have place.
 *
 * @see Capture
 */
template<typename Lambda, typename... Args>
decltype(auto) CaptureSafe(Lambda func, Args&&... args)
{
    return Details::CaptureImpl<true>(BASE_NS::move(func), Details::CaptureWrap(BASE_NS::forward<Args>(args))...);
}

template<typename Lambda, typename Ret, typename... Args>
void AssureCaptureTypeAndNoCapture()
{
    using fp = Ret (*)(Args...);
    static_assert(BASE_NS::is_convertible_v<Lambda, fp>, "Type mismatch or lambda capture");
}

META_END_NAMESPACE()

#endif
