/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_RENDER_3D_BASE_OBJECT_JS_H
#define OHOS_RENDER_3D_BASE_OBJECT_JS_H
#include <meta/api/make_callback.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <napi_api.h>

// tasks execute in the engine/render thread.
static constexpr BASE_NS::Uid ENGINE_THREAD { "2070e705-d061-40e4-bfb7-90fad2c280af" };

// tasks execute in the javascript mainthread.
static constexpr BASE_NS::Uid JS_THREAD { "b2e8cef3-453a-4651-b564-5190f8b5190d" };

class TrueRootObject {
public:
    // Store a reference to a native IObject to the "JSBridge" object
    // Optionally make the reference strong so that the lifetime is controlled by JS.
    // for example. scene is kept strongly, and objects owned by scene are weak.

    void SetNativeObject(META_NS::IObject::Ptr real, bool Strong);
    META_NS::IObject::Ptr GetNativeObject();
    virtual void* GetInstanceImpl(uint32_t id) = 0;

    virtual void Finalize(napi_env env);
    virtual void DisposeNative() = 0;

protected:
    TrueRootObject();
    virtual ~TrueRootObject() = default;
    static void destroy(TrueRootObject* object)
    {
        delete object;
    }

private:
    META_NS::IObject::Ptr obj_;
    META_NS::IObject::WeakPtr objW_;
};

template<class FinalObject>
class BaseObject : public TrueRootObject {
protected:
    bool disposed_ { false };
    virtual ~BaseObject() {};
    BaseObject(napi_env env, napi_callback_info info) : TrueRootObject()
    {
        napi_value thisVar = nullptr;
        napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr);

        auto DTOR = [](napi_env env, void* nativeObject, void* finalize) {
            TrueRootObject* ptr = static_cast<TrueRootObject*>(nativeObject);
            ptr->Finalize(env);
            TrueRootObject::destroy(ptr);
        };
        napi_wrap(env, thisVar, reinterpret_cast<void*>((TrueRootObject*)this), DTOR, nullptr, nullptr);
    }
    template<typename Object>
    static inline napi_callback ctor()
    {
        napi_callback ctor = [](napi_env env, napi_callback_info info) -> napi_value {
            napi_value thisVar = nullptr;
            // fetch the "this" from javascript.
            napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr);
            // The BaseObject constructor actually handles wrapping..
            auto r = BASE_NS::make_unique<Object>(env, info);
            r.release();
            return thisVar;
        };
        return ctor;
    }
};

static inline TrueRootObject* GetRootObject(napi_env e, napi_value o)
{
    TrueRootObject* p { nullptr };
    napi_unwrap(e, o, (void**)&p);
    return p;
}

template<typename... types>
static inline TrueRootObject* GetThisRootObject(NapiApi::FunctionContext<types...>& ctx)
{
    return GetRootObject(ctx, ctx.This());
}

template<typename FC>
static inline auto GetThisNativeObject(FC& ctx)
{
    TrueRootObject* instance = GetThisRootObject(ctx);
    if (!instance) {
        return META_NS::IObject::Ptr {};
    }
    return instance->GetNativeObject();
}

template<typename Object, napi_value (Object::*F)(NapiApi::FunctionContext<>&)>
static inline napi_value TROGetter(napi_env env, napi_callback_info info)
{
    NapiApi::FunctionContext fc(env, info);
    if (fc) {
        if (TrueRootObject* instance = GetThisRootObject(fc)) {
            if (Object* impl = (Object*)instance->GetInstanceImpl(Object::ID)) {
                return (impl->*F)(fc);
            }
        };
    }
    napi_value undefineVar;
    napi_get_undefined(env, &undefineVar);
    return undefineVar;
}
template<typename Type, typename Object, void (Object::*F)(NapiApi::FunctionContext<Type>&)>
static inline napi_value TROSetter(napi_env env, napi_callback_info info)
{
    NapiApi::FunctionContext<Type> fc(env, info);
    if (fc) {
        if (TrueRootObject* instance = GetThisRootObject(fc)) {
            if (Object* impl = (Object*)instance->GetInstanceImpl(Object::ID)) {
                (impl->*F)(fc);
            }
        }
    }
    napi_value undefineVar;
    napi_get_undefined(env, &undefineVar);
    return undefineVar;
};

template<typename Type, typename Object, void (Object::*F2)(NapiApi::FunctionContext<Type>&)>
static inline napi_property_descriptor TROSetProperty(
    const char* const name, napi_property_attributes flags = napi_default_jsproperty)
{
    static_assert(F2 != nullptr);
    return napi_property_descriptor { name, nullptr, nullptr, nullptr, TROSetter<Type, Object, F2>, nullptr, flags,
        nullptr };
}

