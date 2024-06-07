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

#ifndef OHOS_RENDER_3D_NODE_IMPL_H
#define OHOS_RENDER_3D_NODE_IMPL_H
#include <meta/interface/intf_object.h>

#include "QuatProxy.h"
#include "SceneResourceImpl.h"
#include "Vec3Proxy.h"
class NodeImpl : public SceneResourceImpl {
public:
    static constexpr uint32_t ID = 2;
    enum NodeType { NODE = 1, GEOMETRY = 2, CAMERA = 3, LIGHT = 4 };

    static void RegisterEnums(NapiApi::Object exports);

protected:
    static void GetPropertyDescs(BASE_NS::vector<napi_property_descriptor>& props);
    NodeImpl(NodeType type);
    virtual ~NodeImpl();

    void* GetInstanceImpl(uint32_t id);
    napi_value GetNodeType(NapiApi::FunctionContext<>& fc);

    napi_value GetPosition(NapiApi::FunctionContext<>& fc);
    void SetPosition(NapiApi::FunctionContext<NapiApi::Object>& fc);

    napi_value GetScale(NapiApi::FunctionContext<>& fc);
    void SetScale(NapiApi::FunctionContext<NapiApi::Object>& fc);

    napi_value GetRotation(NapiApi::FunctionContext<>& fc);
    void SetRotation(NapiApi::FunctionContext<NapiApi::Object>& fc);

    napi_value GetPath(NapiApi::FunctionContext<>& ctx);
    napi_value GetParent(NapiApi::FunctionContext<>& ctx);

    napi_value GetNodeByPath(NapiApi::FunctionContext<BASE_NS::string>& ctx);

    napi_value GetChildContainer(NapiApi::FunctionContext<>& fc); // returns a container object.

    napi_value GetVisible(NapiApi::FunctionContext<>& ctx);
    void SetVisible(NapiApi::FunctionContext<bool>& ctx);

    napi_value GetLayerMask(NapiApi::FunctionContext<>& ctx);

    napi_value Dispose(NapiApi::FunctionContext<>& ctx);

    napi_value GetLayerMaskEnabled(NapiApi::FunctionContext<uint32_t>& ctx);
    napi_value SetLayerMaskEnabled(NapiApi::FunctionContext<uint32_t, bool>& ctx);

    napi_value GetCount(NapiApi::FunctionContext<>& ctx);
    napi_value GetChild(NapiApi::FunctionContext<uint32_t>& ctx);

    napi_value ClearChildren(NapiApi::FunctionContext<>& ctx);
    napi_value InsertChildAfter(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx);
    napi_value AppendChild(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    napi_value RemoveChild(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    void ResetNativeObj(NapiApi::FunctionContext<>& ctx, NapiApi::Object& obj);

private:
    NodeType type_;
    BASE_NS::unique_ptr<Vec3Proxy> posProxy_ { nullptr };
    BASE_NS::unique_ptr<Vec3Proxy> sclProxy_ { nullptr };
    BASE_NS::unique_ptr<QuatProxy> rotProxy_ { nullptr };
};
#endif // OHOS_RENDER_3D_NODE_IMPL_H