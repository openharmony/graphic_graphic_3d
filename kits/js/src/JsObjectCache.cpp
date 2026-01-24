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
        NapiApi::Scope scope(env);
        if (!scope) {
            return false;
        }
        napi_value obj = nullptr;
        napi_get_reference_value(env, ref, &obj);
        if (obj) {
            // make sure it's disposed..
            // as the native backing died.
            napi_value fun = nullptr;
            napi_get_named_property(env, obj, "destroy", &fun);
            if (fun) {
                napi_value res = nullptr;
                napi_call_function(env, obj, fun, 0, nullptr, &res);
            }
        }
#ifdef __OHOS_PLATFORM__
        uint32_t result{};
        napi_reference_ref(env, ref, &result);
#endif
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
    if (auto appMeta = interface_pointer_cast<IMetadata>(obj)) {
        if (auto wrapper = appMeta->GetProperty<SharedPtrIInterface>(name, MetadataQuery::EXISTING)) {
            // The native object already contains a JS object.
            return interface_cast<JSWrapperState>(wrapper->GetValue())->GetObject();
        }
    }
    return {};
}
void DetachJsObj(META_NS::IObject::Ptr& obj, BASE_NS::string_view name)
{
    using namespace META_NS;
    if (auto appMeta = interface_pointer_cast<IMetadata>(obj)) {
        // Remove the jsw property from native object.
        const auto& obr = GetObjectRegistry();
        if (auto wrapper = appMeta->GetProperty<SharedPtrIInterface>(name, MetadataQuery::EXISTING)) {
            appMeta->RemoveProperty(wrapper);
        }
    }
}
NapiApi::Object StoreJsObj(const META_NS::IObject::Ptr& obj, const NapiApi::Object& jsobj, BASE_NS::string_view name)
{
    using namespace META_NS;
    if (auto appMeta = interface_pointer_cast<IMetadata>(obj)) {
        // Add a reference to the JS object to the native object.
        const auto& obr = GetObjectRegistry();
        auto wrapper = appMeta->GetProperty<SharedPtrIInterface>(name, META_NS::MetadataQuery::EXISTING);
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
            appMeta->AddProperty(wrapper);
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

#if !defined(ENABLE_DIAGNOSTICS) || (ENABLE_DIAGNOSTICS==0)
bool LogInterface(const META_NS::IObject::Ptr& v, META_NS::Property<META_NS::SharedPtrIInterface>& p,
    const BASE_NS::string& classname, META_NS::IObject* objInterface)
{
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
                                auto ji = interface_cast<META_NS::IObject>(m);
                                if (ji != objInterface) {
                                    BASE_NS::string classname;
                                    if (ji) {
                                        classname = ji->GetClassName();
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
        return true;
    }
    return false;
}

void DebugNativesHavingJS()
{
    LOG_F("Dump MetaObjects");
    for (auto&& v : META_NS::GetObjectRegistry().GetAllObjectInstances()) {
        if (auto i = interface_cast<META_NS::IMetadata>(v)) {
            auto objInterface = interface_cast<META_NS::IObject>(i);
            BASE_NS::string classname;
            if (objInterface) {
                classname = objInterface->GetClassName();
            }
            auto p = i->GetProperty<META_NS::SharedPtrIInterface>("_JSW", META_NS::MetadataQuery::EXISTING);
            if (!p) {
                p = i->GetProperty<META_NS::SharedPtrIInterface>("_JSWMesh", META_NS::MetadataQuery::EXISTING);
            }
            if (!p) {
                p = i->GetProperty<META_NS::SharedPtrIInterface>("_JSWMorpher", META_NS::MetadataQuery::EXISTING);
            }
            if (!LogInterface(v, p, classname, objInterface)) {
                LOG_F("obj: %s %s (strong count %u)", classname.c_str(), v->GetName().c_str(),
                    v.use_count());
            }
        }
    }
}
#else
#include <map>
#include <scene/ext/intf_component.h>
#include <scene/ext/intf_ecs_object_access.h>
#include <scene/interface/intf_mesh.h>
#include <vector>

struct bridgeState {
    BASE_NS::string type;
    enum state { undefined = 0, created = 1, disposed = 2, finalized = 3 } state;
    NapiApi::WeakObjectRef weak;
};
std::map<void*, bridgeState> gBridges;

void AddBridge(const BASE_NS::string_view& type, NapiApi::Object js)
{
    void* ptr = js.GetRoot();
    gBridges[ptr].type = type;
    gBridges[ptr].weak = js;
    gBridges[ptr].state = bridgeState::created;
}
void FinalizeBridge(void* ptr)
{
    gBridges[ptr].state = bridgeState::finalized;
}
void DisposeBridge(void* ptr)
{
    gBridges[ptr].state = bridgeState::disposed;
}
void DestroyBridge(void* ptr)
{
    gBridges.erase(ptr);
}

void DumpBridges()
{
    uint32_t zom = 0;
    uint32_t liv = 0;
    uint32_t oth = 0;
    uint32_t disp = 0;
    struct StateCount {
        uint32_t live = 0;
        uint32_t disp = 0;
        uint32_t zom = 0;
    };
    std::map<BASE_NS::string, StateCount> counts;
    const char* state[] = { "undefined", "created", "disposed", "finalized" };
    for (auto& t : gBridges) {
        const void* ptr = t.first;
        const auto& b = t.second;
        bool isAlive = b.weak.GetNapiObject(); //<META_NS::IObject>().get() != nullptr;

        if (b.state == bridgeState::disposed) {
            counts[b.type].disp++;
            disp++;
        } else if (b.state == bridgeState::created) {
            if (!isAlive) {
                zom++;
                counts[b.type].zom++;
            } else {
                liv++;
                counts[b.type].live++;
            }
        } else {
            oth++;
        }
    }
    LOG_F("#### Dump Objects");
    LOG_F("Zombie: %d Disposed:%d LiveOnes:%d  oth:%d", zom, disp, liv, oth);
    for (auto t : counts) {
        LOG_F("%20s live: %4d disposed: %4d zombie: %4d", t.first.c_str(), t.second.live, t.second.disp, t.second.zom);
    }
    LOG_F("Total count: %zd", gBridges.size());
}

struct Nod {
    BASE_NS::string ind;
    META_NS::IObject::Ptr obj;
};

namespace {
std::map<void*, bool> added;
std::vector<Nod> sorted;
std::map<void*, std::vector<META_NS::IObject::Ptr>> parents;
std::map<META_NS::IObject*, META_NS::IObject::Ptr> tmp;
}

void add(BASE_NS::string ind, std::vector<Nod>& sorted, std::map<void*, std::vector<META_NS::IObject::Ptr>>& parents,
    META_NS::IObject::Ptr obj)
{
    if (obj) {
        sorted.push_back({ ind, obj });
    }
    for (const auto& p : parents[obj.get()]) {
        add(ind + " ", sorted, parents, p);
    }
}
bool IsAdded(META_NS::IObject::Ptr obj)
{
    // is added under some object. (non null owner)
    return added.count(obj.get()) > 0;
}
void Add(META_NS::IObject::Ptr parent, META_NS::IObject::Ptr chld, bool always = false)
{
    if (!always && IsAdded(chld)) {
        // added to something already.
        return;
    }
    if (parent) {
        added[chld.get()] = true;
    }
    auto& chl = parents[parent.get()];
    if (always || chl.empty() || (std::find(chl.begin(), chl.end(), chld) == chl.end())) {
        chl.push_back(chld);
    }
}
void DoEcsObject(META_NS::IObject::Ptr v)
{
    // add ecsobjects for node..
    if (auto ec = interface_cast<SCENE_NS::IEcsObjectAccess>(v)) {
        // ecsobject..
        auto eco = interface_pointer_cast<META_NS::IObject>(ec->GetEcsObject());
        Add(v, eco, true);
    }
}
void DoAttachments(META_NS::IObject::Ptr v)
{
    if (auto att = interface_cast<META_NS::IAttach>(v)) {
        if (auto cont = att->GetAttachmentContainer(false)) {
            auto all = cont->GetAll();
            for (auto a : all) {
                if (IsAdded(a)) {
                    continue;
                }
                auto prop = interface_cast<META_NS::IProperty>(a);
                if (!prop) {
                    Add(v, a);
                    DoEcsObject(a);
                }
            }
        }
    }
}
void Scan()
{
    // nodes
    for (auto t : tmp) {
        const auto& v = t.second;
        if (IsAdded(v))
            continue;
        if (auto n = interface_cast<SCENE_NS::INode>(v)) {
            DoEcsObject(v);
            DoAttachments(v);
            if (auto access = interface_pointer_cast<SCENE_NS::IMeshAccess>(v)) {
                auto mesh = interface_pointer_cast<META_NS::IObject>(access->GetMesh().GetResult());
                Add(v, mesh);
                DoEcsObject(mesh);
            }
        }
    }
    // others..
    for (auto t : tmp) {
        const auto& v = t.second;
        if (IsAdded(v))
            continue;
        if (auto n = interface_cast<SCENE_NS::INode>(v)) {
            // skip as it's done already
            continue;
        }
        DoEcsObject(v);
        DoAttachments(v);
    }
}
void Sort()
{
    uint32_t jsA = 0;
    uint32_t jsD = 0;
    uint32_t o = 0;
    uint32_t jso = 0;
    uint32_t nmo = 0;

    for (auto& vv : sorted) {
        const auto& v = vv.obj;
        if (v == nullptr) {
            LOG_F("OBJ LOST");
            continue;
        }
        if (v.use_count() == 1) {
            // ephemereal link
            LOG_V("*GHOST*");
        }
        BASE_NS::string ind = vv.ind;
        BASE_NS::string classname(v->GetClassName());
        BASE_NS::string name = v->GetName();
        if (auto i = interface_cast<META_NS::IMetadata>(v)) {
            auto objInterface = interface_cast<META_NS::IObject>(i);
            auto p = i->GetProperty<META_NS::SharedPtrIInterface>("_JSW", META_NS::MetadataQuery::EXISTING);
            if (!p) {
                p = i->GetProperty<META_NS::SharedPtrIInterface>("_JSWMesh", META_NS::MetadataQuery::EXISTING);
            }
            if (!p) {
                p = i->GetProperty<META_NS::SharedPtrIInterface>("_JSWMorpher", META_NS::MetadataQuery::EXISTING);
            }
            if (p) {
                if (auto val = interface_cast<JSWrapperState>(p->GetValue())) {
                    auto rv = val->GetObject();
                    if (rv) {
                        jsA++;
                    } else {
                        jsD++;
                    }
                } else {
                    jso++;
                }
            } else {
                o++;
            }
        } else {
            // non meta
            nmo++;
        }
    }
    decltype(sorted) t2;
    sorted.swap(t2);
}
void DebugNativesHavingJS()
{
    for (auto& v : META_NS::GetObjectRegistry().GetAllObjectInstances()) {
        tmp[v.get()] = v;
    }

    Scan();

    // then do the ref dance.
    for (auto t : tmp) {
        const auto& v = t.second;
        if (IsAdded(v))
            continue;
        BASE_NS::shared_ptr<META_NS::IObject> own;
        own = v->Resolve(META_NS::RefUri { BASE_NS::string_view("ref:/../") });
        if (!own) {
            if (auto n = interface_cast<SCENE_NS::INode>(v)) {
                own = interface_pointer_cast<META_NS::IObject>(n->GetScene());
            }
        }
        Add(own, v);
    }
    for (const auto& p : parents[nullptr]) {
        add("", sorted, parents, p);
    }
    {
        decltype(added) t1;
        decltype(parents) t3;
        decltype(tmp) t4;
        added.swap(t1);
        parents.swap(t3);
        tmp.swap(t4);
    }

    Sort();
    DumpBridges();
}
#endif
