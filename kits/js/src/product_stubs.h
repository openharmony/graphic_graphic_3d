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

#ifndef PRODUCT_STUBS_H
#define PRODUCT_STUBS_H

#ifdef __SCENE_ADAPTER__
#include <parameters.h>
#include "scene_adapter/scene_adapter.h"
#endif

#include <meta/interface/intf_metadata.h>
#include <scene/interface/intf_camera.h>

// Private hooks for RenderContextJS
class RenderContextJSPrivate {
public:
#ifdef __SCENE_ADAPTER__
    static void InitSystems(std::shared_ptr<OHOS::Render3D::SceneAdapter> sceneAdapter, const BASE_NS::string& uri);
#endif
    static void PopulateCustomParams(META_NS::IMetadata::Ptr params);
};

// Ptivate hooks for SceneJS
class SceneJSPrivate {
public:
    static void InitCamera(SCENE_NS::ICamera::Ptr camera, uint32_t& pipeline);
};

#endif
