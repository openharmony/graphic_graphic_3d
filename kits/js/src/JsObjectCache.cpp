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

#include "JsObjectCache.h"

#include <meta/api/make_callback.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/property/construct_property.h>

#include <base/util/uid.h>
#include <core/plugin/intf_interface.h>

#include "TrueRootObject.h"

#if !defined(JSW_USE_TSF) || (!JSW_USE_TSF)
#include "nodejstaskqueue.h"
#endif

/**
 * @brief Store a reference to a JS object in the metaobject.
 */
class JSWrapperState : public CORE_NS::IInterface {
public:
    static constexpr BASE_NS::Uid UID { "2ef39765-91f2-46c4-b85f-7cad40dd3bcd" };
    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;
    void Ref() override;
    void Unref() override;
    JSWrapperState(NapiApi::Object obj, BASE_NS::string_view name);
    ~JSWrapperState();
    NapiApi::Object GetObject();

private:
    static void Call(napi_env env, napi_value js_callback, void* context, void* inData);
    static void Final(napi_env env, void* finalize_data, void* finalize_hint);
    BASE_NS::string name_;
    volatile int32_t count_ { 0 };
    napi_env env_ { nullptr };
    napi_ref ref_ { nullptr };
#if JSW_USE_TSF
    napi_threadsafe_function termfun { nullptr };
    struct fun_parm {
        napi_ref ref { nullptr };
        napi_threadsafe_function termfun { nullptr };
    };
#else
    static bool DeleteReference(napi_ref);
#endif
};

const CORE_NS::IInterface* JSWrapperState::GetInterface(const BASE_NS::Uid& uid) const
{
    if (uid == CORE_NS::IInterface::UID) {
        return this;
    }
    if (uid == JSWrapperState::UID) {
        return this;
    }
    return nullptr;
}

CORE_NS::IInterface* JSWrapperState::GetInterface(const BASE_NS::Uid& uid)
{
    if (uid == CORE_NS::IInterface::UID) {
        return this;
    }
    if (uid == JSWrapperState::UID) {
        return this;
    }
    return nullptr;
}

void JSWrapperState::Ref()
{
    BASE_NS::AtomicIncrement(&count_);
}

void JSWrapperState::Unref()
{
    if (BASE_NS::AtomicDecrement(&count_) == 0) {
        delete this;
    }
}

JSWrapperState::JSWrapperState(NapiApi::Object obj, BASE_NS::string_view name) : name_(name), env_(obj.GetEnv())
{
    napi_status status;
    // Create a WEAK reference to the object
    status = napi_create_reference(env_, obj.ToNapiValue(), 0, &ref_);
#if JSW_USE_TSF
    // Create a thread safe function to release the js reference, as the destructor MAY be called from engine thread.
    napi_value jname = nullptr;
    status = napi_create_string_latin1(env_, "JSW", NAPI_AUTO_LENGTH, &jname);
    status = napi_create_threadsafe_function(
        env_, nullptr, nullptr, jname, 0, 1, nullptr, &JSWrapperState::Final, nullptr, &JSWrapperState::Call, &termfun);
#else
    // make sure nodejstaskqueue wont die before we are ready..
    auto& tr = META_NS::GetTaskQueueRegistry();
    auto jsQ = tr.GetTaskQueue(JS_THREAD_DEP);
    auto queueRefCount = interface_cast<INodeJSTaskQueue>(jsQ);
    queueRefCount->Acquire();
#endif
}

#if JSW_USE_TSF
void JSWrapperState::Call(napi_env env, napi_value js_callback, void* context, void* inData)
{
    napi_status status;
    fun_parm* ref = (fun_parm*)inData;
    status = napi_delete_reference(env, ref->ref);
    status = napi_release_threadsafe_function(ref->termfun, napi_threadsafe_function_release_mode::napi_tsfn_release);
    delete ref;
}
void JSWrapperState::Final(napi_env env, void* finalize_data, void* context) {};
#else

bool JSWrapperState::DeleteReference(napi_ref ref)
{
    auto curQueue = META_NS::GetTaskQueueRegistry().GetCurrentTaskQueue();
    if (auto p = interface_cast<INodeJSTaskQueue>(curQueue)) {
        napi_env env = p->GetNapiEnv();
        napi_status status = napi_delete_reference(env, ref);
        p->Release(); // we don't need it anymore so release it.
    }
    return false;
}
#endif

JSWrapperState::~JSWrapperState()
{
#if JSW_USE_TSF
    napi_status status;
    // trigger the threadsafe function, so that the javascript "weak reference" will be destroyed.
    status = napi_call_threadsafe_function(
        termfun, new fun_parm { ref_, termfun }, napi_threadsafe_function_call_mode::napi_tsfn_blocking);
#else
    auto tq = META_NS::GetTaskQueueRegistry().GetTaskQueue(JS_THREAD_DEP);
    auto curQueue = META_NS::GetTaskQueueRegistry().GetCurrentTaskQueue();
    if (curQueue == tq) {
        // in jsqueue, can call directly
        // this is optional as the queue will handle it also "somewhat" directly in this case
        DeleteReference(ref_);
        return;
    }
    if (auto p = interface_cast<INodeJSTaskQueue>(tq)) {
        auto tok = tq->AddTask(META_NS::MakeCallback<META_NS::ITaskQueueTask>(DeleteReference, ref_));
        if (!tok) {
            LOG_F("Cleanup task add failed!");
        }
    } else {
        LOG_F("NODE JS TASK QUEUE IS NOT INITIALIZED OR INVALID TYPE");
    }

#endif
}

