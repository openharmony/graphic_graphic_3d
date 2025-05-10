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

#ifndef ARRAY_PROPERTY_PROXY_H
#define ARRAY_PROPERTY_PROXY_H
#include <meta/interface/intf_event.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/property/array_property.h>
#include <meta/interface/property/property.h>
#include <meta/interface/property/property_events.h>
#include <meta/api/util.h>
#include <napi_api.h>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>

class ArrayPropertyProxy {
public:
    explicit ArrayPropertyProxy(META_NS::IProperty::Ptr prop, META_NS::IArrayAny::IndexType index);
    virtual ~ArrayPropertyProxy();

    virtual void SetValue(NapiApi::FunctionContext<>& info) = 0;
    virtual napi_value Value() = 0;
    virtual void Reset() = 0;

protected:
    /// Returns a Property<Type> instance from underlying property
    template<typename Type>
    META_NS::ArrayProperty<Type> GetProperty() const
    {
        return META_NS::ArrayProperty<Type>(prop_);
    }
    META_NS::IArrayAny::IndexType GetIndex() const
    {
        return index_;
    }

    /// Returns the underlying property ptr
    META_NS::IProperty::Ptr GetPropertyPtr() const noexcept;

private:
    META_NS::IProperty::Ptr prop_;
    META_NS::IArrayAny::IndexType index_;
};

template<typename Type>
class TypeArrayProxy : public ArrayPropertyProxy {
public:
    TypeArrayProxy(NapiApi::Object obj, META_NS::IProperty::Ptr prop, META_NS::IArrayAny::IndexType index)
        : ArrayPropertyProxy(prop, index), obj_(obj)
    {}
    ~TypeArrayProxy() override
    {
        Reset();
    }
    void Reset() override
    {
        if (!obj_.IsEmpty()) {
            if (auto prop = GetPropertyPtr()) {
                obj_.GetObject().DeleteProperty(prop->GetName());
            }
        }
        obj_.Reset();
    }

protected:
    napi_value Value() override
    {
        if (auto arrayProp = GetProperty<Type>()) {
            auto value = arrayProp->GetValueAt(GetIndex());
            if constexpr (BASE_NS::is_same_v<Type, BASE_NS::string>) {
                return NapiApi::Env(obj_.GetEnv()).GetString(value);
            } else {
                return NapiApi::Env(obj_.GetEnv()).GetNumber(value);
            }
        }
        return NapiApi::Env(obj_.GetEnv()).GetUndefined();
    }
    void SetValue(NapiApi::FunctionContext<>& info) override
    {
        NapiApi::FunctionContext<Type> data { info };
        if (data) {
            Type value = data.template Arg<0>();
            auto arrayProp = GetProperty<Type>();
            if (auto arrayProp = GetProperty<Type>()) {
                arrayProp->SetValueAt(GetIndex(), value);
            }
        }
    }
    void SetValue(const Type& v)
    {
        if (auto arrayProp = GetProperty<Type>()) {
            arrayProp->SetValueAt(GetIndex(), v);
        }
    }

private:
    NapiApi::WeakRef obj_;
};

template<typename Type> 
BASE_NS::shared_ptr<ArrayPropertyProxy> PropertyToArrayProxy(NapiApi::Object scene, NapiApi::Object obj,
    META_NS::ArrayProperty<Type>& t, META_NS::IArrayAny::IndexType index)
{
    return BASE_NS::shared_ptr { new TypeArrayProxy<Type>(obj, t, index) };
}

napi_property_descriptor CreateArrayProxyDesc(const char* name, BASE_NS::shared_ptr<ArrayPropertyProxy> proxy);

#endif
