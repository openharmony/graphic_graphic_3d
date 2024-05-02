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

#ifndef OHOS_RENDER_3D_PROPERTY_PROXY_H
#define OHOS_RENDER_3D_PROPERTY_PROXY_H
#include <meta/interface/intf_event.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/property/intf_property.h>
#include <meta/interface/property/property_events.h>
#include <napi_api.h>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>

class PropertyProxy {
    BASE_NS::vector<class PropCtx*> accessors;

protected:
    CORE_NS::SpinLock duh;
    META_NS::IProperty::Ptr prop_;
    NapiApi::StrongRef obj;
    void Create(napi_env env, const BASE_NS::string jsName);
    void Hook(const BASE_NS::string member);
    META_NS::IOnChanged::InterfaceTypePtr changeHandler_;
    META_NS::IEvent::Token changeToken_;
    META_NS::ITaskQueueTask::Ptr updateTask_;
    META_NS::ITaskQueue::Token updateToken_ { nullptr };
    virtual void UpdateLocalValues() = 0;
    virtual void UpdateRemoteValues() = 0;
    void UpdateLocal();
    int32_t UpdateRemote();
    void ScheduleUpdate();

    // synchronized get value.
    void SyncGet();
    BASE_NS::string name_;
public:
    operator napi_value();
    explicit PropertyProxy(META_NS::IProperty::Ptr prop);
    virtual ~PropertyProxy();

    // copy data from jsobject
    virtual bool SetValue(NapiApi::Object data) = 0;
    NapiApi::Value<NapiApi::Object> Value();
    virtual void SetValue(NapiApi::FunctionContext<>& info, BASE_NS::string_view memb) = 0;
    virtual napi_value GetValue(NapiApi::FunctionContext<>& info, BASE_NS::string_view memb) = 0;
    const META_NS::IProperty::Ptr& GetProperty() const;
};
#endif // OHOS_RENDER_3D_PROPERTY_PROXY_H
