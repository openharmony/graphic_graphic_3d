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

#include "PropertyProxy.h"

#include <meta/api/make_callback.h>
#include <meta/api/util.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <napi_api.h>

#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_ecs_object_access.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/intf_render_resource.h>
#include <scene/interface/intf_shader.h>

#include <3d/ecs/components/render_handle_component.h>

#include "BaseObjectJS.h"
#include "ColorProxy.h"
#include "QuatProxy.h"
#include "Vec2Proxy.h"
#include "Vec3Proxy.h"
#include "Vec4Proxy.h"

PropertyProxy::~PropertyProxy()
{
    // should be a no-op.
}

PropertyProxy::PropertyProxy(META_NS::IProperty::Ptr prop) : prop_(prop)
{
    assert(prop);
}

META_NS::IProperty::Ptr PropertyProxy::GetPropertyPtr() const noexcept
{
    return prop_.lock();
}

void PropertyProxy::SetExtra(const BASE_NS::shared_ptr<CORE_NS::IInterface> extra)
{
    extra_ = extra;
}

const BASE_NS::shared_ptr<CORE_NS::IInterface> PropertyProxy::GetExtra() const noexcept
{
    return extra_.lock();
}

void PropertyProxy::ResetValue()
{
    if (auto p = GetPropertyPtr()) {
        p->ResetValue();
    }
}

ObjectPropertyProxy::MemberProxy::MemberProxy(ObjectPropertyProxy* p, BASE_NS::string m) : proxy_(p), memb_(m) {}
ObjectPropertyProxy::MemberProxy::~MemberProxy() {}
const BASE_NS::string_view ObjectPropertyProxy::MemberProxy::Name() const
{
    return memb_;
}

napi_value ObjectPropertyProxy::MemberProxy::Getter(napi_env e, napi_callback_info i)
{
    NapiApi::FunctionContext<> info(e, i);
    auto pc = static_cast<ObjectPropertyProxy::MemberProxy*>(info.GetData());
    if ((pc) && (pc->proxy_)) {
        return pc->proxy_->GetMemberValue(info.Env(), pc->memb_);
    }
    return info.GetUndefined();
}
napi_value ObjectPropertyProxy::MemberProxy::Setter(napi_env e, napi_callback_info i)
{
    NapiApi::FunctionContext<> info(e, i);
    auto pc = static_cast<ObjectPropertyProxy::MemberProxy*>(info.GetData());
    if ((pc) && (pc->proxy_)) {
        pc->proxy_->SetMemberValue(info, pc->memb_);
    }
    return info.GetUndefined();
}

void ObjectPropertyProxy::Create(napi_env env, const BASE_NS::string jsName)
{
    if (jsName.empty()) {
        obj_ = NapiApi::StrongRef { NapiApi::Object(env) };
    } else {
        NapiApi::MyInstanceState* mis;
        NapiApi::MyInstanceState::GetInstance(env, reinterpret_cast<void**>(&mis));
        if (!mis) {
            CORE_LOG_E("Could not Get Napi Instance");
            return;
        }
        auto ref = NapiApi::Object(env, mis->GetRef());
        auto cl = ref.Get(jsName.c_str());
        if (cl) {
            napi_value value;
            napi_new_instance(env, cl, 0, nullptr, &value);
            obj_ = NapiApi::StrongRef { NapiApi::Object(env, value) };
        }
    }
    if (obj_.IsEmpty()) {
        CORE_LOG_F("Could not create property object for %s", jsName.c_str());
    }
}

void ObjectPropertyProxy::Hook(const BASE_NS::string member)
{
    if (obj_.IsEmpty()) {
        return;
    }
    auto ValueObject = obj_.GetObject();
    auto* accessor = new MemberProxy(this, member);
    accessors_.push_back(BASE_NS::unique_ptr<MemberProxy>(accessor));
    ValueObject.AddProperty({ accessor->Name().data(), nullptr, nullptr, MemberProxy::Getter, MemberProxy::Setter,
        nullptr, napi_default_jsproperty, static_cast<void*>(accessor) });
}

void ObjectPropertyProxy::Reset()
{
    napi_status status;
    if (obj_.IsEmpty()) {
        return;
    }

    NapiApi::Env env(obj_.GetEnv());
    // unhook all hooked members.
    auto ValueObject = obj_.GetObject();
    for (; !accessors_.empty();) {
        auto it(accessors_.begin());
        // remove the property
        ValueObject.DeleteProperty((*it)->Name());
        // delete accessor
        accessors_.erase(it);
    }
    bool exception = false;
    status = napi_is_exception_pending(env, &exception);
    if (exception) {
        // if it's set, just clear it.
        // interestingly even though the napi_*_property might have returned napi_pending_exception (or other error
        // state) it seems that napi_is_exception_pending WILL return false though.
        napi_value res;
        status = napi_get_and_clear_last_exception(env, &res);
    }
}

