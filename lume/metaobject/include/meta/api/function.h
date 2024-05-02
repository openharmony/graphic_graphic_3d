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

#ifndef META_API_FUNCTION_H
#define META_API_FUNCTION_H

#include <meta/api/call_context.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_cloneable.h>
#include <meta/interface/intf_function.h>
#include <meta/interface/object_macros.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Function implementation that is used for the meta function system.
 */
template<typename Obj, typename Func>
class DefaultFunction : public IntroduceInterfaces<IFunction, ICloneable> {
public:
    ~DefaultFunction() override = default;
    DefaultFunction& operator=(const DefaultFunction&) noexcept = delete;
    META_NO_MOVE(DefaultFunction)

    BASE_NS::string GetName() const override
    {
        return name_;
    }

    IObject::ConstPtr GetDestination() const override
    {
        return interface_pointer_cast<IObject>(obj_);
    }

    void Invoke(const ICallContext::Ptr& context) const override
    {
        func_(obj_, context);
    }

    ICallContext::Ptr CreateCallContext() const override
    {
        if (context_) {
            return context_();
        }
        return nullptr;
    }

    BASE_NS::shared_ptr<CORE_NS::IInterface> GetClone() const override
    {
        BASE_NS::shared_ptr<DefaultFunction> p(new DefaultFunction(*this));
        return interface_pointer_cast<CORE_NS::IInterface>(p);
    }

    DefaultFunction(
        BASE_NS::string n, BASE_NS::weak_ptr<BASE_NS::remove_const_t<Obj>> obj, Func f, Internal::FContext* context)
        : name_(BASE_NS::move(n)), obj_(obj), func_(BASE_NS::move(f)), context_(context)
    {}

protected:
    DefaultFunction(const DefaultFunction& s) : name_(s.name_), obj_(s.obj_), func_(s.func_), context_(s.context_) {}

protected:
    BASE_NS::string name_;
    BASE_NS::weak_ptr<BASE_NS::remove_const_t<Obj>> obj_ {};
    Func func_;
    Internal::FContext* const context_;
};

/**
 * @brief Create DefaultFunction object for obj+memfun (used in metadata initialisation)
 */
template<typename Obj, typename MemFun>
IFunction::Ptr CreateFunction(BASE_NS::string_view name, Obj* obj, MemFun func, Internal::FContext* context)
{
    using ObjType = BASE_NS::shared_ptr<BASE_NS::remove_const_t<Obj>>;
    ObjType objPtr { obj->GetSelf(), obj };
    auto l = [func](auto obj, const ICallContext::Ptr& context) {
        if (auto o = obj.lock()) {
            (o.get()->*func)(context);
        }
    };
    return IFunction::Ptr(
        new DefaultFunction<Obj, decltype(l)>(BASE_NS::string(name), objPtr, BASE_NS::move(l), context));
}

/**
 * @brief Create DefaultFunction object from lambda
 */
template<typename Func, typename = EnableIfBindFunction<Func>>
IFunction::Ptr CreateBindFunction(Func func)
{
    auto ccontext = []() {
        ::BASE_NS::string_view arr[] = { "" };
        return CreateCallContextImpl<decltype(func())>(ParamNameToView(arr));
    };
    // wrap to make CallFunction to work with operator()(auto...)
    auto wrapper = [func]() mutable { return func(); };
    auto l = [wrapper](auto, const ICallContext::Ptr& context) { ::META_NS::CallFunction(context, wrapper); };
    return IFunction::Ptr(new DefaultFunction<IObject, decltype(l)>("Bind", nullptr, BASE_NS::move(l), ccontext));
}

/**
 * @brief Create DefaultFunction object from lambda
 */
template<typename Func, typename... Args>
IFunction::Ptr CreateBindFunctionSafe(Func func, Args&&... args)
{
    return CreateBindFunction(CaptureSafe(BASE_NS::move(func), BASE_NS::forward<Args>(args)...));
}

/**
 * @brief Create forwarding function object
 */
