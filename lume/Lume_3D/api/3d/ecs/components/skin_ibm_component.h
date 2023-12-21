/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#if !defined(SKIN_IBM_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define SKIN_IBM_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/containers/vector.h>
#include <base/math/matrix.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/property_types.h>

CORE3D_BEGIN_NAMESPACE()
#endif
/** Contains the inverse bind matrices (IBM) of a skin. The transform each skin joint to the initial pose.
 */
BEGIN_COMPONENT(ISkinIbmComponentManager, SkinIbmComponent)

    DEFINE_PROPERTY(BASE_NS::vector<BASE_NS::Math::Mat4X4>, matrices, "", 0,)

END_COMPONENT(ISkinIbmComponentManager, SkinIbmComponent, "f62fc6d6-de51-42f6-86fd-bed8ce5f3d03")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif
