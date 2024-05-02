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
#ifndef SCENEPLUGIN_INTF_RENDERCONFIGURATION_H
#define SCENEPLUGIN_INTF_RENDERCONFIGURATION_H

#include <scene_plugin/interface/intf_environment.h>

#include <meta/interface/intf_named.h>

SCENE_BEGIN_NAMESPACE()

// Implemented by SCENE_NS::ClassId::PostProcess
REGISTER_INTERFACE(IRenderConfiguration, "4d432499-8048-42ef-bca0-a73db61d5304")
class IRenderConfiguration : public META_NS::INamed {
    META_INTERFACE(META_NS::INamed, IRenderConfiguration, InterfaceId::IRenderConfiguration)
public:
    /**
     * @brief Environment settings.
     * @return
     */
    META_PROPERTY(IEnvironment::Ptr, Environment)
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IRenderConfiguration::WeakPtr);
META_TYPE(SCENE_NS::IRenderConfiguration::Ptr);

#endif // SCENEPLUGIN_INTF_RENDERCONFIGURATION_H