NapiApi::Object JSWrapperState::GetObject()
{
    napi_status status;
    napi_value value;
    status = napi_get_reference_value(env_, ref_, &value);
    return { env_, value };
}

NapiApi::Object FetchJsObj(const META_NS::IObject::Ptr& obj, BASE_NS::string_view name)
{
    using namespace META_NS;

    // access hidden property.
    if (auto AppMeta = interface_pointer_cast<IMetadata>(obj)) {
        if (auto wrapper = AppMeta->GetProperty<SharedPtrIInterface>(name, MetadataQuery::EXISTING)) {
            // The native object already contains a JS object.
            return interface_cast<JSWrapperState>(wrapper->GetValue())->GetObject();
        }
    }
    return {};
}

NapiApi::Object StoreJsObj(const META_NS::IObject::Ptr& obj, const NapiApi::Object& jsobj, BASE_NS::string_view name)
{
    using namespace META_NS;
    if (auto AppMeta = interface_pointer_cast<IMetadata>(obj)) {
        // Add a reference to the JS object to the native object.
        const auto& obr = GetObjectRegistry();
        auto wrapper = AppMeta->GetProperty<SharedPtrIInterface>(name, META_NS::MetadataQuery::EXISTING);
        // check if the property exists.
        if (wrapper) {
            // .. does the wrapper exist? (ie. reference from native to js)
            if (auto val = interface_cast<JSWrapperState>(wrapper->GetValue())) {
                // validate it
                auto ref = val->GetObject();
                bool equals = false;
                if (ref) {
                    // we have ref.. so make the compare here
                    napi_strict_equals(jsobj.GetEnv(), jsobj.ToNapiValue(), ref.ToNapiValue(), &equals);
                    if (!equals) {
                        // this may be a problem
                        // (there should only be a 1-1 mapping between js objects and native objects)
                        LOG_F("_JSW exists and points to different object!");
                    } else {
                        // objects already linked
                        return ref;
                    }
                } else {
                    // creating a new jsobject for existing native object that was bound before (but js object was GC:d)
                    // this is fine.
                    LOG_V("Rewrapping an object! (creating a new js object to replace a GC'd one for a native object)");
                }
            }
        } else {
            // create the wrapper property
            wrapper = ConstructProperty<SharedPtrIInterface>(
                name, nullptr, ObjectFlagBits::INTERNAL | ObjectFlagBits::NATIVE);
            AppMeta->AddProperty(wrapper);
        }
        // link native to js.
        auto res = BASE_NS::shared_ptr<CORE_NS::IInterface>(new JSWrapperState(jsobj, obj->GetClassName()));
        wrapper->SetValue(res);
        return interface_cast<JSWrapperState>(res)->GetObject();
    }
    napi_value val;
    napi_get_null(jsobj.GetEnv(), &val);
    return { jsobj.GetEnv(), val };
}

void DebugNativesHavingJS()
{
    LOG_F("Dump MetaObjects");
    for (auto&& v : META_NS::GetObjectRegistry().GetAllObjectInstances()) {
        if (auto i = interface_cast<META_NS::IMetadata>(v)) {
            auto II = interface_cast<META_NS::IObject>(i);
            BASE_NS::string classname;
            if (II) {
                classname = II->GetClassName();
            }
            auto p = i->GetProperty<META_NS::SharedPtrIInterface>("_JSW", META_NS::MetadataQuery::EXISTING);
            if (!p) {
                p = i->GetProperty<META_NS::SharedPtrIInterface>("_JSWMesh", META_NS::MetadataQuery::EXISTING);
            }
            if (p) {
                LOG_F("jsobj: %s %s (strong count %u) (has JSW)", classname.c_str(), v->GetName().c_str(),
                    v.use_count());

                if (auto val = interface_cast<JSWrapperState>(p->GetValue())) {
                    if (auto ref = val->GetObject()) {
                        LOG_F("\tJS object still alive.");
                        if (auto* tro = ref.GetRoot()) {
                            auto nat = tro->GetNativeObject();
                            if (!nat) {
                                LOG_F("\tand is wrapped but has no native object.");
                            } else {
                                if (interface_cast<CORE_NS::IInterface>(nat) !=
                                    interface_cast<CORE_NS::IInterface>(v)) {
                                    LOG_F("\t** JS OBJECT POINTS TO DIFFERENT NATIVE OBJECT! **");

                                    if (auto m = tro->GetNativeObject()) {
                                        auto JI = interface_cast<META_NS::IObject>(m);
                                        if (JI != II) {
                                            BASE_NS::string classname;
                                            if (JI) {
                                                classname = JI->GetClassName();
                                            }
                                            LOG_F("but the _JSW points to another object?: %s %s (strong count %u) "
                                                  "strong: %d",
                                                classname.c_str(), m->GetName().c_str(), m.use_count(),
                                                tro->IsStrong());
                                        }
                                    }
                                } else {
                                    LOG_F("\t and correctly links to this native object.");
                                }
                            }
                        } else {
                            LOG_F("\t.. but is unwrapped (null tro).");
                        }
                    } else {
                        LOG_F("Javascript object already dead.");
                    }
                }
            } else {
                LOG_F("obj: %s %s (strong count %u)", classname.c_str(), v->GetName().c_str(),
                    v.use_count());
            }
        }
    }
}