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

#include "PropertyProxy.h"

#include <meta/api/make_callback.h>
#include <meta/api/util.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <napi_api.h>
#include <scene/ext/intf_internal_scene.h>

#include "BaseObjectJS.h"
#include "Vec2Proxy.h"
#include "Vec3Proxy.h"
#include "Vec4Proxy.h"

PropertyProxy::~PropertyProxy()
{
    // should be a no-op.
    RemoveHandlers();
}

void PropertyProxy::UpdateLocal()
{
    // should execute in engine thread.
    Lock();
    if (prop_) {
        UpdateLocalValues();
    }
    Unlock();
}
int32_t PropertyProxy::UpdateRemote()
{
    // executed in engine thread (ie. happens between frames)
    Lock();
    if (prop_) {
        // make sure the handler is not called..
        auto token = changeToken_;
        if (token) {
            prop_->OnChanged()->RemoveHandler(token);
        }

        UpdateRemoteValues();

        if (auto ext = interface_pointer_cast<SCENE_NS::IInternalScene>(GetExtra())) {
            ext->SyncProperty(prop_, META_NS::EngineSyncDirection::AUTO);
        }

        // add it back.
        if (token) {
            changeToken_ = prop_->OnChanged()->AddHandler(changeHandler_);
        }
    }
    updateToken_ = nullptr;
    Unlock();
    return 0;
}

void PropertyProxy::ScheduleUpdate()
{
    // create a task to engine queue to sync the property.
    if (updateToken_ == nullptr) {
        // sync not queued, so queue sync.
        updateToken_ = META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD)->AddTask(updateTask_);
    }
}

PropertyProxy::PropertyProxy(META_NS::IProperty::Ptr prop) : prop_(prop)
{
    assert(prop_);
    changeHandler_ = META_NS::MakeCallback<META_NS::IOnChanged>(this, &PropertyProxy::UpdateLocal);
    updateTask_ = META_NS::MakeCallback<META_NS::ITaskQueueTask>(this, &PropertyProxy::UpdateRemote);
    changeToken_ = prop_->OnChanged()->AddHandler(changeHandler_);
}

void PropertyProxy::SetExtra(const BASE_NS::shared_ptr<CORE_NS::IInterface> extra)
{
    extra_ = extra;
}

const BASE_NS::shared_ptr<CORE_NS::IInterface> PropertyProxy::GetExtra() const
{
    return extra_.lock();
}

void PropertyProxy::RemoveHandlers()
{
    if (!changeHandler_) {
        return;
    }
    ExecSyncTask([this]() {
        if (updateToken_) {
            META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD)->CancelTask(updateToken_);
            updateToken_ = nullptr;
        }
        if (prop_ && changeToken_) {
            prop_->OnChanged()->RemoveHandler(changeToken_);
        }
        changeToken_ = {};
        prop_.reset();
        changeHandler_.reset();
        updateTask_.reset();
        return META_NS::IAny::Ptr {};
    });
}
void PropertyProxy::SyncGet()
{
    // initialize current values.
    ExecSyncTask([this]() {
        // executed in engine thread
        UpdateLocal();
        return META_NS::IAny::Ptr {};
    });
}
void PropertyProxy::Lock()
{
    lock_.Lock();
}
void PropertyProxy::Unlock()
{
    lock_.Unlock();
}

ObjectPropertyProxy::MemberProxy::MemberProxy(ObjectPropertyProxy* p, BASE_NS::string m) : proxy_(p), memb_(m) {}
ObjectPropertyProxy::MemberProxy::~MemberProxy() {}
const BASE_NS::string_view ObjectPropertyProxy::MemberProxy::Name() const
{
    return memb_;
}

napi_value ObjectPropertyProxy::MemberProxy::Getter(napi_env e, napi_callback_info i)
{
    NapiApi::FunctionContext<> info(e, i);
    auto pc = static_cast<ObjectPropertyProxy::MemberProxy*>(info.GetData());
    if ((pc) && (pc->proxy_)) {
        return pc->proxy_->GetMemberValue(info.Env(), pc->memb_);
    }
    return info.GetUndefined();
}
napi_value ObjectPropertyProxy::MemberProxy::Setter(napi_env e, napi_callback_info i)
{
    NapiApi::FunctionContext<> info(e, i);
    auto pc = static_cast<ObjectPropertyProxy::MemberProxy*>(info.GetData());
    if ((pc) && (pc->proxy_)) {
        pc->proxy_->SetMemberValue(info, pc->memb_);
    }
    return info.GetUndefined();
}

