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
#include <meta/api/util.h>
#include <napi_api.h>

#include <scene/interface/intf_image.h>
#include <scene/interface/intf_shader.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>

class PropertyProxy {
public:
    explicit PropertyProxy(META_NS::IProperty::Ptr prop);
    virtual ~PropertyProxy();

    virtual void SetValue(NapiApi::FunctionContext<>& info) = 0;
    virtual void ResetValue();
    virtual napi_value Value() = 0;
    virtual void Reset() = 0;

    virtual void SetExtra(const BASE_NS::shared_ptr<CORE_NS::IInterface>);
    virtual const BASE_NS::shared_ptr<CORE_NS::IInterface> GetExtra() const noexcept;
protected:
    /// Returns a Property<Type> instance from underlying property
    template<typename Type>
    META_NS::Property<Type> GetProperty() const
    {
        return META_NS::Property<Type>(prop_.lock());
    }
    /// Returns the underlying property ptr
    META_NS::IProperty::Ptr GetPropertyPtr() const noexcept;
private:
    BASE_NS::weak_ptr<CORE_NS::IInterface> extra_;
    META_NS::IProperty::WeakPtr prop_;
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

class EntityProxy : public PropertyProxy {
public:
    EntityProxy(NapiApi::Object scn, NapiApi::Object obj, META_NS::Property<CORE_NS::Entity> prop);
    ~EntityProxy() override;
    void Reset() override;

protected:
    napi_value Value() override;
    void SetValue(NapiApi::FunctionContext<>& info) override;
    void SetValue(const CORE_NS::Entity v);
    NapiApi::StrongRef obj_;
    NapiApi::WeakObjectRef scene_;
    CORE_NS::EntityReference entityReference_;
    NapiApi::StrongRef shader_;
};

class ImageProxy : public PropertyProxy {
public:
    ImageProxy(NapiApi::Object scn, NapiApi::Object obj, META_NS::Property<SCENE_NS::IImage::Ptr> prop);
    ~ImageProxy() override;
    void Reset() override;

protected:
    napi_value Value() override;
    void SetValue(NapiApi::FunctionContext<>& info) override;
    void SetValue(const SCENE_NS::IImage::Ptr& v);
    NapiApi::StrongRef obj_;
    NapiApi::WeakObjectRef scene_;
};

template<typename Type>
class TypeProxy : public PropertyProxy {
public:
    TypeProxy(NapiApi::Object obj, META_NS::Property<Type> prop) : PropertyProxy(prop), obj_(obj) {}
    ~TypeProxy() override
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
        auto value = META_NS::GetValue(GetProperty<Type>());
        if constexpr (BASE_NS::is_same_v<Type, BASE_NS::string>) {
            return NapiApi::Env(obj_.GetEnv()).GetString(value);
        } else if constexpr (BASE_NS::is_same_v<Type, bool>) {
            return NapiApi::Env(obj_.GetEnv()).GetBoolean(value);
        } else {
            return NapiApi::Env(obj_.GetEnv()).GetNumber(value);
        }
    }
    void SetValue(NapiApi::FunctionContext<>& info) override
    {
        NapiApi::FunctionContext<Type> data { info };
        if (data) {
            Type value = data.template Arg<0>();
            META_NS::SetValue(GetProperty<Type>(), value);
        }
    }
    void SetValue(const Type& v)
    {
        META_NS::SetValue(GetProperty<Type>(), v);
    }

private:
    NapiApi::StrongRef obj_;
};


BASE_NS::shared_ptr<PropertyProxy> PropertyToProxy(

    NapiApi::Object scene, NapiApi::Object obj, const META_NS::IProperty::Ptr& t);

napi_property_descriptor CreateProxyDesc(const char* name, BASE_NS::shared_ptr<PropertyProxy> proxy);

#endif
