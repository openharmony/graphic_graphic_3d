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
#ifndef META_API_CALL_CONTEXT_H
#define META_API_CALL_CONTEXT_H

#include <meta/base/expected.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/intf_call_context.h>
#include <meta/interface/intf_object_registry.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Set argument value for parameter
 * @param context Call context where the argument is set
 * @param name Name of the defined parameter
 * @param value to set, type must match with the defined parameter type.
 * @return True on success.
 */
template<typename Type>
bool Set(const ICallContext::Ptr& context, BASE_NS::string_view name, const Type& value)
{
    return context->Set(name, Any<Type>(value));
}

/**
 * @brief Set result value
 * @param context Call context where the result is set
 * @param value to set, type must match with the defined result type.
 */
template<typename Type>
bool SetResult(const ICallContext::Ptr& context, const Type& value)
{
    return context->SetResult(Any<Type>(value));
}

/**
 * @brief Set void result value
 * @param context Call context where the result is set
 */
template<typename Type = void, typename = BASE_NS::enable_if_t<BASE_NS::is_same_v<Type, void>>>
bool SetResult(const ICallContext::Ptr& context)
{
    return context->SetResult();
}

/**
 * @brief Get argument value if set, otherwise parameter default value.
 * @param context Call context to get from.
 * @param Name of the defined parameter, the type must match.
 * @return Pointer to contained value if successful, otherwise null.
 */
template<typename Type>
Expected<Type, GenericError> Get(const ICallContext::Ptr& context, BASE_NS::string_view name)
{
    if (auto any = context->Get(name)) {
        Type t;
        if (any->GetValue(t)) {
            return t;
        }
    }
    return GenericError::FAIL;
}

/**
 * @brief Get result value/type
 * @param context Call context to get from.
 * @return Pointer to contained result if type matches, otherwise null.
 * Note: One should first use ICallContext Success function to see if the call was
 *       successful before querying for value.
 */
template<typename Type>
Expected<Type, GenericError> GetResult(const ICallContext::Ptr& context)
{
    if (auto any = context->GetResult()) {
        Type t;
        if (any->GetValue(t)) {
            return t;
        }
    }
    return GenericError::FAIL;
}

/**
 * @brief Define parameter name, type and default value in call context
 * @param context Call context to define parameter.
 * @param name Name of the parameter.
 * @param value Type and default value of the parameter.
 */
template<typename Type>
bool DefineParameter(const ICallContext::Ptr& context, BASE_NS::string_view name, const Type& value = {})
{
    return context->DefineParameter(name, IAny::Ptr(new Any<Type>(value)));
}

/**
 * @brief Define result type in call context
 * @param context Call context for define result type
 * @value Type of the result, the value is ignored if SetResult is used.
 */
template<typename Type>
bool DefineResult(const ICallContext::Ptr& context, const Type& value)
{
    return context->DefineResult(IAny::Ptr(new Any<Type>(value)));
}

/**
 * @brief Define result type in call context
 */
template<typename Type>
bool DefineResult(const ICallContext::Ptr& context)
{
    if constexpr (BASE_NS::is_same_v<Type, void>) {
        return context->DefineResult(nullptr);
    }
    if constexpr (!BASE_NS::is_same_v<Type, void>) {
        return DefineResult(context, Type {});
    }
}

/**
 * @brief Set values for call context, used for setting parameters for meta function calls.
 */
template<typename... Args, size_t... Index>
bool SetContextValues(
    const ICallContext::Ptr& context, IndexSequence<Index...>, const BASE_NS::array_view<BASE_NS::string_view>& names)
{
    return (true && ... && DefineParameter<Args>(context, names[Index]));
}

/**
 * @brief Create call context for meta function calls with parameter names.
 * @param Ret Return type of the meta function.
 * @param Args Types of the meta function parameters.
 * @param names Names of the meta function parameters.
 */
template<typename Ret, typename... Args>
ICallContext::Ptr CreateCallContextImpl(const BASE_NS::array_view<BASE_NS::string_view>& names)
{
    if (sizeof...(Args) != names.size()) {
        CORE_LOG_E("Invalid amount of parameter names");
        return nullptr;
    }
    auto context = GetObjectRegistry().ConstructDefaultCallContext();
    if (!SetContextValues<PlainType_t<Args>...>(context, MakeIndexSequenceFor<Args...>(), names)) {
        CORE_LOG_E("Failed setting parameters");
        return nullptr;
    }
    if (!DefineResult<PlainType_t<Ret>>(context)) {
        CORE_LOG_E("Failed setting return type");
        return nullptr;
    }
    return context;
}

/**
 * @brief Create call context for meta function calls by deducing types from member function.
 */
template<typename Obj, typename Ret, typename... Args>
ICallContext::Ptr CreateCallContext(Ret (Obj::*)(Args...), const BASE_NS::array_view<BASE_NS::string_view>& names)
{
    return CreateCallContextImpl<Ret, Args...>(names);
}

/**
 * @brief Create call context for meta function calls by deducing types from member function.
 */
template<typename Obj, typename Ret, typename... Args>
ICallContext::Ptr CreateCallContext(Ret (Obj::*)(Args...) const, const BASE_NS::array_view<BASE_NS::string_view>& names)
{
    return CreateCallContextImpl<Ret, Args...>(names);
}

