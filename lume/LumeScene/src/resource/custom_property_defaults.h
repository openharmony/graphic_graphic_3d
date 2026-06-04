/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SCENE_SRC_RESOURCE_CUSTOM_PROPERTY_DEFAULTS_H
#define SCENE_SRC_RESOURCE_CUSTOM_PROPERTY_DEFAULTS_H

#include <scene/base/namespace.h>

#include <core/json/json.h>
#include <render/device/intf_shader_manager.h>

#include <meta/interface/intf_metadata.h>

SCENE_BEGIN_NAMESPACE()

const CORE_NS::json::value* FindCustomPropertiesJson(
    const RENDER_NS::IShaderManager& sman, const RENDER_NS::RenderHandleReference& shader);

void UpdateCustomPropertyDefaults(const META_NS::IMetadata::Ptr& meta, const CORE_NS::json::value& propsJson);

SCENE_END_NAMESPACE()

#endif
