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

#ifndef _RAYCAST_RESULT_H_
#define _RAYCAST_RESULT_H_

#include <scene/interface/intf_raycast.h>

#include "napi_api.h"

NapiApi::Object CreateRaycastResult(NapiApi::StrongRef scene, napi_env env, SCENE_NS::NodeHit hitResult);
SCENE_NS::RayCastOptions ToNativeOptions(napi_env env, NapiApi::Object raycastParameters);

#endif
