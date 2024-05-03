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

#ifndef API_3D_ECS_COMPONENTS_LAYER_DEFINES_H
#define API_3D_ECS_COMPONENTS_LAYER_DEFINES_H

#include <cstdint>

#include <3d/namespace.h>

CORE3D_BEGIN_NAMESPACE()
/** \addtogroup group_layerdefines
 *  @{
 */
/** Layer mask bits. These enum values are for providing property metadata. */
enum LayerFlagBits : uint64_t {
    CORE_LAYER_FLAG_BIT_NONE = 0ULL,
    CORE_LAYER_FLAG_BIT_00 = (1ULL << 0U),
    CORE_LAYER_FLAG_BIT_01 = (1ULL << 1U),
    CORE_LAYER_FLAG_BIT_02 = (1ULL << 2U),
    CORE_LAYER_FLAG_BIT_03 = (1ULL << 3U),
    CORE_LAYER_FLAG_BIT_04 = (1ULL << 4U),
    CORE_LAYER_FLAG_BIT_05 = (1ULL << 5U),
    CORE_LAYER_FLAG_BIT_06 = (1ULL << 6U),
    CORE_LAYER_FLAG_BIT_07 = (1ULL << 7U),
    CORE_LAYER_FLAG_BIT_08 = (1ULL << 8U),
    CORE_LAYER_FLAG_BIT_09 = (1ULL << 9U),
    CORE_LAYER_FLAG_BIT_10 = (1ULL << 10U),
    CORE_LAYER_FLAG_BIT_11 = (1ULL << 11U),
    CORE_LAYER_FLAG_BIT_12 = (1ULL << 12U),
    CORE_LAYER_FLAG_BIT_13 = (1ULL << 13U),
    CORE_LAYER_FLAG_BIT_14 = (1ULL << 14U),
    CORE_LAYER_FLAG_BIT_15 = (1ULL << 15U),
    CORE_LAYER_FLAG_BIT_16 = (1ULL << 16U),
    CORE_LAYER_FLAG_BIT_17 = (1ULL << 17U),
    CORE_LAYER_FLAG_BIT_18 = (1ULL << 18U),
    CORE_LAYER_FLAG_BIT_19 = (1ULL << 19U),
    CORE_LAYER_FLAG_BIT_20 = (1ULL << 20U),
    CORE_LAYER_FLAG_BIT_21 = (1ULL << 21U),
    CORE_LAYER_FLAG_BIT_22 = (1ULL << 22U),
    CORE_LAYER_FLAG_BIT_23 = (1ULL << 23U),
    CORE_LAYER_FLAG_BIT_24 = (1ULL << 24U),
    CORE_LAYER_FLAG_BIT_25 = (1ULL << 25U),
    CORE_LAYER_FLAG_BIT_26 = (1ULL << 26U),
    CORE_LAYER_FLAG_BIT_27 = (1ULL << 27U),
    CORE_LAYER_FLAG_BIT_28 = (1ULL << 28U),
    CORE_LAYER_FLAG_BIT_29 = (1ULL << 29U),
    CORE_LAYER_FLAG_BIT_30 = (1ULL << 30U),
    CORE_LAYER_FLAG_BIT_31 = (1ULL << 31U),
    CORE_LAYER_FLAG_BIT_32 = (1ULL << 32U),
    CORE_LAYER_FLAG_BIT_33 = (1ULL << 33U),
    CORE_LAYER_FLAG_BIT_34 = (1ULL << 34U),
    CORE_LAYER_FLAG_BIT_35 = (1ULL << 35U),
    CORE_LAYER_FLAG_BIT_36 = (1ULL << 36U),
    CORE_LAYER_FLAG_BIT_37 = (1ULL << 37U),
    CORE_LAYER_FLAG_BIT_38 = (1ULL << 38U),
    CORE_LAYER_FLAG_BIT_39 = (1ULL << 39U),
    CORE_LAYER_FLAG_BIT_40 = (1ULL << 40U),
    CORE_LAYER_FLAG_BIT_41 = (1ULL << 41U),
    CORE_LAYER_FLAG_BIT_42 = (1ULL << 42U),
    CORE_LAYER_FLAG_BIT_43 = (1ULL << 43U),
    CORE_LAYER_FLAG_BIT_44 = (1ULL << 44U),
    CORE_LAYER_FLAG_BIT_45 = (1ULL << 45U),
    CORE_LAYER_FLAG_BIT_46 = (1ULL << 46U),
    CORE_LAYER_FLAG_BIT_47 = (1ULL << 47U),
    CORE_LAYER_FLAG_BIT_48 = (1ULL << 48U),
    CORE_LAYER_FLAG_BIT_49 = (1ULL << 49U),
    CORE_LAYER_FLAG_BIT_50 = (1ULL << 50U),
    CORE_LAYER_FLAG_BIT_51 = (1ULL << 51U),
    CORE_LAYER_FLAG_BIT_52 = (1ULL << 52U),
    CORE_LAYER_FLAG_BIT_53 = (1ULL << 53U),
    CORE_LAYER_FLAG_BIT_54 = (1ULL << 54U),
    CORE_LAYER_FLAG_BIT_55 = (1ULL << 55U),
    CORE_LAYER_FLAG_BIT_56 = (1ULL << 56U),
    CORE_LAYER_FLAG_BIT_57 = (1ULL << 57U),
    CORE_LAYER_FLAG_BIT_58 = (1ULL << 58U),
    CORE_LAYER_FLAG_BIT_59 = (1ULL << 59U),
    CORE_LAYER_FLAG_BIT_60 = (1ULL << 60U),
    CORE_LAYER_FLAG_BIT_61 = (1ULL << 61U),
    CORE_LAYER_FLAG_BIT_62 = (1ULL << 62U),
    CORE_LAYER_FLAG_BIT_63 = (1ULL << 63U),
    CORE_LAYER_FLAG_BIT_ALL = 0xFFFFFFFFffffffff,
};

/** Layer constants */
struct LayerConstants {
    /** Default layer mask */
    static constexpr uint64_t DEFAULT_LAYER_MASK { LayerFlagBits::CORE_LAYER_FLAG_BIT_00 };
    /** All/Everything layer mask */
    static constexpr uint64_t ALL_LAYER_MASK { LayerFlagBits::CORE_LAYER_FLAG_BIT_ALL };
    /** None layer mask */
    static constexpr uint64_t NONE_LAYER_MASK { LayerFlagBits::CORE_LAYER_FLAG_BIT_NONE };
};
CORE3D_END_NAMESPACE()

#endif // API_3D_ECS_COMPONENTS_LAYER_DEFINES_H
