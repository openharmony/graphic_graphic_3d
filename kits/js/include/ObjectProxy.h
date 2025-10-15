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

#ifndef OBJECT_PROXY_H
#define OBJECT_PROXY_H

#include <functional>

#include <base/containers/string.h>
#include <base/containers/vector.h>

#include "PropertyProxy.h"
#include "napi/napi_api.h"

/**
 * @brief Proxy members of a JS object to certain native properties.
 * @note This is a partial reimplementation of ObjectPropertyProxy and uses TypeProxies in an unoptimal way.
 */
class ObjectProxy {
public:
    /// Convert values between JS API scale and native scale. Return undefined to skip setting native value.
    using ScalingFunc = std::function<napi_value(napi_env, napi_value)>;
    /// Map a JS property with the given name to the native target property. Scaling functions are optional.
    struct PropertyMapping {
        BASE_NS::string jsName;
        META_NS::IProperty::Ptr nativeProp;
        ScalingFunc scaleToNative;
        ScalingFunc scaleToJs;
    };

    /// Return the held object that has the given proxied properties.
    NapiApi::Object GetObject() const;
    /// Return the held object that has the given proxied properties.
    operator NapiApi::Object() const;

    /**
     * @brief Define the properties that will be proxied by the held object and create that object.
     * @param env The napi environment.
     * @param mapping Mapping pairs between JS properties and native properties.
     * @param scene Optionally provide the scene required by some types. @see PropertyToProxy
     */
    void Init(napi_env env, const BASE_NS::vector<PropertyMapping>& mapping, NapiApi::Object scene = {});

    /// Unbind the old, bind the replacement and apply values from it.
    void ReplaceObject(NapiApi::Object& replacement);

    /// Unbind the held object from the native values. Clear all stored data.
    void Reset();

private:
    struct Mapping {
        BASE_NS::string jsName;
        BASE_NS::shared_ptr<PropertyProxy> proxy;
        ScalingFunc scaleToNative;
        ScalingFunc scaleToJs;
    };

    // Unbind JS object properties from native properties. Keep values unchanged.
    void UnbindObject();
    // Bind JS values to native props and set values from JS to native.
    void BindObject();

    static napi_property_descriptor CreateProxyDesc(const char* name, Mapping* mapping);
    static napi_value PropProxyGet(napi_env e, napi_callback_info i);
    static napi_value PropProxySet(napi_env e, napi_callback_info i);

    BASE_NS::vector<Mapping> mappings_;
    NapiApi::StrongRef obj_;
};
#endif