void ObjectPropertyProxy::Destroy()
{
    Reset();
    // release ref.
    obj_.Reset();
}

napi_value ObjectPropertyProxy::Value()
{
    return obj_.GetValue();
}

void ObjectPropertyProxy::SetValue(NapiApi::FunctionContext<>& info)
{
    NapiApi::FunctionContext<NapiApi::Object> data { info };
    if (data) {
        SetValue(data.Arg<0>());
    }
}

ObjectPropertyProxy::ObjectPropertyProxy(META_NS::IProperty::Ptr prop) : PropertyProxy(prop) {}

ObjectPropertyProxy::~ObjectPropertyProxy()
{
    Reset();
}

EntityProxy::EntityProxy(NapiApi::Object scn, NapiApi::Object obj, META_NS::Property<CORE_NS::Entity> prop)
    : PropertyProxy(prop), obj_(obj), scene_(scn)
{
}

EntityProxy::~EntityProxy()
{
    // Unhook the objects.
    Reset();
}

void EntityProxy::Reset()
{
    auto prop = GetPropertyPtr();
    if (!obj_.IsEmpty() && prop) {
        obj_.GetObject().DeleteProperty(prop->GetName());
    }
    obj_.Reset();
    scene_.Reset();
}

void EntityProxy::SetValue(const CORE_NS::Entity v)
{
    META_NS::SetValue(GetProperty<CORE_NS::Entity>(), v);
}

napi_value EntityProxy::Value()
{
    NapiApi::Env env(obj_.GetEnv());
    if (auto entity = META_NS::GetValue(GetProperty<CORE_NS::Entity>()); CORE_NS::EntityUtil::IsValid(entity)) {
        if (auto scene = scene_.GetObject<SCENE_NS::IScene>()) {
            if (auto internal = scene->GetInternalScene()) {
                SCENE_NS::INode::Ptr node;

                if (auto shader = shader_.GetObject()) {
                    return shader.ToNapiValue();
                } else {
                    internal
                        ->AddTask([&node, &internal, &entity]() {
                            if ((node = internal->FindNode(entity, {}))) {
                                return;
                            }
                        })
                        .Wait();
                }

                if (node) {
                    NapiApi::Object parms(env);
                    napi_value args[] = { scene_.GetValue(), parms.ToNapiValue() };
                    return CreateFromNativeInstance(env, node, PtrType::WEAK, args).ToNapiValue();
                } else {
                    LOG_E("Unable to determine the type of the entity stored in the property");
                }
            }
        }
    }
    return env.GetNull();
}

void EntityProxy::SetValue(NapiApi::FunctionContext<>& info)
{
    NapiApi::FunctionContext<NapiApi::Object> data { info };
    if (data) {
        NapiApi::Object val = data.Arg<0>();
        CORE_NS::Entity entity;
        if (auto ecsoa = val.GetNative<SCENE_NS::IEcsObjectAccess>()) {
            if (auto ecso = ecsoa->GetEcsObject()) {
                entity = ecso->GetEntity();
            }
        } else if (auto renderResource = val.GetNative<SCENE_NS::IRenderResource>()) {
            shader_ = NapiApi::StrongRef(data.Env(), val.ToNapiValue());
            if (auto scene = scene_.GetObject<SCENE_NS::IScene>()) {
                auto internalScene = scene->GetInternalScene();
                scene->GetInternalScene()
                    ->AddTask([&entity, internalScene, renderResource, this]() {
                        auto& ecsContext = internalScene->GetEcsContext();
                        entityReference_ = ecsContext.GetRenderHandleEntity(renderResource->GetRenderHandle());
                        entity = entityReference_;
                    })
                    .Wait();
            }
        } else {
            LOG_E("Unknown entity type given for entity property");
        }
        META_NS::SetValue(GetProperty<CORE_NS::Entity>(), entity);
    }
}

void ImageProxy::Reset()
{
    obj_.Reset();
    scene_.Reset();
}

