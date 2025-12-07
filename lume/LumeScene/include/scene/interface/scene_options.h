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

#ifndef SCENE_INTERFACE_SCENE_OPTIONS_H
#define SCENE_INTERFACE_SCENE_OPTIONS_H

#include <scene/base/types.h>

#include <core/resources/intf_resource.h>

SCENE_BEGIN_NAMESPACE()

struct SceneOptions {
    /// Set custom system graph, empty one uses system default
    BASE_NS::string systemGraphUri;
    /// If true, scene will control any startables attached to nodes in the scene
    bool enableStartables { true };
    /// Create and add all resources when loading scene
    bool createResources { true };
    /// Resource id if this is loaded via resource manager
    CORE_NS::ResourceId resourceId;
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::SceneOptions)

#endif
