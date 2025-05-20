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

#ifndef CORE3D_RENDER_DEFAULT_CONSTANTS_H
#define CORE3D_RENDER_DEFAULT_CONSTANTS_H

#include <cstdint>

#include <3d/namespace.h>
#include <base/math/vector.h>

CORE3D_BEGIN_NAMESPACE()
/** Default debug constants */
struct DefaultDebugConstants {
    /** Render debug marker colors */
    static constexpr const BASE_NS::Math::Vec4 DEFAULT_DEBUG_COLOR { 0.25f, 0.75f, 0.75f, 1.0f };
};
/** @} */
CORE3D_END_NAMESPACE()

#endif // CORE3D_RENDER_DEFAULT_CONSTANTS_H
