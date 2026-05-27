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

#include "bowyer_watson_delaunay_3d.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <util/log.h>

#include <3d/light_probe_types/light_probe_constants.h>

CORE3D_BEGIN_NAMESPACE()

void BowyerWatsonDelaunay3D::BuildTetrahedralMesh(LightProbeVolume& lightProbeVolume)
{
    if (lightProbeVolume.lightProbes.size() < 4) {
        PLUGIN_LOG_E("BowyerWatson: Need at least 4 light probes for tetrahedral mesh");
        return;
    }
    // Using uint32_t indices and adding a few points for the super tetrahedron
    constexpr uint32_t maxProbes = 0xFFFFffffu / 2u;
    if (lightProbeVolume.lightProbes.size() > maxProbes) {
        PLUGIN_LOG_E("BowyerWatson: Too many light probes for tetrahedral mesh");
        return;
    }

    lightProbeVolume.tetrahedrons.clear();
    allPoints_.clear();

    for (const auto& probe : lightProbeVolume.lightProbes) {
        allPoints_.push_back(probe.position);
    }

    CreateSuperTetrahedron(lightProbeVolume);

    for (uint32_t i = 0; i < static_cast<uint32_t>(lightProbeVolume.lightProbes.size()); ++i) {
        InsertPoint(i, lightProbeVolume);
    }
    RemoveSuperTetrahedron(lightProbeVolume);
    allPoints_.clear();
    PLUGIN_LOG_I("BowyerWatson: Generated %zu tetrahedra from %zu light probes",
        lightProbeVolume.tetrahedrons.size(),
        lightProbeVolume.lightProbes.size());
}

