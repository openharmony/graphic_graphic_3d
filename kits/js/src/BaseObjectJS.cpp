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
#include <scene_plugin/interface/intf_mesh.h>
#include <scene_plugin/interface/intf_nodes.h>

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
    BASE_NS::string name_;
    volatile int32_t count_ { 0 };
    napi_env env_;
    napi_ref ref_;
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
JSWrapperState::JSWrapperState(NapiApi::Object obj, BASE_NS::string_view name)
{
    name_ = name;
    LOG_F("JSWrapperState ++ %s", name_.c_str());
    env_ = obj.GetEnv();
    // Create a WEAK reference to the object
    napi_create_reference(env_, obj, 0, &ref_);
}
JSWrapperState::~JSWrapperState()
{
    // release the reference.

    napi_delete_reference(env_, ref_);
    LOG_F("JSWrapperState -- %s", name_.c_str());
}
NapiApi::Object JSWrapperState::GetObject()
{
    napi_value value;
    napi_get_reference_value(env_, ref_, &value);
    return { env_, value };
}

TrueRootObject::TrueRootObject() {}
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
    napi_get_instance_data(env, (void**)&mis);
    return NapiApi::Function(env, mis->FetchCtor(jsName));
}
NapiApi::Object CreateJsObj(napi_env env, const BASE_NS::string_view jsName, META_NS::IObject::Ptr real, bool strong,
    uint32_t argc, napi_value* argv)
{
    NapiApi::Object obj(GetJSConstructor(env, jsName), argc, argv);
    if (!obj) {
        return {};
    }
    auto oo = GetRootObject(env, obj);
    oo->SetNativeObject(real, strong);
    return obj;
}
bool IsInstanceOf(const NapiApi::Object& obj, const BASE_NS::string_view jsName)
{
    auto env = obj.GetEnv();
    auto cl = GetJSConstructor(env, jsName);
    bool result = false;
    auto status = napi_instanceof(env, obj, cl, &result);
    return result;
}

NapiApi::Object FetchJsObj(const META_NS::IObject::Ptr& obj)
{
    using namespace META_NS;

    // access hidden property.
    if (auto AppMeta = interface_pointer_cast<IMetadata>(obj)) {
        if (auto wrapper = AppMeta->GetPropertyByName<SharedPtrIInterface>("_JSW")) {
            // The native object already contains a JS object.
            return interface_cast<JSWrapperState>(wrapper->GetValue())->GetObject();
        }
    }
    return nullptr;
}
NapiApi::Object StoreJsObj(const META_NS::IObject::Ptr& obj, NapiApi::Object jsobj)
{
    using namespace META_NS;
    if (auto AppMeta = interface_pointer_cast<IMetadata>(obj)) {
        // Add a reference to the JS object to the native object.
        auto res = BASE_NS::shared_ptr<JSWrapperState>(new JSWrapperState(jsobj, obj->GetClassName()));
        auto& obr = GetObjectRegistry();
        auto wrapper = AppMeta->GetPropertyByName<SharedPtrIInterface>("_JSW");
        // check if the property exists.
        if (wrapper) {
            // .. does the wrapper exist? (ie. reference from native to js)
            if (auto val = interface_cast<JSWrapperState>(wrapper->GetValue())) {
                // validate it
                auto ref = val->GetObject();
                bool equals = false;
                if (ref) {
                    // we have ref.. so make the compare here
                    napi_strict_equals(jsobj.GetEnv(), jsobj, ref, &equals);
                    if (!equals) {
                        // this may be a problem
                        // (there should only be a 1-1 mapping between js objects and native objects)
                        CORE_LOG_F("_JSW exists and points to different object!");
                    } else {
                        // objects already linked
                        return ref;
                    }
                } else {
                    // creating a new jsobject for existing native object that was bound before (but js object was GC:d)
                    // this is fine.
                    CORE_LOG_V(
                        "Rewrapping an object! (creating a new js object to replace a GC'd one for a native object)");
                }
            }
        } else {
            // create the wrapper property
            wrapper = ConstructProperty<SharedPtrIInterface>(
                "_JSW", nullptr, ObjectFlagBits::INTERNAL | ObjectFlagBits::NATIVE);
            AppMeta->AddProperty(wrapper);
        }
        // link native to js.
        wrapper->SetValue(interface_pointer_cast<CORE_NS::IInterface>(res));
        return res->GetObject();
    }
    napi_value val;
    napi_get_null(jsobj.GetEnv(), &val);
    return { jsobj.GetEnv(), val };
}
NapiApi::Object CreateFromNativeInstance(
    napi_env env, const META_NS::IObject::Ptr& obj, bool strong, uint32_t argc, napi_value* argv)
{
    napi_value null;
    napi_get_null(env, &null);
    if (obj == nullptr) {
        return {};
    }
    using namespace META_NS;
    NapiApi::Object nodeJS = FetchJsObj(obj);
    if (nodeJS) {
        // we have a cached js object for this native object
        return nodeJS;
    }
    // no js object. create it.
    BASE_NS::string name { obj->GetClassName() };
    // specialize/remap class names & interfaces.
    if (name == "Bitmap") {
        name = "Image";
    } else if (name == "Tonemap") {
        name = "ToneMappingSettings";
    } else if (name == "PostProcess") {
        name = "PostProcessSettings";
    } else if (name == "Material") {
        // okay. specialize then...
        SCENE_NS::IMaterial* mat = interface_cast<SCENE_NS::IMaterial>(obj);
        auto shdr = mat->MaterialShader()->GetValue();
        auto shdruri = shdr->Uri()->GetValue();
        if (!shdruri.empty()) {
            name = "ShaderMaterial";
        } else {
            // hide other material types..
            return {};
        }
    } else if (name == "Shader") {
        // possible specialization?
        name = "Shader";
    } else if (name == "Node") {
        if (interface_cast<SCENE_NS::INode>(obj)->GetMesh()) {
            name = "Geometry";
        } else {
            name = "Node";
        }
    }

    MakeNativeObjectParam(env, obj, argc, argv);

    nodeJS = CreateJsObj(env, name, obj, strong, argc, argv);
    if (!nodeJS) {
        // EEK. could not create the object.
        CORE_LOG_E("Could not create JSObject for Class %s", name.c_str());
        return {};
    }
    return StoreJsObj(obj, nodeJS);
}