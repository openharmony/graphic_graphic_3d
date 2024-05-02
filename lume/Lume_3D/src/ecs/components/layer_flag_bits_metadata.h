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

#if !defined(LAYER_FLAG_BITS_METADATA) || defined(IMPLEMENT_MANAGER)
#define LAYER_FLAG_BITS_METADATA

#include <PropertyTools/core_metadata.inl>

#include <3d/ecs/components/layer_defines.h>

CORE_BEGIN_NAMESPACE()
BEGIN_ENUM(LayerFlagBitsMetaData, CORE3D_NS::LayerFlagBits)
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_00, "Layer 0")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_01, "Layer 1")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_02, "Layer 2")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_03, "Layer 3")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_04, "Layer 4")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_05, "Layer 5")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_06, "Layer 6")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_07, "Layer 7")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_08, "Layer 8")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_09, "Layer 9")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_10, "Layer 10")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_11, "Layer 11")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_12, "Layer 12")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_13, "Layer 13")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_14, "Layer 14")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_15, "Layer 15")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_16, "Layer 16")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_17, "Layer 17")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_18, "Layer 18")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_19, "Layer 19")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_20, "Layer 20")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_21, "Layer 21")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_22, "Layer 22")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_23, "Layer 23")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_24, "Layer 24")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_25, "Layer 25")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_26, "Layer 26")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_27, "Layer 27")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_28, "Layer 28")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_29, "Layer 29")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_30, "Layer 30")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_31, "Layer 31")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_32, "Layer 32")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_33, "Layer 33")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_34, "Layer 34")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_35, "Layer 35")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_36, "Layer 36")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_37, "Layer 37")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_38, "Layer 38")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_39, "Layer 39")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_40, "Layer 40")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_41, "Layer 41")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_42, "Layer 42")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_43, "Layer 43")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_44, "Layer 44")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_45, "Layer 45")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_46, "Layer 46")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_47, "Layer 47")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_48, "Layer 48")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_49, "Layer 49")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_50, "Layer 50")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_51, "Layer 51")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_52, "Layer 52")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_53, "Layer 53")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_54, "Layer 54")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_55, "Layer 55")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_56, "Layer 56")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_57, "Layer 57")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_58, "Layer 58")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_59, "Layer 59")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_60, "Layer 60")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_61, "Layer 61")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_62, "Layer 62")
DECL_ENUM(CORE3D_NS::LayerFlagBits, CORE_LAYER_FLAG_BIT_63, "Layer 63")
END_ENUM(LayerFlagBitsMetaData, CORE3D_NS::LayerFlagBits)
CORE_END_NAMESPACE()
#endif // LAYER_FLAG_BITS_METADATA
