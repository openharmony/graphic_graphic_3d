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
#include "BaseObjectJS.h"

#include <meta/interface/intf_metadata.h>
#include <meta/interface/property/construct_property.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_light.h>

// this class is used to store a reference to a JS object in the metaobject.
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
    napi_threadsafe_function termfun { nullptr };
    struct fun_parm {
        napi_ref ref { nullptr };
        napi_threadsafe_function termfun { nullptr };
    };
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
    CORE_NS::AtomicIncrement(&count_);
}
void JSWrapperState::Unref()
{
    if (CORE_NS::AtomicDecrement(&count_) == 0) {
        delete this;
    }
}

JSWrapperState::JSWrapperState(NapiApi::Object obj, BASE_NS::string_view name) : name_(name), env_(obj.GetEnv())
{
    napi_status status;
    // Create a WEAK reference to the object
    status = napi_create_reference(env_, obj.ToNapiValue(), 0, &ref_);

    // Create a thread safe function to release the js reference, as the destructor MAY be called from engine thread.
    napi_value jname = nullptr;
    status = napi_create_string_latin1(env_, "JSW", NAPI_AUTO_LENGTH, &jname);
    status = napi_create_threadsafe_function(
        env_, nullptr, nullptr, jname, 0, 1, nullptr, &JSWrapperState::Final, nullptr, &JSWrapperState::Call, &termfun);
}
void JSWrapperState::Call(napi_env env, napi_value js_callback, void* context, void* inData)
{
    napi_status status;
    fun_parm* ref = (fun_parm*)inData;
    status = napi_delete_reference(env, ref->ref);
    status = napi_release_threadsafe_function(ref->termfun, napi_threadsafe_function_release_mode::napi_tsfn_release);
    delete ref;
}
void JSWrapperState::Final(napi_env env, void* finalize_data, void* context) {};

JSWrapperState::~JSWrapperState()
{
    napi_status status;
    // trigger the threadsafe function, so that the javascript "weak reference" will be destroyed.
    status = napi_call_threadsafe_function(
        termfun, new fun_parm { ref_, termfun }, napi_threadsafe_function_call_mode::napi_tsfn_blocking);
}
NapiApi::Object JSWrapperState::GetObject()
{
    napi_status status;
    napi_value value;
    status = napi_get_reference_value(env_, ref_, &value);
    return { env_, value };
}

TrueRootObject::TrueRootObject() {}
void TrueRootObject::destroy(TrueRootObject* object)
{
    delete object;
}

