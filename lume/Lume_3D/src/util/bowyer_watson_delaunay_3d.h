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

#ifndef CORE_UTIL_BOWYER_WATSON_DELAUNAY_3D_H
#define CORE_UTIL_BOWYER_WATSON_DELAUNAY_3D_H

#include <algorithm>
#include <array>
#include <functional>

#include <3d/light_probe_types/light_probe.h>
#include <3d/light_probe_types/light_probe_constants.h>
#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <core/namespace.h>

CORE3D_BEGIN_NAMESPACE()

class BowyerWatsonDelaunay3D {
public:
    struct LightProbeVolume {
        BASE_NS::array_view<const LightProbeGroupComponent::LightProbe> lightProbes;
        BASE_NS::vector<Tetrahedron>& tetrahedrons;
    };
    BowyerWatsonDelaunay3D() = default;
    ~BowyerWatsonDelaunay3D() = default;

    void BuildTetrahedralMesh(LightProbeVolume& lightProbeVolume);

private:
    struct FaceData {
        uint32_t indices[3];
        BASE_NS::Math::Vec3 vertices[3];
    };

    void CreateSuperTetrahedron(LightProbeVolume& lightProbeVolume);

    void InsertPoint(uint32_t pointIndex, LightProbeVolume& lightProbeVolume);

    void ComputeCircumsphere(Tetrahedron& tet, const LightProbeVolume& lightProbeVolume);
    void RemoveSuperTetrahedron(LightProbeVolume& lightProbeVolume);

    uint32_t superTetrahedronStartIndex_ = 0;
    BASE_NS::vector<BASE_NS::Math::Vec3> allPoints_;
};

CORE3D_END_NAMESPACE()

#endif  // CORE_UTIL_BOWYER_WATSON_DELAUNAY_3D_H
