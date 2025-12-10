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

#include "strong_ref.h"

namespace NapiApi {

StrongRef::StrongRef(napi_env env, napi_value obj)
{
    if (env && obj) {
        env_ = env;
        napi_create_reference(env_, obj, 1, &ref_);
    }
}

StrongRef::StrongRef(napi_env env, napi_ref ref)
{
    if (env && ref) {
        env_ = env;
        ref_ = ref;
        Ref();
    }
}

StrongRef::StrongRef(const NapiApi::Object& obj)
{
    if (obj) {
        env_ = obj.GetEnv();
        napi_create_reference(env_, obj.ToNapiValue(), 1, &ref_);
    }
}

StrongRef::StrongRef(const NapiApi::Value<NapiApi::Object> objValue)
    : StrongRef(objValue.GetEnv(), objValue.ToNapiValue())
{}

StrongRef::StrongRef(const NapiApi::StrongRef& ref)
{
    if (!ref.IsEmpty()) {
        env_ = ref.env_;
        ref_ = ref.ref_;
        Ref();
    }
}

StrongRef::StrongRef(NapiApi::StrongRef&& ref) noexcept
{
    if (!ref.IsEmpty()) {
        env_ = ref.env_;
        ref_ = ref.ref_;
        ref.env_ = nullptr;
        ref.ref_ = nullptr;
    }
}

StrongRef& StrongRef::operator=(const NapiApi::StrongRef& ref)
{
    if (&ref != this) {
        Reset();
        if (!ref.IsEmpty()) {
            env_ = ref.env_;
            ref_ = ref.ref_;
            Ref();
        }
    }
    return *this;
}

StrongRef& StrongRef::operator=(NapiApi::StrongRef&& ref) noexcept
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

StrongRef::~StrongRef()
{
    if (!IsEmpty()) {
        Reset();
    }
}

void StrongRef::Reset()
{
    if (IsEmpty()) {
        return;
    }
    uint32_t cnt;
    napi_status stat = napi_reference_unref(env_, ref_, &cnt);
    if (stat != napi_ok) {
        return;
    }
    if (cnt == 0) {
        // that was the last reference.
#ifdef __OHOS_PLATFORM__
        napi_reference_ref(env_, ref_, &cnt);
#endif
        napi_delete_reference(env_, ref_);
    }
    env_ = nullptr;
    ref_ = nullptr;
}

bool StrongRef::IsEmpty() const
{
    return !(env_ && ref_);
}

napi_env StrongRef::GetEnv() const
{
    return env_;
}

napi_value StrongRef::GetValue() const
{
    if (IsEmpty()) {
        return {};
    }
    napi_value value;
    napi_get_reference_value(env_, ref_, &value);
    return value;
}

NapiApi::Object StrongRef::GetObject() const
{
    return NapiApi::Object(env_, (napi_value)GetValue());
}

uint32_t StrongRef::GetRefCount() const
{
    // this is racy, but for debug use "fine"
    if (!IsEmpty()) {
        uint32_t cnt;
        napi_reference_ref(env_, ref_, &cnt);
        napi_reference_unref(env_, ref_, &cnt);
        return cnt;
    }
    return 0;
}

void StrongRef::Ref()
{
    uint32_t cnt;
    napi_status stat = napi_reference_ref(env_, ref_, &cnt);
}

} // namespace NapiApi
