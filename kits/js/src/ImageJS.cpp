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
#include "ImageJS.h"

#include <scene/interface/intf_scene.h>

#include "JsObjectCache.h"
#include "ParamParsing.h"
#include "RenderContextJS.h"
#include "SceneJS.h"

using namespace SCENE_NS;

void ImageJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;
    // clang-format off
    BASE_NS::vector<napi_property_descriptor> props;
    SceneResourceImpl::GetPropertyDescs(props);
    props.emplace_back(GetProperty<uint32_t, ImageJS, &ImageJS::GetWidth>("width"));
    props.emplace_back(GetProperty<uint32_t, ImageJS, &ImageJS::GetHeight>("height"));

    // clang-format on

    napi_value func;
    auto status = napi_define_class(
        env, "Image", NAPI_AUTO_LENGTH, BaseObject::ctor<ImageJS>(), nullptr, props.size(), props.data(), &func);

    NapiApi::MyInstanceState *mis;
    NapiApi::MyInstanceState::GetInstance(env, (void **)&mis);
    if (mis) {
        mis->StoreCtor("Image", func);
    }
}

void ImageJS::DisposeNative(void *sc)
{
    if (!disposed_) {
        disposed_ = true;
        LOG_V("ImageJS::DisposeNative");
        DestroyBridge(this);
        if (resources_) {
            auto uri = ExtractUri(uri_.GetObject());
            LOG_V("#### dispoose uri: %s", uri.c_str());
            if (!uri.empty()) {
                ExecSyncTask([uri, resources = resources_]() -> META_NS::IAny::Ptr {
                    if (resources) {
                        resources->StoreBitmap(uri, nullptr);
                    }
                    return {};
                });
            } else {
                LOG_V("### No uri for image resource when disposing");
            }
            resources_->ReleaseDispose(reinterpret_cast<uintptr_t>(&scene_));
        }

        UnsetNativeObject();
        scene_.Reset();
        resources_.reset();
    }
}
void* ImageJS::GetInstanceImpl(uint32_t id)
{
    if (id == ImageJS::ID) {
        return static_cast<ImageJS*>(this);
    }
    if (auto ret = SceneResourceImpl::GetInstanceImpl(id)) {
        return ret;
    }
    return BaseObject::GetInstanceImpl(id);
}
void ImageJS::Finalize(napi_env env)
{
    FinalizeBridge(this);
    DisposeNative(nullptr);
    BaseObject::Finalize(env);
}

ImageJS::ImageJS(napi_env e, napi_callback_info i) : BaseObject(e, i), SceneResourceImpl(SceneResourceType::IMAGE)
{
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    NapiApi::Object meJs(fromJs.This());
    NapiApi::Object renderContext = fromJs.Arg<0>(); // access to owning context...
    // This is awkward (and unfair to base class)
    scene_ = renderContext;
    // Not sure if using a render context here and scene elsewhere is optimal solution in a long run
    if (!renderContext.GetJsWrapper<RenderContextJS>()) {
        CORE_LOG_V("### Not RenderContext");
        if (renderContext.GetJsWrapper<SceneJS>()) {
            LOG_V("### Render Context is scene");
            if (NapiApi::Function func = renderContext.Get<NapiApi::Function>("getRenderContext")){
                if (auto rc = NapiApi::Object(e, func.Invoke(renderContext, {}))) {
                    scene_ = rc;
                    LOG_V("### remapped render context to scene_");
                }
            }
        } else {
            LOG_F("Invalid render context for ImageJS");
        }
    }
    NapiApi::Object args = fromJs.Arg<1>();
    if (args.Get("uri")) {
        SetUri(args);
    } else {
        LOG_E("### building image without uri");
    }

    if (auto bitmap = GetNativeObject<SCENE_NS::IBitmap>()) {
        auto uri = ExtractUri(uri_.GetObject());
        LOG_V("### uri: %s", uri.c_str());
	    if (auto* m = interface_cast<META_NS::IMetadata>(bitmap)) {
            if (auto p = m->GetProperty("Uri", META_NS::MetadataQuery::EXISTING)) {
                auto prop = META_NS::ConstructProperty<BASE_NS::string>(
                    "Uri", uri, META_NS::ObjectFlagBits::INTERNAL | META_NS::ObjectFlagBits::NATIVE);
                m->AddProperty(prop);
            } else {
                LOG_V("### meta-uri: %s", META_NS::GetValue<BASE_NS::string>(p).c_str());
            }
        }
        if (const auto renderContextJs = scene_.GetJsWrapper<RenderContextJS>()) {
            resources_ = renderContextJs->GetResources();
            if (resources_) {
                resources_->DisposeHook(reinterpret_cast<uintptr_t>(&scene_), meJs);
                resources_->StoreBitmap(uri, BASE_NS::move(bitmap));
            } else {
                LOG_E("Cannot finish creating an image: no resources");
                assert(false);
                return;
            }
        }
    } else {
        LOG_E("Cannot finish creating an image: Native image object missing");
        assert(false);
        return;
    }
    if (const auto name = ExtractName(args); !name.empty()) {
        meJs.Set("name", name);
    }
    AddBridge("ImageJS", meJs);
}

ImageJS::~ImageJS()
{
    DisposeNative(nullptr);
}

napi_value ImageJS::GetWidth(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    uint32_t width = 0;
    if (auto env = interface_cast<IBitmap>(GetNativeObject())) {
        width = env->Size()->GetValue().x;
    }
    return ctx.GetNumber(width);
}

napi_value ImageJS::GetHeight(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    uint32_t height = 0;
    if (auto env = interface_cast<IBitmap>(GetNativeObject())) {
        height = env->Size()->GetValue().y;
    }
    return ctx.GetNumber(height);
}
