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
#ifndef API_3D_LIGHT_PROBE_TYPES_LIGHT_PROBE_CONSTANTS_H
#define API_3D_LIGHT_PROBE_TYPES_LIGHT_PROBE_CONSTANTS_H

#include <cstdint>

#include <3d/namespace.h>

CORE3D_BEGIN_NAMESPACE()
struct LightProbeConstants {
    static constexpr float EPSILON{1e-6f};
    static constexpr uint32_t LIGHT_PROBE_SH_COEFFICIENT_COUNT{9};
    static constexpr uint32_t TETRAHEDRON_LIGHT_PROBE_COUNT{4};
    static constexpr uint32_t TETRAHEDRON_FACE_COUNT{4};
    static constexpr uint32_t VERTICES_PER_FACE_COUNT{3u};

    static constexpr uint32_t SSBO_LIGHT_PROBE_VEC4_COUNT{4};
    static constexpr uint32_t SSBO_LIGHT_PROBE_BENTNORMAL_VEC3_COUNT{3};
    static constexpr uint32_t SSBO_LIGHT_PROBE_AO_FLOAT_COUNT{1};
    static constexpr uint32_t SSBO_SINGLE_LIGHT_PROBE_DATA_SIZE{
        sizeof(float) * (SSBO_LIGHT_PROBE_VEC4_COUNT * LIGHT_PROBE_SH_COEFFICIENT_COUNT +
                            SSBO_LIGHT_PROBE_BENTNORMAL_VEC3_COUNT + SSBO_LIGHT_PROBE_AO_FLOAT_COUNT)};
};

CORE3D_END_NAMESPACE()
#endif  // API_3D_LIGHT_PROBE_TYPES_LIGHT_PROBE_CONSTANTS_H