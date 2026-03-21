/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#if !defined(PLUGIN_API_FG_COMPNENT_H) || defined(IMPLEMENT_MANAGER)
#define PLUGIN_API_FG_COMPNENT_H

#include <cstdint>
#include <base/math/quaternion.h>
#include <base/math/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_component_manager.h>

#include <render/device/pipeline_layout_desc.h>

struct FGDetailConfiguration {
    /** FG quality type name */
    enum FGQualityType {
        QUALITY_TYPE_FIX = 0,

        QUALITY_TYPE_NORMAL = 1,

        QUALITY_TYPE_LOW = 2,

        QUALITY_TYPE_BAD = 3,
    };

    enum FGAlgorithm {
        FG_HYDRA = 0,
        FG_GRIDWARP = 1,
        FG_IOI = 2,
        FG_ALGO_COUNT
    };

    /** frame generation quality type */
    FGQualityType qualityType { FGQualityType::QUALITY_TYPE_NORMAL };
    FGAlgorithm algorithm { FGAlgorithm::FG_HYDRA };

    int32_t FGWidth {0};
    int32_t FGHeight {0};

    bool withSR {false};
};

/* FG effect component
 * Holds on to the storage required for FGs in the effect.
 */
BEGIN_COMPONENT(IFGComponentManager, FGComponent)
#ifndef IMPLEMENT_MANAGER
    enum FlagBits :: uint32_t {
      FG1_BIT = (1 << 0),
      FG2_BIT = (1 << 1),
      FG3_BIT = (1 << 2),
    };
  
    /** Container for frame_generation flag bits */
    using Flags = uint32_t;
#endif

    /** The method of frame_generation fill when rendering.
    */
    DEFINE_BITFIELD_PROPERTY(
        Flags, enableFlags, "Enabled FrameGeration", PropertyFlgs::IS_BITFILED, VALUE(0), FGComponent::FlagBits)
    
    /** FG detail configuration.
    */
    DEFINE_PROPERTY(FGDetailConfiguration, fgDetailConfiguration, "FG Detail Configuration", 0, ARRAY_VALUE())
    DEFINE_PROPERTY(uint32_t, algorithm, "algorithm", 0, 0u)
    DEFINE_PROPERTY(uint32_t, quality, "quality", 0, 1u)

END_COMPONENT(IFGComponentManager, FGComponent, "6b45abd1-5ce4-49ba-b6c6-2c282e20a7cf")

#endif