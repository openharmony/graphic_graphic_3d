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

#ifndef SCENEPLUGIN_COMPATIBILITY_H
#define SCENEPLUGIN_COMPATIBILITY_H

#include <base/math/mathf.h>
#include <base/math/matrix_util.h>
#include <base/math/quaternion_util.h>
#include <base/math/vector.h>
#include <base/util/color.h>

#include <meta/base/meta_types.h>

#include "scene_plugin/namespace.h"

SCENE_BEGIN_NAMESPACE()

using Color = Base::Color;

static const Color BLACK_COLOR = BASE_NS::MakeColorFromLinear(0xFF000000);
static const Color WHITE_COLOR = BASE_NS::MakeColorFromLinear(0xFFFFFFFF);
static const Color BLUE_COLOR = BASE_NS::MakeColorFromLinear(0xFF0000FF);
static const Color GREEN_COLOR = BASE_NS::MakeColorFromLinear(0xFF00FF00);
static const Color RED_COLOR = BASE_NS::MakeColorFromLinear(0xFFFF0000);
static const Color TRANSPARENT_COLOR = BASE_NS::MakeColorFromLinear(0x00000000);

namespace Colors {
#undef TRANSPARENT
static constexpr Color TRANSPARENT { 0.f, 0.f, 0.f, 0.f };
static constexpr Color BLACK { 0.f, 0.f, 0.f, 1.f };
static constexpr Color GRAY { .5f, .5f, .5f, 1.f };
static constexpr Color WHITE { 1.f, 1.f, 1.f, 1.f };
static constexpr Color RED { 1.f, 0.f, 0.f, 1.f };
static constexpr Color GREEN { 0.f, 1.f, 0.f, 1.f };
static constexpr Color BLUE { 0.f, 0.f, 1.f, 1.f };
static constexpr Color YELLOW { 1.f, 1.f, 0.f, 1.f };
} // namespace Colors

SCENE_END_NAMESPACE()


#endif // SCENEPLUGIN_COMPATIBILITY_H
