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

#ifndef JS_OBJECT_CACHE_H
#define JS_OBJECT_CACHE_H

#include <meta/interface/intf_object.h>
#include <napi_api.h>

/**
 * @brief Fetch a JS object from the cache if it's there.
 * @param obj The native object for which to fetch the JS wrapper.
 * @param name An identifier to separate multiple JS objects wrapping the same native.
 * @return The JS object wrapping the native one. If none found, a JS object with a JS null as value.
 */
NapiApi::Object FetchJsObj(const META_NS::IObject::Ptr& obj, BASE_NS::string_view name = "_JSW");

template<typename t>
NapiApi::Object FetchJsObj(const t& obj, BASE_NS::string_view name = "_JSW")
{
    return FetchJsObj(interface_pointer_cast<META_NS::IObject>(obj));
}

/**
 * @brief Store a reference of the JS object in the cache.
 * @param obj The native object wrapped by jsObj.
 * @param jsObj The JS object that wraps the native obj.
 * @param name An identifier to separate multiple JS objects wrapping the same native.
 * @return The stored JS object. If an error occurred, a JS object with a JS null as value.
 */
NapiApi::Object StoreJsObj(
    const META_NS::IObject::Ptr& obj, const NapiApi::Object& jsObj, BASE_NS::string_view name = "_JSW");

#endif
