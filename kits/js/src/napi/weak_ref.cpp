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

#include "weak_ref.h"
#include "JsObjectCache.h"

namespace NapiApi {

WeakRef::WeakRef(const NapiApi::WeakRef& ref)
{
    if (!ref.IsEmpty()) {
        napi_status stat;
        env_ = ref.env_;
        stat = napi_create_reference(env_, ref.GetValue(), 0, &ref_);
    }
}

WeakRef::WeakRef(NapiApi::WeakRef&& ref) noexcept
{
    env_ = ref.env_;
    ref_ = ref.ref_;
    ref.env_ = nullptr;
    ref.ref_ = nullptr;
}

WeakRef::WeakRef(NapiApi::Object obj)
{
    env_ = obj.GetEnv();
    napi_create_reference(env_, obj.ToNapiValue(), 0, &ref_);
}

WeakRef::WeakRef(napi_env env, napi_value obj)
{
    env_ = env;
    napi_create_reference(env_, obj, 0, &ref_);
}

NapiApi::WeakRef& WeakRef::operator=(const NapiApi::WeakRef& ref)
{
    if (&ref != this) {
        Reset();
        if (!ref.IsEmpty()) {
            napi_status stat;
            // unh just create a new one..
            env_ = ref.env_;
            stat = napi_create_reference(env_, ref.GetValue(), 0, &ref_);
        }
    }
    return *this;
}

NapiApi::WeakRef& WeakRef::operator=(NapiApi::WeakRef&& ref) noexcept
{
    if (&ref != this) {
        Reset();
        env_ = ref.env_;
        ref_ = ref.ref_;
        ref.env_ = nullptr;
        ref.ref_ = nullptr;
    }
    return *this;
}

WeakRef::~WeakRef()
{
    Reset();
}

void WeakRef::Reset()
{
    if (env_ && ref_) {
#ifdef __OHOS_PLATFORM__
        uint32_t result{};
        napi_reference_ref(env_, ref_, &result);
#endif
        napi_delete_reference(env_, ref_);
    }
    env_ = nullptr;
    ref_ = nullptr;
}

bool WeakRef::IsEmpty() const
{
    if (env_ && ref_) {
        // possibly actually check the ref?
        return false;
    }
    return true;
}

napi_env WeakRef::GetEnv() const
{
    return env_;
}

napi_value WeakRef::GetValue() const
{
    if (env_ && ref_) {
        napi_value value;
        napi_get_reference_value(env_, ref_, &value);
        return value;
    }
    return {};
}

NapiApi::Object WeakRef::GetObject() const
{
    if (env_ && ref_) {
        napi_value value;
        napi_get_reference_value(env_, ref_, &value);
        return NapiApi::Object(env_, value);
    }
    return {};
}

NapiApi::Object WeakObjectRef::GetNapiObject(BASE_NS::string_view name) const
{
    if (auto obj = GetObject<META_NS::IObject>()) {
        return FetchJsObj(obj, name);
    }
    return napiObject_.GetObject();
}

} // namespace NapiApi