void ObjectPropertyProxy::Create(napi_env env, const BASE_NS::string jsName)
{
    if (jsName.empty()) {
        obj_ = NapiApi::StrongRef { NapiApi::Object(env) };
    } else {
        NapiApi::MyInstanceState* mis;
        napi_get_instance_data(env, reinterpret_cast<void**>(&mis));
        auto ref = NapiApi::Object(env, mis->getRef());
        auto cl = ref.Get(jsName.c_str());
        if (cl) {
            napi_value value;
            napi_new_instance(env, cl, 0, nullptr, &value);
            obj_ = NapiApi::StrongRef { NapiApi::Object(env, value) };
        }
    }
    if (obj_.IsEmpty()) {
        CORE_LOG_F("Could not create property object for %s", jsName.c_str());
    }
}

void ObjectPropertyProxy::Hook(const BASE_NS::string member)
{
    if (obj_.IsEmpty()) {
        return;
    }
    auto ValueObject = obj_.GetObject();

    auto* accessor = new MemberProxy(this, member);
    accessors_.push_back(BASE_NS::unique_ptr<MemberProxy>(accessor));
    ValueObject.AddProperty({ accessor->Name().data(), nullptr, nullptr, MemberProxy::Getter, MemberProxy::Setter,
        nullptr, napi_default_jsproperty, static_cast<void*>(accessor) });
}

void ObjectPropertyProxy::Reset()
{
    napi_status status;
    if (obj_.IsEmpty()) {
        return;
    }

    RemoveHandlers();

    NapiApi::Env env(obj_.GetEnv());
    // unhook all hooked members.
    auto ValueObject = obj_.GetObject();
    for (; !accessors_.empty();) {
        auto it(accessors_.begin());
        // remove the property
        ValueObject.DeleteProperty((*it)->Name());
        // delete accessor
        accessors_.erase(it);
    }
    bool exception = false;
    status = napi_is_exception_pending(env, &exception);
    if (exception) {
        // if it's set, just clear it.
        // interestingly even though the napi_*_property might have returned napi_pending_exception (or other error
        // state) it seems that napi_is_exception_pending WILL return false though.
        napi_value res;
        status = napi_get_and_clear_last_exception(env, &res);
    }
}

void ObjectPropertyProxy::Destroy()
{
    Reset();
    // release ref.
    obj_.Reset();
}

napi_value ObjectPropertyProxy::Value()
{
    return obj_.GetValue();
}

void ObjectPropertyProxy::SetValue(NapiApi::FunctionContext<>& info)
{
    NapiApi::FunctionContext<NapiApi::Object> data { info };
    if (data) {
        SetValue(data.Arg<0>());
    }
}

ObjectPropertyProxy::ObjectPropertyProxy(META_NS::IProperty::Ptr prop) : PropertyProxy(prop) {}

ObjectPropertyProxy::~ObjectPropertyProxy()
{
    Reset();
}
void BitmapProxy::Reset()
{
    Lock();
    if (!prop_) {
        Unlock();
        return;
    }
    BASE_NS::string name = prop_->GetName();
    RemoveHandlers();

    if (!obj_.IsEmpty()) {
        obj_.GetObject().DeleteProperty(name);
    }
    obj_.Reset();
    scene_.Reset();
    Unlock();
}

BitmapProxy::BitmapProxy(NapiApi::Object scn, NapiApi::Object obj, META_NS::Property<SCENE_NS::IBitmap::Ptr> prop)
    : PropertyProxy(prop)
{
    scene_ = scn;
    obj_ = obj;
    SyncGet();
}
BitmapProxy::~BitmapProxy()
{
    // Unhook the objects.
    Reset();
}

