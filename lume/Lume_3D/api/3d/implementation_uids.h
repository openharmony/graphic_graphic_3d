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

#ifndef API_3D_IMPLEMENTATION_UIDS_H
#define API_3D_IMPLEMENTATION_UIDS_H

#include <3d/namespace.h>
#include <base/util/uid.h>

CORE3D_BEGIN_NAMESPACE()
static constexpr BASE_NS::Uid UID_3D_PLUGIN { "5832b35d-b28e-4343-8ad6-1c4e6f610473" };

static constexpr BASE_NS::Uid UID_GRAPHICS_CONTEXT { "437cbd33-b012-417c-b647-a0863f51829f" };
static constexpr BASE_NS::Uid UID_MESH_BUILDER { "7ece9671-6238-45b3-881b-fdf583181d5e" };
static constexpr BASE_NS::Uid UID_PICKING { "85ad23ec-00bf-40a0-ae5a-dac98e3e5c04" };
static constexpr BASE_NS::Uid UID_RENDER_NODE_SCENE_UTIL { "8cdc39e9-2c86-4cf7-a3ca-f739aee72012" };
CORE3D_END_NAMESPACE()

#endif // API_3D_IMPLEMENTATION_UIDS_H