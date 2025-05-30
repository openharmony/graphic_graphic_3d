/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef SCENE_SRC_RESOURCE_RESOURCE_TYPES_H
#define SCENE_SRC_RESOURCE_RESOURCE_TYPES_H

#include <scene/interface/intf_render_context.h>
#include <scene/interface/scene_options.h>

SCENE_BEGIN_NAMESPACE()

void RegisterResourceTypes(const IRenderContext::Ptr& context, const SceneOptions& opts);
void RegisterResourceTypes(const CORE_NS::IResourceManager::Ptr& res, const META_NS::IMetadata::Ptr& args);

SCENE_END_NAMESPACE()

#endif