inline IFunction::Ptr CreateFunction(const IObject::Ptr& obj, BASE_NS::string_view name)
{
    if (auto f = META_NS::GetObjectRegistry().Create<ISettableFunction>(ClassId::SettableFunction)) {
        if (f->SetTarget(obj, name)) {
            return f;
        }
    }
    return nullptr;
}

/**
 * @brief Helper class for meta function call result.
 */
template<typename Type>
struct CallResult {
    explicit operator bool() const
    {
        return success;
    }

    /**
     * @brief Call context that was used for the call.
     */
    ICallContext::Ptr context;
    /**
     * @brief True if it was possible to make the call (i.e. the argument types match the parameters).
     */
    bool success {};
    /**
     * @brief Return value of the function call.
     */
    Type value {};
};

template<>
struct CallResult<void> {
    explicit operator bool() const
    {
        return success;
    }

    ICallContext::Ptr context;
    bool success {};
};

template<typename Ret, typename... Args, size_t... Index>
CallResult<Ret> CallMetaFunctionImpl(const IFunction::Ptr& func, IndexSequence<Index...>, Args&&... args)
{
    auto context = func->CreateCallContext();
    if (!context) {
        return {};
    }
    auto params = context->GetParameters();
    // Allow to use defaults from call context
    if (params.size() < sizeof...(Args)) {
        context->ReportError("invalid meta call");
        return { context };
    }

    if (!(true && ... && Set<PlainType_t<Args>>(context, params[Index].name, args))) {
        context->ReportError("invalid meta call");
        return { context };
    }

    func->Invoke(context);
    if (context->Succeeded()) {
        if constexpr (BASE_NS::is_same_v<Ret, void>) {
            return CallResult<Ret> { context, true };
        }
        if constexpr (!BASE_NS::is_same_v<Ret, void>) {
            if (auto p = GetResult<Ret>(context)) {
                return CallResult<Ret> { context, true, *p };
            }
        }
    }
    return { context };
}

/**
 * @brief Call function via interface with the given arguments.
 * @param func Function to call.
 * @param args Arguments for the function call.
 * @return Result of the call.
 * @see CallResult
 */
template<typename Ret, typename... Args>
CallResult<Ret> CallMetaFunction(const IFunction::Ptr& func, Args&&... args)
{
    return CallMetaFunctionImpl<Ret>(func, MakeIndexSequenceFor<Args...>(), BASE_NS::forward<Args>(args)...);
}

template<typename Signature>
struct IsFunctionCompatibleImpl;

template<typename Ret, typename... Args>
struct IsFunctionCompatibleImpl<Ret(Args...)> {
    template<size_t... Index>
    static bool Call(const IFunction::Ptr& func, IndexSequence<Index...>)
    {
        auto context = func->CreateCallContext();
        // e.g. wrong number of parameters when implementing meta function
        if (!context) {
            return false;
        }
        auto params = context->GetParameters();
        // Allow to use defaults from call context
        if (params.size() < sizeof...(Args)) {
            return false;
        }

        // if we have void, allow any return type, it will just be ignored
        if constexpr (!BASE_NS::is_same_v<Ret, void>) {
            if (!IsCompatibleWith<Ret>(context->GetResult())) {
                return false;
            }
        }

        if (!(true && ... && IsCompatibleWith<Args>(*params[Index].value))) {
            return false;
        }
        return true;
    }
    static bool Call(const IFunction::Ptr& func)
    {
        if constexpr ((true && ... && HasUid_v<PlainType_t<Args>>)) {
            return Call(func, MakeIndexSequenceFor<Args...>());
        }
        return false;
    }
};

/**
 * @brief Check if function is compatible with given signature (The return type and parameter types match).
 */
template<typename FuncSignature>
bool IsFunctionCompatible(const IFunction::Ptr& func)
{
    return IsFunctionCompatibleImpl<FuncSignature>::Call(func);
}

META_END_NAMESPACE()

#endif