ImageProxy::ImageProxy(NapiApi::Object scn, NapiApi::Object obj, META_NS::Property<SCENE_NS::IImage::Ptr> prop)
    : PropertyProxy(prop), obj_(obj), scene_(scn)
{
}
ImageProxy::~ImageProxy()
{
    // Unhook the objects.
    Reset();
}
void ImageProxy::SetValue(const SCENE_NS::IBitmap::Ptr& v)
{
    META_NS::SetValue(GetProperty<SCENE_NS::IBitmap::Ptr>(), v);
}
napi_value ImageProxy::Value()
{
    auto value = META_NS::GetValue(GetProperty<SCENE_NS::IImage::Ptr>());
    NapiApi::Env env(obj_.GetEnv());
    if (value) {
        NapiApi::Object parms(env);
        napi_value args[] = { scene_.GetValue(), parms.ToNapiValue() };
        BASE_NS::string uri;
        if (auto m = interface_cast<META_NS::IMetadata>(value)) {
            if (auto p = m->GetProperty<BASE_NS::string>("Uri")) {
                uri = p->GetValue();
            }
        }
        parms.Set("uri", uri);
        return CreateFromNativeInstance(env, value, PtrType::WEAK, args).ToNapiValue();
    }
    return env.GetNull();
}
void ImageProxy::SetValue(NapiApi::FunctionContext<>& info)
{
    NapiApi::FunctionContext<NapiApi::Object> data { info };
    if (data) {
        NapiApi::Object val = data.Arg<0>();
        auto bitmap = val.GetNative<SCENE_NS::IImage>();
        META_NS::SetValue(GetProperty<SCENE_NS::IImage::Ptr>(), bitmap);
    }
}

#define SET_AND_RETURN(cc) \
    if (META_NS::IsCompatibleWith<cc>(t)) {\
        return BASE_NS::shared_ptr { new TypeProxy<cc>(obj, t) };\
    }

BASE_NS::shared_ptr<PropertyProxy> PropertyToProxy(
    NapiApi::Object scene, NapiApi::Object obj, const META_NS::IProperty::Ptr& t)
{
    SET_AND_RETURN(float)
    SET_AND_RETURN(int8_t)
    SET_AND_RETURN(uint8_t)
    SET_AND_RETURN(int16_t)
    SET_AND_RETURN(uint16_t)
    SET_AND_RETURN(int32_t)
    SET_AND_RETURN(uint32_t)
    SET_AND_RETURN(int64_t)
    SET_AND_RETURN(uint64_t)
    SET_AND_RETURN(BASE_NS::string)
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec2>(t)) {
        return BASE_NS::shared_ptr { new Vec2Proxy(obj.GetEnv(), t) };
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec3>(t)) {
        return BASE_NS::shared_ptr { new Vec3Proxy(obj.GetEnv(), t) };
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec4>(t)) {
        return BASE_NS::shared_ptr { new Vec4Proxy(obj.GetEnv(), t) };
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Quat>(t)) {
        return BASE_NS::shared_ptr { new QuatProxy(obj.GetEnv(), t) };
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Color>(t)) {
        return BASE_NS::shared_ptr { new ColorProxy(obj.GetEnv(), META_NS::Property<BASE_NS::Color>(t)) };
    }
    if (META_NS::IsCompatibleWith<SCENE_NS::IImage::Ptr>(t)) {
        return BASE_NS::shared_ptr { new ImageProxy(scene, obj, t) };
    }
    if (META_NS::IsCompatibleWith<CORE_NS::Entity>(t)) {
        return BASE_NS::shared_ptr { new EntityProxy(scene, obj, t) };
    }
    SET_AND_RETURN(bool)
    auto any = META_NS::GetInternalAny(t);
    LOG_F("Unsupported property type [%s]", any ? any->GetTypeIdString().c_str() : "<Unknown>");
    return nullptr;
}

static napi_value DummyFunc(napi_env e, napi_callback_info i)
{
    NapiApi::FunctionContext<> info(e, i);
    return info.GetUndefined();
};
static napi_value PropProxGet(napi_env e, napi_callback_info i)
{
    NapiApi::FunctionContext<> info(e, i);
    auto pc = static_cast<PropertyProxy*>(info.GetData());
    if (pc) {
        return pc->Value();
    }
    return info.GetUndefined();
};
static napi_value PropProxSet(napi_env e, napi_callback_info i)
{
    NapiApi::FunctionContext<>info (e, i);
    auto pc = static_cast<PropertyProxy*>(info.GetData());
    if (pc) {
        pc->SetValue(info);
    }
    return info.GetUndefined();
};
napi_property_descriptor CreateProxyDesc(const char* name, BASE_NS::shared_ptr<PropertyProxy> proxy)
{
    napi_property_descriptor desc { name, nullptr, nullptr, nullptr, nullptr, nullptr, napi_default_jsproperty,
        static_cast<void*>(proxy.get()) };
    if (proxy) {
        desc.getter = PropProxGet;
        desc.setter = PropProxSet;
    } else {
        desc.getter = desc.setter = DummyFunc;
    }
    return desc;
}
