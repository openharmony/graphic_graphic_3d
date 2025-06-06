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

#ifndef SHADERS__COMMON__DOTFIELD_COMMON_H
#define SHADERS__COMMON__DOTFIELD_COMMON_H

#include "render/shaders/common/render_compatibility_common.h"

// must match RenderDataStoreDefaultDotfield
#ifdef VULKAN
#define DOTFIELD_SIMULATION_TGS 64
#define DOTFIELD_MAX_COUNT 1024 * 128
#define DOTFIELD_EFFECT_MAX_COUNT 16
#else
constexpr uint32_t DOTFIELD_SIMULATION_TGS = 64u;
constexpr uint32_t DOTFIELD_MAX_COUNT = 1024u * 128u;
constexpr uint32_t DOTFIELD_EFFECT_MAX_COUNT = 16u;
#endif

#endif // SHADERS__COMMON__DOTFIELD_COMMON_H
