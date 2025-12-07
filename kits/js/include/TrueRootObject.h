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

#ifndef TRUE_ROOT_OBJECT_H
#define TRUE_ROOT_OBJECT_H

#ifdef __OHOS_PLATFORM__
#include "napi/native_api.h"
#else
#include <node_api.h>
#endif

#include <meta/interface/intf_object.h>

namespace NapiApi {
class Object;
struct JsFuncArgs;
} // namespace NapiApi

// Which type of a pointer to use for storing a reference to a native object.
enum class PtrType { WEAK, STRONG };

/**
 * @brief Provide access to a native IObject that backs a JS object. Optionally make the IObject reference strong so
 *        that the lifetime is controlled by JS. For example, scene is kept strongly and objects owned by scene weakly.
 */
class TrueRootObject {
public:
    static constexpr uint32_t ID = 0xCAFED00D;

    TrueRootObject() = delete;
    TrueRootObject(napi_env env, napi_callback_info info);

    virtual void* GetInstanceImpl(uint32_t id);
    virtual void DisposeNative(void*) = 0;

    virtual bool IsStrong() const;
    virtual META_NS::IObject::Ptr GetNativeObject() const;
    template<typename Type>
    auto GetNativeObject() const
    {
        return interface_pointer_cast<Type>(GetNativeObject());
    }
    virtual void SetNativeObject(META_NS::IObject::Ptr real, PtrType ptrType);
    virtual void UnsetNativeObject();

    virtual void Finalize(napi_env env);

    static void InjectNativeObject(
        napi_env env, const META_NS::IObject::Ptr& obj, PtrType ptrType, NapiApi::JsFuncArgs& args);

protected:
    virtual ~TrueRootObject() = default;
    static void destroy(TrueRootObject* object);

private:
    // Helper class to more safely pass the native object as a javascript constructor argument.
    class InjectedNativeObject {
    public:
        InjectedNativeObject(const META_NS::IObject::Ptr& obj);
        ~InjectedNativeObject() = default;
        META_NS::IObject::Ptr Reset();

    private:
        META_NS::IObject::Ptr object_;
    };
    void ExtractNativeObject(NapiApi::Object& resourceParam);
    META_NS::IObject::Ptr obj_;
    META_NS::IObject::WeakPtr objW_;
};

#endif
