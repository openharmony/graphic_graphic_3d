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

#include "TrueRootObject.h"

#include <napi_api.h>

#include "BaseObjectJS.h"

TrueRootObject::TrueRootObject(napi_env env, napi_callback_info info)
{
    if (auto getOnlyFirstArg = NapiApi::FunctionContext<NapiApi::Object>(env, info, NapiApi::ArgCount::PARTIAL)) {
        auto firstArg = NapiApi::Object { getOnlyFirstArg.Arg<0>() };
        ExtractNativeObject(firstArg);
    }
}

bool TrueRootObject::IsStrong() const
{
    return obj_ != nullptr;
}

META_NS::IObject::Ptr TrueRootObject::GetNativeObject() const
{
    if (obj_) {
        return obj_;
    }
    return objW_.lock();
}

void TrueRootObject::SetNativeObject(META_NS::IObject::Ptr real, PtrType ptrType)
{
    if (ptrType == PtrType::STRONG) {
        obj_ = real;
        objW_ = nullptr;
    } else {
        obj_ = nullptr;
        objW_ = real;
    }
}

void TrueRootObject::UnsetNativeObject()
{
    obj_ = nullptr;
    objW_ = nullptr;
}

void TrueRootObject::Finalize(napi_env env)
{
    // Synchronously destroy the lume object in engine thread.. (only for strong refs.)
    if (obj_) {
        ExecSyncTask([obj = BASE_NS::move(obj_)]() { return META_NS::IAny::Ptr {}; });
    }
    // and reset the weak ref too. (which may be null anyway)
    objW_.reset();
}

void TrueRootObject::destroy(TrueRootObject* object)
{
    delete object;
}

void TrueRootObject::ExtractNativeObject(NapiApi::Object& resourceParam)
{
    const auto env = resourceParam.GetEnv();
    const auto nativeObjStash = resourceParam.Get("NativeObject");
    auto ptrTypeJs = resourceParam.Get<uint32_t>("NativeObjectPtrType");
    if (!nativeObjStash || !ptrTypeJs.IsValid()) {
        // Okay, not created via injecting a native object.
        return;
    }

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    resourceParam.Set("NativeObject", undefined);
    resourceParam.Set("NativeObjectPtrType", undefined);

    META_NS::IObject::Ptr metaPtr;
    InjectedNativeObject* ptr { nullptr };
    napi_get_value_external(env, nativeObjStash, (void**)&ptr);
    if (ptr == nullptr || !(metaPtr = ptr->Reset())) {
        LOG_E("Unable to wrap a native object with TrueRootObject: Native object was null");
        assert(false);
        return;
    }
    SetNativeObject(metaPtr, static_cast<PtrType>(ptrTypeJs.valueOrDefault()));
}

// TrueRootObject stores the pointer to native. Pass the pointer and strength info over there.
void TrueRootObject::InjectNativeObject(
    napi_env env, const META_NS::IObject::Ptr& obj, PtrType ptrType, NapiApi::JsFuncArgs& args)
{
    if (args.argc == 0) {
        LOG_E("Empty arg list given for native object injection");
        assert(false);
        return;
    }
    const auto tmpPtr = new TrueRootObject::InjectedNativeObject(obj);
    const auto deleteTmpPtr = [](napi_env env, void* obj, void* finalize_info) {
        delete (TrueRootObject::InjectedNativeObject*)obj;
    };
    napi_value nativeObjStash;
    napi_create_external(env, (void*)tmpPtr, deleteTmpPtr, nullptr, &nativeObjStash);

    NapiApi::Object resourceParam(env, args.argv[0]);
    resourceParam.Set("NativeObject", nativeObjStash);
    resourceParam.Set("NativeObjectPtrType", NapiApi::Env { env }.GetNumber(static_cast<uint32_t>(ptrType)));
}

TrueRootObject::InjectedNativeObject::InjectedNativeObject(const META_NS::IObject::Ptr& obj)
{
    object_ = obj;
}
META_NS::IObject::Ptr TrueRootObject::InjectedNativeObject::Reset()
{
    return BASE_NS::move(object_);
}