bool TrueRootObject::IsStrong() const
{
    return obj_ != nullptr;
}
void TrueRootObject::SetNativeObject(META_NS::IObject::Ptr real, bool strong)
{
    if (strong) {
        obj_ = real;
    } else {
        objW_ = real;
    }
}
META_NS::IObject::Ptr TrueRootObject::GetNativeObject()
{
    // if we have a strong ref...
    if (obj_) {
        // return that directly.
        return obj_;
    }
    // otherwise try to lock the weak reference.
    return objW_.lock();
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
NapiApi::Function GetJSConstructor(napi_env env, const BASE_NS::string_view jsName)
{
    NapiApi::MyInstanceState* mis;
    GetInstanceData(env, (void**)&mis);
    return NapiApi::Function(env, mis->FetchCtor(jsName));
}
NapiApi::Object CreateJsObj(napi_env env, const BASE_NS::string_view jsName, META_NS::IObject::Ptr real, bool strong,
    uint32_t argc, napi_value* argv)
{
    NapiApi::Object obj(GetJSConstructor(env, jsName), argc, argv);
    if (!obj) {
        return {};
    }
    auto oo = GetRootObject(obj);
    oo->SetNativeObject(real, strong);
    return obj;
}
bool IsInstanceOf(const NapiApi::Object& obj, const BASE_NS::string_view jsName)
{
    auto env = obj.GetEnv();
    auto cl = GetJSConstructor(env, jsName);
    bool result = false;
    auto status = napi_instanceof(env, obj.ToNapiValue(), cl, &result);
    return result;
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
NapiApi::Object StoreJsObj(const META_NS::IObject::Ptr& obj, NapiApi::Object jsobj, BASE_NS::string_view name)
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
                    LOG_V(
                        "Rewrapping an object! (creating a new js object to replace a GC'd one for a native object)");
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

BASE_NS::string GetConstructorName(const META_NS::IObject::Ptr& obj)
{
    BASE_NS::string name { obj->GetClassName() };
    // specialize/remap class names & interfaces.
    if (name == "Bitmap") {
        name = "Image";
    } else if (name == "Tonemap") {
        name = "ToneMappingSettings";
    } else if (name == "PostProcess") {
        name = "PostProcessSettings";
    } else if (name == "Material") {
        // ok
    } else if (name == "Shader") {
        // possible specialization?
    } else if (name == "EcsAnimation") {
        name = "Animation";
    } else if (name == "MeshNode") {
        name = "Geometry";
    } else if (name == "CameraNode") {
        name = "Camera";
    } else if (name == "LightNode") {
        SCENE_NS::ILight* lgt = interface_cast<SCENE_NS::ILight>(obj);
        if (lgt == nullptr) {
            LOG_E("lgt is null");
            return name;
        }
        auto type = lgt->Type()->GetValue();
        if (type == Scene::LightType::DIRECTIONAL) {
            name = "DirectionalLight";
        } else if (type == Scene::LightType::POINT) {
            name = "PointLight";
        } else if (type == Scene::LightType::SPOT) {
            name = "SpotLight";
        } else {
            name = "Node";
        }
    } else if (name.ends_with("Node")) {
        name = "Node";
    }
    return name;
}

NapiApi::Object CreateFromNativeInstance(napi_env env, const META_NS::IObject::Ptr& obj,
    bool strong, uint32_t argc, napi_value* argv, BASE_NS::string_view pname)
{
    napi_value null;
    napi_get_null(env, &null);
    if (obj == nullptr) {
        return {};
    }
    using namespace META_NS;
    NapiApi::Object nodeJS = FetchJsObj(obj, pname);
    if (nodeJS) {
        // we have a cached js object for this native object
        return nodeJS;
    }
    // no js object. create it.
    auto name = GetConstructorName(obj);
    MakeNativeObjectParam(env, obj, argc, argv);

    nodeJS = CreateJsObj(env, name, obj, strong, argc, argv);
    if (!nodeJS) {
        // EEK. could not create the object.
        LOG_E("Could not create JSObject for Class %s", name.c_str());
        return {};
    }
    return StoreJsObj(obj, nodeJS, pname);
}
void DebugNativesHavingJS(void)
{
    for (auto&& v : META_NS::GetObjectRegistry().GetAllObjectInstances()) {
        if (auto i = interface_cast<META_NS::IMetadata>(v)) {
            auto p = i->GetProperty<META_NS::SharedPtrIInterface>("_JSW", META_NS::MetadataQuery::EXISTING);
            if (!p) {
                p = i->GetProperty<META_NS::SharedPtrIInterface>("_JSWMesh", META_NS::MetadataQuery::EXISTING);
            }
            if (p) {
                LOG_W("obj: %s (%p, strong count %u)", v->GetName().c_str(), v.get(), v.use_count());
                if (auto val = interface_cast<JSWrapperState>(p->GetValue())) {
                    if (auto ref = val->GetObject()) {
                        auto* tro = ref.Native<TrueRootObject>();
                        if (tro) {
                            if (auto m = interface_pointer_cast<META_NS::IObject>(tro->GetNativeObject())) {
                                LOG_W("still has valid object: %s (%p, strong count %u) strong: %d (tro: %p)",
                                    m->GetName().c_str(), m.get(), m.use_count(), tro->IsStrong(), tro);
                            }
                        }
                    }
                }
            }
        }
    }
}
