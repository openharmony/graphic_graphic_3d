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

#if !defined(INITIAL_TRANSFORM_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define INITIAL_TRANSFORM_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/containers/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/property_types.h>

CORE3D_BEGIN_NAMESPACE()
#endif
BEGIN_COMPONENT(IInitialTransformComponentManager, InitialTransformComponent)
    DEFINE_PROPERTY(uint32_t, references, "Reference count", 0, VALUE(0))
    DEFINE_PROPERTY(BASE_NS::vector<uint8_t>, initialData, "Initial data", 0,)
END_COMPONENT(IInitialTransformComponentManager, InitialTransformComponent, "3949c596-435b-4210-8a90-c8976c7168a4")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif