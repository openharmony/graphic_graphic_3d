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

#ifndef OHOS_RENDER_3D_NAMED_IMPL_H
#define OHOS_RENDER_3D_NAMED_IMPL_H
#include <napi_api.h>
class SceneResourceImpl {
public:
    static constexpr uint32_t ID = 1;
    enum SceneResourceType {
        /**
         * The resource is an Unknow.
         */
        UNKNOWN = 0,
        /**
         * The resource is an Node.
         */
        NODE = 1,
        /**
         * The resource is an Environment.
         */
        ENVIRONMENT = 2,
        /**
         * The resource is a Material.
         */
        MATERIAL = 3,
        /**
         * The resource is a Mesh.
         */
        MESH = 4,
        /**
         * The resource is an Animation.
         */
        ANIMATION = 5,
        /**
         * The resource is a Shader.
         */
        SHADER = 6,

        /**
         * The resource is a Image.
         */
        IMAGE = 7,
    };
    SceneResourceImpl(SceneResourceType type);
    virtual ~SceneResourceImpl();
    static void RegisterEnums(NapiApi::Object exports);

    void* GetInstanceImpl(uint32_t id);

protected:
    static void GetPropertyDescs(BASE_NS::vector<napi_property_descriptor>& props);

    void SetUri(const NapiApi::StrongRef& uri);
    napi_value GetObjectType(NapiApi::FunctionContext<>& ctx);
    napi_value GetName(NapiApi::FunctionContext<>& ctx);
    void SetName(NapiApi::FunctionContext<BASE_NS::string>& ctx);
    napi_value GetUri(NapiApi::FunctionContext<>& ctx);
    napi_value Dispose(NapiApi::FunctionContext<>& ctx);

    NapiApi::StrongRef scene_;
private:
    SceneResourceType type_;
    NapiApi::StrongRef uri_;
};
#endif // OHOS_RENDER_3D_NAMED_IMPL_H