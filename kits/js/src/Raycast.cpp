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

#include "Raycast.h"

#include <meta/interface/intf_object.h>
#include <napi_api.h>
#include <scene/interface/component_util.h>
#include <scene/interface/intf_raycast.h>

#include "BaseObjectJS.h"
#include "NodeImpl.h"
#include "Vec3Proxy.h"

NapiApi::Object CreateRaycastResult(NapiApi::StrongRef scene, napi_env env, SCENE_NS::NodeHit hitResult)
{
    auto result = NapiApi::Object { env };

    napi_value args[] = { scene.GetValue(), NapiApi::Object { env }.ToNapiValue() };
    const auto jsNode = CreateFromNativeInstance(env, hitResult.node, PtrType::WEAK, args);
    if (auto nodeImpl = jsNode.GetJsWrapper<NodeImpl>()) {
        nodeImpl->Attached(true);
    }

    auto centerDistance = napi_value {};
    auto distance = napi_value {};
    napi_create_double(env, hitResult.distance, &distance);
    napi_create_double(env, hitResult.distanceToCenter, &centerDistance);

    result.Set("node", jsNode);
    result.Set("centerDistance", centerDistance);
    result.Set("distance", distance);
    result.Set("hitPosition", Vec3Proxy::ToNapiObject(hitResult.position, env));
    return result;
}

SCENE_NS::RayCastOptions ToNativeOptions(napi_env env, NapiApi::Object raycastParameters)
{
    auto layerMask = uint64_t {};
    auto jsObj = NapiApi::Object { env, raycastParameters.Get("layerMask") };
    if (auto rootObject = jsObj.GetRoot()) {
        // Layer masks are served as Node objects to the JS side.
        if (auto nativeNode = interface_pointer_cast<SCENE_NS::INode>(rootObject->GetNativeObject())) {
            if (auto layerComponent = SCENE_NS::GetComponent<SCENE_NS::ILayer>(nativeNode)) {
                layerMask = layerComponent->LayerMask()->GetValue();
            }
        }
    }

    auto rootNode = SCENE_NS::INode::ConstPtr {};
    auto anotherjsObj = NapiApi::Object { env, raycastParameters.Get("rootNode") };
    if (auto rootObject = anotherjsObj.GetRoot()) {
        rootNode = interface_pointer_cast<SCENE_NS::INode>(rootObject->GetNativeObject());
    }
    return { layerMask, rootNode };
}
