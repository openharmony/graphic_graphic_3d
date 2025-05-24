/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef NAPI_API_OBJECT_H
#define NAPI_API_OBJECT_H

#ifdef __OHOS_PLATFORM__
#include "napi/native_api.h"
#else
#include <node_api.h>
#endif

#include <meta/interface/intf_object.h>

#include <base/containers/string.h>

#include "TrueRootObject.h"
#include "env.h"
#include "value.h"

class TrueRootObject;
struct JsFuncArgs;

namespace NapiApi {

class Object {
public:
    Object() = default;
    explicit Object(napi_env env);
    Object(napi_env env, napi_value v);
    Object(napi_env env, BASE_NS::string_view className, const JsFuncArgs& args);

    napi_value ToNapiValue() const;
    napi_env GetEnv() const;

    TrueRootObject* GetRoot() const;

    template<class T>
    T* GetJsWrapper() const
    {
        if (const auto tro = GetRoot()) {
            return static_cast<T*>(tro->GetInstanceImpl(T::ID));
        }
        return nullptr;
    }

    META_NS::IObject::Ptr GetNative() const;
    template<typename T>
    typename T::Ptr GetNative() const
    {
        return interface_pointer_cast<T>(GetNative());
    }

    template<typename t>
    NapiApi::Value<t> Get(const BASE_NS::string_view name)
    {
        return NapiApi::Value<t>(env_, Get(name));
    }

    napi_value Get(const BASE_NS::string_view name);

    template<typename type>
    void Set(const BASE_NS::string_view name, NapiApi::Value<type> value)
    {
        Set(name, value.ToNapiValue());
    }

    void Set(const BASE_NS::string_view name, napi_value value);
    void Set(const BASE_NS::string_view name, const Object& value);
    void Set(const BASE_NS::string_view name, const BASE_NS::string_view v);

    operator bool() const;
    bool Has(const BASE_NS::string_view name);
    bool IsNull() const;
    bool IsNull(const BASE_NS::string_view name);
    bool IsDefined() const;
    bool IsUndefined(const BASE_NS::string_view name);
    bool IsUndefinedOrNull(const BASE_NS::string_view name);

    // Note: this comparison is about NapiApi::Object instances, not about the underlying JS objects.
    bool IsSame(const Object& other) const;
    // This API represents the invocation of the Strict Equality algorithm as defined in Section 7.2.14 of the
    // ECMAScript Language Specification.
    bool StrictEqual(const Object& other) const;

    void AddProperty(const napi_property_descriptor desc);
    bool DeleteProperty(const BASE_NS::string_view name);

protected:
    struct NameAndType {
        bool success { false };
        napi_value res { nullptr };
        napi_valuetype jstype { napi_undefined };
    };
    NameAndType GetNamedPropertyAndType(const BASE_NS::string_view name);
    napi_value MakeTempString(const BASE_NS::string_view v);

private:
    template<class T>
    T* GetInterface() const
    {
        if (!env_ || !object_) {
            return nullptr;
        }
        T* me = nullptr;
        napi_unwrap(env_, object_, (void**)&me);
        return me;
    }

    napi_valuetype jstype = napi_undefined;
    Env env_ { nullptr };
    napi_value object_ { nullptr };
};

} // namespace NapiApi

#endif