template<typename Type, typename Object, napi_value (Object::*F)(NapiApi::FunctionContext<>&)>
static inline napi_property_descriptor TROGetProperty(
    const char* const name, napi_property_attributes flags = napi_default_jsproperty)
{
    static_assert(F != nullptr);
    return napi_property_descriptor { name, nullptr, nullptr, TROGetter<Object, F>, nullptr, nullptr, flags, nullptr };
}

template<typename Type, typename Object, napi_value (Object::*F)(NapiApi::FunctionContext<>&),
    void (Object::*F2)(NapiApi::FunctionContext<Type>&)>
static inline napi_property_descriptor TROGetSetProperty(
    const char* const name, napi_property_attributes flags = napi_default_jsproperty)
{
    static_assert(F != nullptr);
    static_assert(F2 != nullptr);
    return napi_property_descriptor { name, nullptr, nullptr, TROGetter<Object, F>, TROSetter<Type, Object, F2>,
        nullptr, flags, nullptr };
}

template<typename FC, typename Object, napi_value (Object::*F)(FC&)>
napi_value TROMethod(napi_env env, napi_callback_info info)
{
    FC fc(env, info);
    if (fc) {
        if (TrueRootObject* instance = GetThisRootObject(fc)) {
            if (Object* impl = (Object*)instance->GetInstanceImpl(Object::ID)) {
                return (impl->*F)(fc);
            }
        }
    }
    napi_value undefineVar;
    napi_get_undefined(env, &undefineVar);
    return undefineVar;
}

template<typename FC, typename Object, napi_value (Object::*F)(FC&)>
inline napi_property_descriptor MakeTROMethod(
    const char* const name, napi_property_attributes flags = napi_default_method)
{
    return napi_property_descriptor { name, nullptr, TROMethod<FC, Object, F>, nullptr, nullptr, nullptr, flags,
        nullptr };
}

template<typename type>
auto GetNativeMeta(NapiApi::Object obj)
{
    if (obj) {
        auto* tro = obj.Native<TrueRootObject>();
        if (tro) {
            return interface_pointer_cast<type>(tro->GetNativeObject());
        }
    }
    return typename type::Ptr {};
}

//  uses the classid of obj to create correct wrapper.
//  (if the wrapper already exists, returns a new reference to the wrapper)
NapiApi::Object CreateFromNativeInstance(
    napi_env env, const META_NS::IObject::Ptr& obj, bool strong, uint32_t argc, napi_value* argv);
NapiApi::Object CreateJsObj(napi_env env, const BASE_NS::string_view jsName, META_NS::IObject::Ptr real, bool strong,
    uint32_t argc, napi_value* argv);

// check for type.
bool IsInstanceOf(const NapiApi::Object& obj, const BASE_NS::string_view jsName);

// run synchronous task in specific tq.
template<typename func>
META_NS::IAny::Ptr ExecSyncTask(const META_NS::ITaskQueue::Ptr tq, func&& fun)
{
    return tq
        ->AddWaitableTask(BASE_NS::move(META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>(BASE_NS::move(fun))))
        ->GetResult();
}

// run task synchronously in engine thread.
template<typename func>
META_NS::IAny::Ptr ExecSyncTask(func&& fun)
{
    return ExecSyncTask(META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD), BASE_NS::move(fun));
}
template<class type>
void MakeNativeObjectParam(napi_env env, const type& obj, uint32_t argc, napi_value* argv)
{
    // okay.. we "know" that arg[1] should be params.. so add the native object there automatically
    if (argc > 1) {
        napi_value res;
        if (auto mobj = interface_pointer_cast<META_NS::IObject>(obj)) {
            META_NS::IObject::WeakPtr* data = new META_NS::IObject::WeakPtr();
            *data = mobj;
            napi_create_external(
                env, (void*)data,
                [](napi_env env, void* data, void* finalize_hint) {
                    delete (META_NS::IObject::WeakPtr*)data;
                }, nullptr,
                &res);
            NapiApi::Object arg(env, argv[1]);
            arg.Set("NativeObject", res);
        }
    }
}
template<class type>
auto GetNativeObjectParam(NapiApi::Object args)
{
    typename type::Ptr ret;
    if (auto prm = args.Get("NativeObject")) {
        META_NS::IObject::WeakPtr* ptr;
        napi_get_value_external(args.GetEnv(), prm, (void**)&ptr);
        ret = interface_pointer_cast<type>(*ptr);
        napi_value null;
        napi_get_null(args.GetEnv(), &null);
        args.Set("NativeObject", null); // hope to release it now.
    }
    return ret;
}
#endif // OHOS_RENDER_3D_BASE_OBJECT_JS_H
