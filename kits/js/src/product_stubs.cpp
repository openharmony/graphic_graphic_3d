/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifdef __SCENE_ADAPTER__
#include <parameters.h>
#include "scene_adapter/scene_adapter.h"
#endif

#include <meta/interface/intf_metadata.h>
#include <scene/interface/intf_camera.h>

#include "product_stubs.h"

// RenderContextJS
#ifdef __SCENE_ADAPTER__
void RenderContextJSPrivate::InitSystems(std::shared_ptr<OHOS::Render3D::SceneAdapter>, const BASE_NS::string&)
{
}
#endif

void RenderContextJSPrivate::PopulateCustomParams(META_NS::IMetadata::Ptr)
{
}

// SceneJS
void SceneJSPrivate::InitCamera(SCENE_NS::ICamera::Ptr, uint32_t&)
{
}