// convert array of string views to array view and ignore the first one (workaround for empty arrays)
template<size_t S>
BASE_NS::array_view<BASE_NS::string_view> ParamNameToView(BASE_NS::string_view (&arr)[S])
{
    return BASE_NS::array_view<BASE_NS::string_view>(arr + 1, arr + S);
}

template<typename T, bool Ref = BASE_NS::is_same_v<T, PlainType_t<T>&>>
struct CallArg {
    using Type = PlainType_t<T>;
    explicit CallArg(IAny::Ptr any) : any_(any) {}

    /* NOLINTNEXTLINE(google-explicit-constructor) */
    operator Type() const
    {
        Type t;
        any_->GetValue(t);
        return t;
    }

private:
    IAny::Ptr any_;
};

template<typename T>
struct CallArg<T, true> {
    using Type = PlainType_t<T>;
    explicit CallArg(IAny::Ptr any) : any_(any)
    {
        any_->GetValue(t_);
    }
    ~CallArg()
    {
        any_->SetValue(t_);
    }
    META_DEFAULT_COPY_MOVE(CallArg)

    /* NOLINTNEXTLINE(google-explicit-constructor) */
    operator Type&() const
    {
        return t_;
    }

private:
    IAny::Ptr any_;
    mutable Type t_;
};

template<typename Ret, typename... Args>
struct CallFunctionImpl {
    template<typename Func, size_t... Index>
    static bool Call(
        const ICallContext::Ptr& context, Func func, BASE_NS::array_view<IAny::Ptr> argView, IndexSequence<Index...>)
    {
        if (!(true && ... && IsGetCompatibleWith<Args>(*argView[Index]))) {
            context->ReportError("invalid argument type for function call");
            return false;
        }
        return [&](const auto&... args) {
            if constexpr (BASE_NS::is_same_v<Ret, void>) {
                func(args...);
                SetResult(context);
                return true;
            } else {
                auto ret = func(args...);
                return SetResult(context, ret);
            }
        }(CallArg<Args>(argView[Index])...);
    }

    template<typename Func, size_t... Index>
    static bool Call(const ICallContext::Ptr& context, Func func, IndexSequence<Index...> ind)
    {
        auto params = context->GetParameters();
        if (params.size() != sizeof...(Args)) {
            context->ReportError("invalid function call");
            return false;
        }
        if constexpr (sizeof...(Args) != 0) {
            IAny::Ptr args[] = { params[Index].value... };
            return Call(context, func, args, ind);
        } else {
            return Call(context, func, BASE_NS::array_view<IAny::Ptr> {}, ind);
        }
    }

    template<typename Func, size_t... Index>
    static bool Call(const ICallContext::Ptr& context, Func func,
        const BASE_NS::array_view<BASE_NS::string_view>& names, IndexSequence<Index...> ind)
    {
        auto params = context->GetParameters();
        if (params.size() != sizeof...(Args)) {
            context->ReportError("invalid function call");
            return false;
        }
        if constexpr (sizeof...(Args) != 0) {
            IAny::Ptr args[] = { context->Get(names[Index])... };
            return Call(context, func, args, ind);
        } else {
            return Call(context, func, BASE_NS::array_view<IAny::Ptr> {}, ind);
        }
    }
};

/**
 * @brief Call meta function, giving the arguments in the order they are in the context.
 * @param context Call context for meta function (i.e. arguments and return type information).
 * @param obj Target object to call function for.
 * @param func Member function to call.
 */
template<typename Obj, typename Ret, typename... Args>
bool CallFunction(const ICallContext::Ptr& context, Obj* obj, Ret (Obj::*func)(Args...))
{
    return CallFunctionImpl<Ret, Args...>::Call(
        context, [&](Args... args) { return (obj->*func)(args...); }, MakeIndexSequenceFor<Args...>());
}
template<typename Obj, typename Ret, typename... Args>
bool CallFunction(const ICallContext::Ptr& context, const Obj* obj, Ret (Obj::*func)(Args...) const)
{
    return CallFunctionImpl<Ret, Args...>::Call(
        context, [&](Args... args) { return (obj->*func)(args...); }, MakeIndexSequenceFor<Args...>());
}
template<typename Func>
bool CallFunction(const ICallContext::Ptr& context, Func func)
{
    return CallFunction(context, &func, &Func::operator());
}

/**
 * @brief Call meta function, giving the arguments in the order of the given parameter names.
 * @param context Call context for meta function (i.e. arguments and return type information).
 * @param obj Target object to call function for.
 * @param func Member function to call.
 */
template<typename Obj, typename Ret, typename... Args>
bool CallFunction(const ICallContext::Ptr& context, Obj* obj, Ret (Obj::*func)(Args...),
    const BASE_NS::array_view<BASE_NS::string_view>& names)
{
    return CallFunctionImpl<Ret, Args...>::Call(
        context, [&](Args... args) { return (obj->*func)(args...); }, names, MakeIndexSequenceFor<Args...>());
}
template<typename Obj, typename Ret, typename... Args>
bool CallFunction(const ICallContext::Ptr& context, const Obj* obj, Ret (Obj::*func)(Args...) const,
    const BASE_NS::array_view<BASE_NS::string_view>& names)
{
    return CallFunctionImpl<Ret, Args...>::Call(
        context, [&](Args... args) { return (obj->*func)(args...); }, names, MakeIndexSequenceFor<Args...>());
}

META_END_NAMESPACE()

#endif
