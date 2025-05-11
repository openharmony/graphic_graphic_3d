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

#include "ArrayPropertyProxy.h"

#include <meta/api/make_callback.h>
#include <meta/api/util.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <napi_api.h>

#include "BaseObjectJS.h"

ArrayPropertyProxy::~ArrayPropertyProxy()
{
    // should be a no-op.
}

ArrayPropertyProxy::ArrayPropertyProxy(META_NS::IProperty::Ptr prop, META_NS::IArrayAny::IndexType index) 
    : prop_(prop), index_(index)
{
    assert(prop_);
}

META_NS::IProperty::Ptr ArrayPropertyProxy::GetPropertyPtr() const noexcept
{
    return prop_;
}

static napi_value DummyFunc(napi_env e, napi_callback_info i)
{
    NapiApi::FunctionContext<> info(e, i);
    return info.GetUndefined();
};

static napi_value PropProxGet(napi_env e, napi_callback_info i)
{
    NapiApi::FunctionContext<> info(e, i);
    auto pc = static_cast<ArrayPropertyProxy*>(info.GetData());
    if (pc) {
        return pc->Value();
    }
    return info.GetUndefined();
};

static napi_value PropProxSet(napi_env e, napi_callback_info i)
{
    NapiApi::FunctionContext<> info(e, i);
    auto pc = static_cast<ArrayPropertyProxy*>(info.GetData());
    if (pc) {
        pc->SetValue(info);
    }
    return info.GetUndefined();
};

napi_property_descriptor CreateArrayProxyDesc(const char* name, BASE_NS::shared_ptr<ArrayPropertyProxy> proxy)
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