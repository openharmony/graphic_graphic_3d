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

#if !defined(RSDZ_MODEL_ID_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define RSDZ_MODEL_ID_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/property_types.h>

CORE3D_BEGIN_NAMESPACE()
#endif // IMPLEMENT_MANAGER

/** RSDZ model id is used to store extra id from glTF2.
 */
BEGIN_COMPONENT(IRSDZModelIdComponentManager, RSDZModelIdComponent)
    /** Model id value in string format
     */
    DEFINE_ARRAY_PROPERTY(char, 64u, modelId, "Model ID", 0, "")

END_COMPONENT(IRSDZModelIdComponentManager, RSDZModelIdComponent, "a4b1877f-0feb-4d24-88a8-3d08eb5f535e")

#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif // IMPLEMENT_MANAGER

#endif // __RSDZ_MODEL_ID_COMPONENT__
