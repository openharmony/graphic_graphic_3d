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

#ifndef PROPERTY_PROXY_H
#define PROPERTY_PROXY_H
#include <meta/interface/intf_event.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/property/property.h>
#include <meta/interface/property/property_events.h>
#include <napi_api.h>

#include <scene/interface/intf_bitmap.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>

class PropertyProxy {
public:
    explicit PropertyProxy(META_NS::IProperty::Ptr prop);
    virtual ~PropertyProxy();

    virtual void SetValue(NapiApi::FunctionContext<>& info) = 0;
    virtual napi_value Value() = 0;

    // removes hooks.(if removeProperty is true, the property will be fully removed from the object)
    virtual void Reset() = 0;

    virtual void SetExtra(const BASE_NS::shared_ptr<CORE_NS::IInterface>);
    virtual const BASE_NS::shared_ptr<CORE_NS::IInterface> GetExtra() const;
protected:
    void RemoveHandlers();

    virtual void UpdateLocalValues() = 0;
    virtual void UpdateRemoteValues() = 0;
    void UpdateLocal();
    int32_t UpdateRemote();
    void ScheduleUpdate();

    // synchronized get value.
    void SyncGet();

    void Lock();
    void Unlock();

    BASE_NS::weak_ptr<CORE_NS::IInterface> extra_;
    META_NS::IProperty::Ptr prop_;
    META_NS::IEvent::Token changeToken_ { 0 };
    META_NS::ITaskQueue::Token updateToken_ { nullptr };
    META_NS::IOnChanged::InterfaceTypePtr changeHandler_;
    META_NS::ITaskQueueTask::Ptr updateTask_;

private:
    CORE_NS::SpinLock lock_;
};

class ObjectPropertyProxy : public PropertyProxy {
public:
    explicit ObjectPropertyProxy(META_NS::IProperty::Ptr prop);
    ~ObjectPropertyProxy();
    void Reset() override;
    // copy data from jsobject
    void SetValue(NapiApi::FunctionContext<>& info) override;
    napi_value Value() override;

    virtual void SetValue(NapiApi::Object obj) = 0;

    virtual void SetMemberValue(NapiApi::FunctionContext<>& info, BASE_NS::string_view memb) = 0;
    virtual napi_value GetMemberValue(const NapiApi::Env info, BASE_NS::string_view memb) = 0;

    void Create(napi_env env, const BASE_NS::string jsName);
    void Hook(const BASE_NS::string member);
    void Destroy();
    NapiApi::StrongRef obj_;

    class MemberProxy {
    public:
        MemberProxy(ObjectPropertyProxy* p, BASE_NS::string m);
        ~MemberProxy();
        const BASE_NS::string_view Name() const;
        static napi_value Getter(napi_env e, napi_callback_info i);
        static napi_value Setter(napi_env e, napi_callback_info i);

    private:
        ObjectPropertyProxy* proxy_;
        BASE_NS::string memb_;
    };

    BASE_NS::vector<BASE_NS::unique_ptr<MemberProxy>> accessors_;
};
class BitmapProxy : public PropertyProxy {
public:
    BitmapProxy(NapiApi::Object scn, NapiApi::Object obj, META_NS::Property<SCENE_NS::IBitmap::Ptr> prop);
    ~BitmapProxy() override;

    void Reset() override;

protected:
    napi_value Value() override;
    void SetValue(NapiApi::FunctionContext<>& info) override;
    void SetValue(const SCENE_NS::IBitmap::Ptr v);
    void UpdateLocalValues() override;
    void UpdateRemoteValues() override;
    SCENE_NS::IBitmap::Ptr value {};
    NapiApi::WeakRef obj_;
    NapiApi::WeakRef scene_;
};

template<typename type>
class TypeProxy : public PropertyProxy {
public:
    TypeProxy(NapiApi::Object obj, META_NS::Property<type> prop);
    ~TypeProxy() override;

    void Reset() override;

protected:
    napi_value Value() override;
    void SetValue(NapiApi::FunctionContext<>& info) override;
    void SetValue(const type v);
    void UpdateLocalValues() override;
    void UpdateRemoteValues() override;
    type value {};
    NapiApi::WeakRef obj_;
};

template<typename type>
void TypeProxy<type>::Reset()
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
    Unlock();
}
template<typename type>
TypeProxy<type>::TypeProxy(NapiApi::Object obj, META_NS::Property<type> prop) : PropertyProxy(prop)
{
    obj_ = obj;
    SyncGet();
}

template<typename type>
TypeProxy<type>::~TypeProxy()
{
    // Unhook the objects.
    Reset();
}
template<typename type>
void TypeProxy<type>::UpdateLocalValues()
{
    // executed in engine thread (locks handled outside)
    if (prop_) {
        value = META_NS::Property<type>(prop_)->GetValue();
    }
}
template<typename type>
void TypeProxy<type>::UpdateRemoteValues()
{
    // executed in engine thread (locks handled outside)
    if (prop_) {
        META_NS::Property<type>(prop_)->SetValue(value);
    }
}

template<typename type>
void TypeProxy<type>::SetValue(const type v)
{
    Lock();
    if (value != v) {
        value = v;
        ScheduleUpdate();
    }
    Unlock();
}
template<typename type>
napi_value TypeProxy<type>::Value()
{
    Lock();
    type res = value;
    Unlock();
    return NapiApi::Env(obj_.GetEnv()).GetNumber((type)res);
}
template<typename type>
void TypeProxy<type>::SetValue(NapiApi::FunctionContext<>& info)
{
    NapiApi::FunctionContext<type> data { info };
    if (data) {
        type val = data.template Arg<0>();
        Lock();
        value = val;
        ScheduleUpdate();
        Unlock();
    }
}

BASE_NS::shared_ptr<PropertyProxy> PropertyToProxy(

    NapiApi::Object scene, NapiApi::Object obj, const META_NS::IProperty::Ptr& t);

napi_property_descriptor CreateProxyDesc(const char* name, BASE_NS::shared_ptr<PropertyProxy> proxy);

#endif
