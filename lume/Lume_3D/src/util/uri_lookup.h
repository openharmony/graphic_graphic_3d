/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef CORE_UTIL_URI_LOOKUP_H
#define CORE_UTIL_URI_LOOKUP_H

#include <3d/namespace.h>
#include <base/containers/string_view.h>
#include <core/ecs/entity.h>

CORE3D_BEGIN_NAMESPACE()
class IUriComponentManager;
// Instantiated for AnimationComponent, MaterialComponent, MeshComponent, and SkinIbmComponent. Others can be added to
// the cpp or implementation inlined here.
template<typename ComponentManager>
CORE_NS::Entity LookupResourceByUri(
    BASE_NS::string_view uri, const IUriComponentManager& uriManager, const ComponentManager& componentManager);
CORE3D_END_NAMESPACE()
#endif