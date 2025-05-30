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
#ifndef BASE_OBJECT_JS_H
#define BASE_OBJECT_JS_H
#include <meta/api/make_callback.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <napi_api.h>
#include <scene/interface/intf_scene.h>

#include "TrueRootObject.h"

// tasks execute in the engine/render thread.
static constexpr BASE_NS::Uid ENGINE_THREAD { "2070e705-d061-40e4-bfb7-90fad2c280af" };

// tasks execute in the javascript mainthread. *NOT IMPLEMENTED*
static constexpr BASE_NS::Uid JS_THREAD { "b2e8cef3-453a-4651-b564-5190f8b5190d" };

class BaseObject : public TrueRootObject {
protected:
    bool disposed_ { false };
    virtual ~BaseObject() {}
    BaseObject(napi_env env, napi_callback_info info) : TrueRootObject(env, info)
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

template<typename Object, napi_value (Object::*F)(NapiApi::FunctionContext<>&)>
static inline napi_value TROGetter(napi_env env, napi_callback_info info)
{
    NapiApi::FunctionContext fc(env, info);
    if (fc) {
        if (TrueRootObject* instance = fc.This().GetRoot()) {
            if (Object* impl = (Object*)instance->GetInstanceImpl(Object::ID)) {
                return (impl->*F)(fc);
            }
        };
    }
    return fc.GetUndefined();
}
template<typename Type, typename Object, void (Object::*F)(NapiApi::FunctionContext<Type>&)>
static inline napi_value TROSetter(napi_env env, napi_callback_info info)
{
    NapiApi::FunctionContext<Type> fc(env, info);
    if (fc) {
        if (TrueRootObject* instance = fc.This().GetRoot()) {
            if (Object* impl = (Object*)instance->GetInstanceImpl(Object::ID)) {
                (impl->*F)(fc);
            }
        }
    }
    return fc.GetUndefined();
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
        if (TrueRootObject* instance = fc.This().GetRoot()) {
            if (Object* impl = (Object*)instance->GetInstanceImpl(Object::ID)) {
                return (impl->*F)(fc);
            }
        }
    }
    return fc.GetUndefined();
}

template<typename FC, typename Object, napi_value (Object::*F)(FC&)>
inline napi_property_descriptor MakeTROMethod(
    const char* const name, napi_property_attributes flags = napi_default_method)
{
    return napi_property_descriptor { name, nullptr, TROMethod<FC, Object, F>, nullptr, nullptr, nullptr, flags,
        nullptr };
}

//  Create a javascript object that wraps specified IObject.
//  uses the classid of obj to create correct wrapper.
//  (if the wrapper already exists, returns a new reference to the wrapper)
NapiApi::Object CreateFromNativeInstance(napi_env env, const META_NS::IObject::Ptr& obj, PtrType ptrType,
    const NapiApi::JsFuncArgs& args, BASE_NS::string_view pname = "_JSW");

NapiApi::Object CreateFromNativeInstance(napi_env env, const BASE_NS::string& name, const META_NS::IObject::Ptr& obj,
    PtrType ptrType, const NapiApi::JsFuncArgs& args, BASE_NS::string_view pname = "_JSW");

template<typename ObjectPtr>
NapiApi::Object CreateFromNativeInstance(napi_env env, const ObjectPtr& obj, PtrType ptrType,
    const NapiApi::JsFuncArgs& args, BASE_NS::string_view pname = "_JSW")
{
    const auto iobj = interface_pointer_cast<META_NS::IObject>(obj);
    return CreateFromNativeInstance(env, iobj, ptrType, args, pname);
}

template<typename ObjectPtr>
NapiApi::Object CreateFromNativeInstance(napi_env env, const BASE_NS::string& name, const ObjectPtr& obj,
    PtrType ptrType, const NapiApi::JsFuncArgs& args, BASE_NS::string_view pname = "_JSW")
{
    const auto iobj = interface_pointer_cast<META_NS::IObject>(obj);
    return CreateFromNativeInstance(env, name, iobj, ptrType, args, pname);
}

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

#endif