void BitmapProxy::UpdateLocalValues()
{
    // executed in engine thread (locks handled outside)
    if (prop_) {
        value = META_NS::Property<SCENE_NS::IBitmap::Ptr>(prop_)->GetValue();
    }
}
void BitmapProxy::UpdateRemoteValues()
{
    // executed in engine thread (locks handled outside)
    if (prop_) {
        META_NS::Property<SCENE_NS::IBitmap::Ptr>(prop_)->SetValue(value);
    }
}
void BitmapProxy::SetValue(const SCENE_NS::IBitmap::Ptr v)
{
    Lock();
    if (value != v) {
        value = v;
        ScheduleUpdate();
    }
    Unlock();
}
napi_value BitmapProxy::Value()
{
    // should be executed in the javascript thread.
    Lock();
    SCENE_NS::IBitmap::Ptr res = value;
    Unlock();
    if (auto cached = FetchJsObj(res)) {
        return cached.ToNapiValue();
    }
    NapiApi::Env env(obj_.GetEnv());

    if (res) {
        NapiApi::Object parms(env);
        napi_value args[] = {
            scene_.GetValue(),
            parms.ToNapiValue()
        };
        MakeNativeObjectParam(env, res, BASE_NS::countof(args), args);

        auto size = res->Size()->GetValue();
        BASE_NS::string uri;
        if (auto m = interface_cast<META_NS::IMetadata>(res)) {
            if (auto p = m->GetProperty<BASE_NS::string>("Uri")) {
                uri = p->GetValue();
            }
        }
        auto name = interface_cast<META_NS::IObject>(res)->GetName();
        parms.Set("uri", uri);
        NapiApi::Object imageJS(GetJSConstructor(env, "Image"), BASE_NS::countof(args), args);
        return imageJS.ToNapiValue();
    }
    return env.GetNull();
}
void BitmapProxy::SetValue(NapiApi::FunctionContext<>& info)
{
    NapiApi::FunctionContext<NapiApi::Object> data { info };
    if (data) {
        NapiApi::Object val = data.Arg<0>();
        auto bitmap = GetNativeMeta<SCENE_NS::IBitmap>(val);
        Lock();
        value = bitmap;
        ScheduleUpdate();
        Unlock();
    }
}

BASE_NS::shared_ptr<PropertyProxy> PropertyToProxy(
    NapiApi::Object scene, NapiApi::Object obj, const META_NS::IProperty::Ptr& t)
{
    if (META_NS::IsCompatibleWith<float>(t)) {
        return BASE_NS::shared_ptr { new TypeProxy<float>(obj, t) };
    }
    if (META_NS::IsCompatibleWith<int32_t>(t)) {
        return BASE_NS::shared_ptr { new TypeProxy<int32_t>(obj, t) };
    }
    if (META_NS::IsCompatibleWith<uint32_t>(t)) {
        return BASE_NS::shared_ptr { new TypeProxy<uint32_t>(obj, t) };
    }
    if (META_NS::IsCompatibleWith<int64_t>(t)) {
        return BASE_NS::shared_ptr { new TypeProxy<int64_t>(obj, t) };
    }
    if (META_NS::IsCompatibleWith<uint64_t>(t)) {
        return BASE_NS::shared_ptr { new TypeProxy<uint64_t>(obj, t) };
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec2>(t)) {
        return BASE_NS::shared_ptr { new Vec2Proxy(obj.GetEnv(), t) };
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec3>(t)) {
        return BASE_NS::shared_ptr { new Vec3Proxy(obj.GetEnv(), t) };
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec4>(t)) {
        return BASE_NS::shared_ptr { new Vec4Proxy(obj.GetEnv(), t) };
    }
    if (META_NS::IsCompatibleWith<SCENE_NS::IBitmap::Ptr>(t)) {
        return BASE_NS::shared_ptr { new BitmapProxy(scene, obj, t) };
    }
    auto any = META_NS::GetInternalAny(t);
    LOG_F("Unsupported property type [%s]", any ? any->GetTypeIdString().c_str() : "<Unknown>");
    return nullptr;
}

static napi_value DummyFunc(napi_env e, napi_callback_info i)
{
    NapiApi::FunctionContext<> info(e, i);
    return info.GetUndefined();
};
static napi_value PropProxGet(napi_env e, napi_callback_info i)
{
    NapiApi::FunctionContext<> info(e, i);
    auto pc = static_cast<PropertyProxy*>(info.GetData());
    if (pc) {
        return pc->Value();
    }
    return info.GetUndefined();
};
static napi_value PropProxSet(napi_env e, napi_callback_info i)
{
    NapiApi::FunctionContext<>info (e, i);
    auto pc = static_cast<PropertyProxy*>(info.GetData());
    if (pc) {
        pc->SetValue(info);
    }
    return info.GetUndefined();
};
napi_property_descriptor CreateProxyDesc(const char* name, BASE_NS::shared_ptr<PropertyProxy> proxy)
{
    napi_property_descriptor desc { name, nullptr, nullptr, nullptr, nullptr, nullptr, napi_default_jsproperty,
        static_cast<void*>(proxy.get()) };
    if (proxy) {
        desc.getter = PropProxGet;
        desc.setter = PropProxSet;
    } else {
        desc.getter = desc.setter = DummyFunc;
    }
    return desc;
}
