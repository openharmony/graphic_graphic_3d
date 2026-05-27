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

#ifndef SCENECOMPONENTJS_H
#define SCENECOMPONENTJS_H

#include "BaseObjectJS.h"
#include "PropertyProxy.h"

#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>

#include "export.h"

class SCENE_ADDON_PUBLIC SceneComponentJS final : public BaseObject {
public:
    static constexpr uint32_t ID = 200;
    static void Init(napi_env env, napi_value exports);

    SceneComponentJS(napi_env, napi_callback_info);
    SceneComponentJS(const SceneComponentJS&) = delete;
    SceneComponentJS& operator=(const SceneComponentJS&) = delete;
    ~SceneComponentJS() override;
    void* GetInstanceImpl(uint32_t) override;

    struct LazyProperty {
        LazyProperty() = default;
        LazyProperty(const LazyProperty&) = delete;
        LazyProperty& operator=(const LazyProperty&) = delete;

        SceneComponentJS* owner;
        BASE_NS::shared_ptr<PropertyProxy> proxy;
        BASE_NS::string name;      // short JS property name (e.g. "position")
        BASE_NS::string fullPath;  // full dotted path for CreateProperty (e.g. "TransformComponent.position")

        PropertyProxy* GetOrCreateProxy();
        bool IsValid() const;
    };

private:
    napi_value Dispose(NapiApi::FunctionContext<>& ctx);
    void DisposeNative() override;
    void Finalize(napi_env env) override;

    void AddProperties(NapiApi::Object meJs, const META_NS::IObject::Ptr& obj);
    NapiApi::StrongRef jsProps_;
    NapiApi::WeakObjectRef scene_;
    BASE_NS::vector<BASE_NS::unique_ptr<LazyProperty>> lazyProps_;
};
#endif
