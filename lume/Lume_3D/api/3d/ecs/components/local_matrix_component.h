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

#if !defined(LOCAL_MATRIX_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define LOCAL_MATRIX_COMPONENT

#if !defined(IMPLEMENT_MANAGER)

#include <3d/namespace.h>
#include <base/math/matrix.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/property_types.h>

CORE3D_BEGIN_NAMESPACE()
#endif
BEGIN_COMPONENT(ILocalMatrixComponentManager, LocalMatrixComponent)
    /** Local matrix component */
    DEFINE_PROPERTY(BASE_NS::Math::Mat4X4, matrix, "Matrix", 0, VALUE(BASE_NS::Math::IDENTITY_4X4))
END_COMPONENT(ILocalMatrixComponentManager, LocalMatrixComponent, "06dce540-17f6-4446-ad43-2bfd7c61aa0d")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif
