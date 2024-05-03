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
#include <meta/interface/intf_task_queue_registry.h>
#include <napi_api.h>
#include "BaseObjectJS.h"

class PropCtx {
    PropertyProxy* proxy_;
    BASE_NS::string memb_;

public:
    PropCtx(PropertyProxy* p, BASE_NS::string m);
    ~PropCtx();
    napi_value GetValue(NapiApi::FunctionContext<>& info);
    void SetValue(NapiApi::FunctionContext<>& info);
};

PropCtx::PropCtx(PropertyProxy* p, BASE_NS::string m)
{
    proxy_ = p;
    memb_ = m;
}
PropCtx::~PropCtx()
{
}
napi_value PropCtx::GetValue(NapiApi::FunctionContext<>& info)
{
    return proxy_->GetValue(info, memb_);
}
void PropCtx::SetValue(NapiApi::FunctionContext<>& info)
{
    proxy_->SetValue(info, memb_);
}

void PropertyProxy::UpdateLocal()
{
    // should execute in engine thread.
    duh.Lock();
    UpdateLocalValues();
    duh.Unlock();
}
int32_t PropertyProxy::UpdateRemote()
{
    // executed in engine thread (ie. happens between frames)
    duh.Lock();
    // make sure the handler is not called..
    prop_->OnChanged()->RemoveHandler(changeToken_);

    UpdateRemoteValues();

    // add it back.
    changeToken_ = prop_->OnChanged()->AddHandler(changeHandler_);
    updateToken_ = nullptr;
    duh.Unlock();

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
    changeHandler_ = META_NS::MakeCallback<META_NS::IOnChanged>(this, &PropertyProxy::UpdateLocal);
    changeToken_ = prop_->OnChanged()->AddHandler(changeHandler_);
    updateTask_ = META_NS::MakeCallback<META_NS::ITaskQueueTask>(this, &PropertyProxy::UpdateRemote);
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
PropertyProxy::~PropertyProxy()
{
    if (prop_) {
        prop_->OnChanged()->RemoveHandler(changeToken_);
    }

    for (auto t : accessors) {
        delete t;
    }
}

void PropertyProxy::Create(napi_env env, const BASE_NS::string jsName)
{
    NapiApi::MyInstanceState* mis;
    napi_get_instance_data(env, (void**)&mis);
    auto ref = NapiApi::Object(env, mis->getRef());
    auto cl = ref.Get(jsName.c_str());
    if (cl) {
        napi_value value;
        napi_new_instance(env, cl, 0, nullptr, &value);
        obj = NapiApi::StrongRef { NapiApi::Object(env, value) };
    } else {
        CORE_LOG_F("Could not create property object for %s", jsName.c_str());
    }
}

void PropertyProxy::Hook(const BASE_NS::string member)
{
    auto* accessor = new PropCtx(this, member.c_str());
    accessors.push_back(accessor);
    auto value = obj.GetValue();

    napi_property_descriptor desc { member.c_str(), nullptr, nullptr,

        [](napi_env e, napi_callback_info i) -> napi_value {
            NapiApi::FunctionContext<> info(e, i);
            auto pc = (PropCtx*)info.GetData();
            return pc->GetValue(info);
        },
        [](napi_env e, napi_callback_info i) -> napi_value {
            NapiApi::FunctionContext<> info(e, i);
            auto pc = (PropCtx*)info.GetData();
            pc->SetValue(info);
            return {};
        },
        nullptr, napi_default_jsproperty, (void*)accessor };
    napi_status status = napi_define_properties(obj.GetEnv(), value, 1, &desc);
}
PropertyProxy::operator napi_value()
{
    return obj.GetValue();
}

NapiApi::Value<NapiApi::Object> PropertyProxy::Value()
{
    return { obj.GetEnv(), obj.GetValue() };
}

const META_NS::IProperty::Ptr& PropertyProxy::GetProperty() const
{
    return prop_;
}
