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

#ifndef SCENEPLUGINAPI_RENDERCONFIGURATION_H
#define SCENEPLUGINAPI_RENDERCONFIGURATION_H

#include <scene_plugin/api/render_configuration_uid.h>
#include <scene_plugin/interface/intf_render_configuration.h>

#include <meta/api/internal/object_api.h>
SCENE_BEGIN_NAMESPACE()

class RenderConfiguration final
    : public META_NS::Internal::ObjectInterfaceAPI<RenderConfiguration, ClassId::RenderConfiguration> {
    META_API(RenderConfiguration)
    API_OBJECT_CONVERTIBLE(IRenderConfiguration)
    API_CACHE_INTERFACE(IRenderConfiguration, RenderConfiguration)

public:
    API_INTERFACE_PROPERTY_CACHED(RenderConfiguration, Name, BASE_NS::string)
    API_INTERFACE_PROPERTY_CACHED(RenderConfiguration, Environment, IEnvironment::Ptr)
};

SCENE_END_NAMESPACE()

#endif // SCENEPLUGINAPI_RENDERCONFIGURATION_H
