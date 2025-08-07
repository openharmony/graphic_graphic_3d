/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "SceneTypesImpl.h"

namespace OHOS::Render3D::KITETS {
::SceneTypes::CustomGeometry CreateCustomGeometry() {
    // The parameters in the make_holder function should be of the same type
    // as the parameters in the constructor of the actual implementation class.
    return taihe::make_holder<CustomGeometryImpl, ::SceneTypes::CustomGeometry>();
}

::SceneTypes::CubeGeometry CreateCubeGeometry() {
    // The parameters in the make_holder function should be of the same type
    // as the parameters in the constructor of the actual implementation class.
    return taihe::make_holder<CubeGeometryImpl, ::SceneTypes::CubeGeometry>();
}

::SceneTypes::PlaneGeometry CreatePlaneGeometry() {
    // The parameters in the make_holder function should be of the same type
    // as the parameters in the constructor of the actual implementation class.
    return taihe::make_holder<PlaneGeometryImpl, ::SceneTypes::PlaneGeometry>();
}

::SceneTypes::SphereGeometry CreateSphereGeometry() {
    // The parameters in the make_holder function should be of the same type
    // as the parameters in the constructor of the actual implementation class.
    return taihe::make_holder<SphereGeometryImpl, ::SceneTypes::SphereGeometry>();
}
}  // namespace OHOS::Render3D::KITETS

using namespace OHOS::Render3D::KITETS;
// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_CreateCustomGeometry(CreateCustomGeometry);
TH_EXPORT_CPP_API_CreateCubeGeometry(CreateCubeGeometry);
TH_EXPORT_CPP_API_CreatePlaneGeometry(CreatePlaneGeometry);
TH_EXPORT_CPP_API_CreateSphereGeometry(CreateSphereGeometry);
// NOLINTEND