void BowyerWatsonDelaunay3D::CreateSuperTetrahedron(LightProbeVolume& lightProbeVolume)
{
    BASE_NS::Math::Vec3 minP{
        std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    BASE_NS::Math::Vec3 maxP{std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest()};

    for (const auto& p : allPoints_) {
        minP.x = std::min(minP.x, p.x);
        minP.y = std::min(minP.y, p.y);
        minP.z = std::min(minP.z, p.z);
        maxP.x = std::max(maxP.x, p.x);
        maxP.y = std::max(maxP.y, p.y);
        maxP.z = std::max(maxP.z, p.z);
    }

    const BASE_NS::Math::Vec3 center = (minP + maxP) * 0.5f;
    const float maxRange = std::max({maxP.x - minP.x, maxP.y - minP.y, maxP.z - minP.z});
    const float scale = std::max(maxRange * 2.0f, 1.0f);

    Tetrahedron superTet;
    superTet.vertices[0] = {center.x - scale, center.y - scale, center.z - scale};
    superTet.vertices[1] = {center.x + scale * 3.0f, center.y - scale, center.z - scale};
    superTet.vertices[2] = {center.x - scale, center.y + scale * 3.0f, center.z - scale};
    superTet.vertices[3] = {center.x - scale, center.y - scale, center.z + scale * 3.0f};

    superTetrahedronStartIndex_ = static_cast<uint32_t>(allPoints_.size());

    PLUGIN_ASSERT(sizeof(superTet.indices) / sizeof(superTet.indices[0]) ==
                  sizeof(superTet.vertices) / sizeof(superTet.vertices[0]));
    for (auto i = 0u; i < sizeof(superTet.indices) / sizeof(superTet.indices[0]); i++) {
        superTet.indices[i] = superTetrahedronStartIndex_ + i;
        allPoints_.push_back(superTet.vertices[i]);
    }

    ComputeCircumsphere(superTet, lightProbeVolume);
    lightProbeVolume.tetrahedrons.push_back(superTet);
}

void BowyerWatsonDelaunay3D::InsertPoint(uint32_t pointIndex, LightProbeVolume& lightProbeVolume)
{
    const BASE_NS::Math::Vec3& point = allPoints_[pointIndex];

    BASE_NS::vector<uint32_t> badTetrahedra;

    for (uint32_t i = 0; i < static_cast<uint32_t>(lightProbeVolume.tetrahedrons.size()); ++i) {
        float distSquared = BASE_NS::Math::Distance2(point, lightProbeVolume.tetrahedrons[i].circumcenter);
        float circumRadiusSquared = lightProbeVolume.tetrahedrons[i].circumradiusSquared;
        float tolerance = std::max(circumRadiusSquared * 1e-4f, 1e-5f);

        if (distSquared <= circumRadiusSquared + tolerance) {
            badTetrahedra.push_back(i);
        }
    }

    if (badTetrahedra.empty()) {
        return;
    }

    std::map<std::array<uint32_t, 3>, FaceData> uniqueFaces;
    std::map<std::array<uint32_t, 3>, int> faceCounts;

    for (uint32_t tetIndex : badTetrahedra) {
        const Tetrahedron& tet = lightProbeVolume.tetrahedrons[tetIndex];
        FaceData faces[4] = {
            {{tet.indices[1], tet.indices[2], tet.indices[3]}, {tet.vertices[1], tet.vertices[2], tet.vertices[3]}},
            {{tet.indices[0], tet.indices[2], tet.indices[3]}, {tet.vertices[0], tet.vertices[2], tet.vertices[3]}},
            {{tet.indices[0], tet.indices[1], tet.indices[3]}, {tet.vertices[0], tet.vertices[1], tet.vertices[3]}},
            {{tet.indices[0], tet.indices[1], tet.indices[2]}, {tet.vertices[0], tet.vertices[1], tet.vertices[2]}},
        };

        for (const auto& face : faces) {
            std::array<uint32_t, LightProbeConstants::VERTICES_PER_FACE_COUNT> sortedIndices = {
                face.indices[0], face.indices[1], face.indices[2]};
            std::sort(sortedIndices.begin(), sortedIndices.end());

            faceCounts[sortedIndices]++;
            if (faceCounts[sortedIndices] == 1) {
                uniqueFaces[sortedIndices] = face;
            }
        }
    }

    for (const auto& pair : faceCounts) {
        if (pair.second == 1) {
            const FaceData& face = uniqueFaces[pair.first];
            Tetrahedron newTet;

            newTet.indices[0] = face.indices[0];
            newTet.indices[1] = face.indices[1];
            newTet.indices[2] = face.indices[2];
            newTet.indices[3] = pointIndex;

            newTet.vertices[0] = face.vertices[0];
            newTet.vertices[1] = face.vertices[1];
            newTet.vertices[2] = face.vertices[2];
            newTet.vertices[3] = point;

            ComputeCircumsphere(newTet, lightProbeVolume);
            lightProbeVolume.tetrahedrons.push_back(newTet);
        }
    }

    std::sort(badTetrahedra.rbegin(), badTetrahedra.rend());
    for (uint32_t tetIndex : badTetrahedra) {
        if (tetIndex < static_cast<uint32_t>(lightProbeVolume.tetrahedrons.size())) {
            lightProbeVolume.tetrahedrons.erase(lightProbeVolume.tetrahedrons.begin() + tetIndex);
        }
    }
}

void BowyerWatsonDelaunay3D::ComputeCircumsphere(Tetrahedron& tet, const LightProbeVolume& lightProbeVolume)
{
    const BASE_NS::Math::Vec3& a = tet.vertices[0];
    const BASE_NS::Math::Vec3& b = tet.vertices[1];
    const BASE_NS::Math::Vec3& c = tet.vertices[2];
    const BASE_NS::Math::Vec3& d = tet.vertices[3];

    BASE_NS::Math::Vec3 ba = b - a;
    BASE_NS::Math::Vec3 ca = c - a;
    BASE_NS::Math::Vec3 da = d - a;

    float baLenSq = SqrMagnitude(ba);
    float caLenSq = SqrMagnitude(ca);
    float daLenSq = SqrMagnitude(da);

    BASE_NS::Math::Vec3 crossCA_DA =
        BASE_NS::Math::Vec3(ca.y * da.z - ca.z * da.y, ca.z * da.x - ca.x * da.z, ca.x * da.y - ca.y * da.x);

    BASE_NS::Math::Vec3 crossDA_BA =
        BASE_NS::Math::Vec3(da.y * ba.z - da.z * ba.y, da.z * ba.x - da.x * ba.z, da.x * ba.y - da.y * ba.x);

    BASE_NS::Math::Vec3 crossBA_CA =
        BASE_NS::Math::Vec3(ba.y * ca.z - ba.z * ca.y, ba.z * ca.x - ba.x * ca.z, ba.x * ca.y - ba.y * ca.x);

    float denominator = 2.0f * (ba.x * crossCA_DA.x + ba.y * crossCA_DA.y + ba.z * crossCA_DA.z);

    float maxLenSq = std::max({baLenSq, caLenSq, daLenSq});
    float tolerance = std::max(maxLenSq * 1e-6f, 1e-10f);

    if (std::abs(denominator) < tolerance) {
        tet.circumcenter = BASE_NS::Math::Vec3(0, 0, 0);
        tet.circumradiusSquared = std::numeric_limits<float>::max();
        return;
    }

    BASE_NS::Math::Vec3 numerator = crossCA_DA * baLenSq + crossDA_BA * caLenSq + crossBA_CA * daLenSq;

    tet.circumcenter = a + numerator * (1.0f / denominator);
    tet.circumradiusSquared = BASE_NS::Math::SqrMagnitude(tet.circumcenter - a);
}

void BowyerWatsonDelaunay3D::RemoveSuperTetrahedron(LightProbeVolume& lightProbeVolume)
{
    BASE_NS::vector<Tetrahedron> validTetrahedra;

    for (const auto& tet : lightProbeVolume.tetrahedrons) {
        bool hasSuperVertex = false;
        for (uint32_t i = 0U; i < LightProbeConstants::TETRAHEDRON_LIGHT_PROBE_COUNT; ++i) {
            if (tet.indices[i] >= superTetrahedronStartIndex_) {
                hasSuperVertex = true;
                break;
            }
        }

        if (!hasSuperVertex) {
            validTetrahedra.push_back(tet);
        }
    }

    lightProbeVolume.tetrahedrons = validTetrahedra;
}

CORE3D_END_NAMESPACE()
