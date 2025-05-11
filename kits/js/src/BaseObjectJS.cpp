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

#include <scene/interface/intf_light.h>

#include "JsObjectCache.h"
#include "TrueRootObject.h"

BASE_NS::string GetConstructorName(const META_NS::IObject::Ptr& obj)
{
    if (!obj) {
        return "";
    }
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
    } else if (name == "Sampler") {
        // ok
    } else if (name == "Texture") {
        name = "MaterialProperty";
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


NapiApi::Object CreateFromNativeInstance(napi_env env, const META_NS::IObject::Ptr& obj, PtrType ptrType,
    const NapiApi::JsFuncArgs& args, BASE_NS::string_view pname)
{
    if (!obj) {
        napi_value null;
        napi_get_null(env, &null);
        return { env, null };
    }
    auto name = GetConstructorName(obj);
    return CreateFromNativeInstance(env, name, obj, ptrType, args, pname);
}

NapiApi::Object CreateFromNativeInstance(napi_env env, const BASE_NS::string& name, const META_NS::IObject::Ptr& obj,
    PtrType ptrType, const NapiApi::JsFuncArgs& args, BASE_NS::string_view pname)
{
    napi_value null;
    napi_get_null(env, &null);
    if (obj == nullptr) {
        return { env, null };
    }
    using namespace META_NS;
    if (const auto cached = FetchJsObj(obj, pname)) {
        // we have a cached js object for this native object
        return cached;
    }

    // Ensure we have at least one arg for injection.
    napi_value dummyArg[] = { NapiApi::Object { env }.ToNapiValue() };
    auto argsToUse = args.argc > 0 ? args : NapiApi::JsFuncArgs { dummyArg };
    TrueRootObject::InjectNativeObject(env, obj, ptrType, argsToUse);
    const auto newJsObj = NapiApi::Object { env, name, argsToUse };
    return StoreJsObj(obj, newJsObj, pname);
}
