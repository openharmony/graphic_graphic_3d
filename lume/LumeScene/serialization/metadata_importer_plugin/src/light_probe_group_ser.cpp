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

#include "light_probe_group_ser.h"

#include <meta/interface/serialization/intf_ser_node.h>

SCENE_MIMP_BEGIN_NAMESPACE()

constexpr uint32_t SH_COEFFICIENT_COUNT = 9;  // 9: number of coefficients in LightProbe
constexpr uint32_t NUM_VEC3 = 3;              // 3: number of floats in Vec3

META_NS::ReturnError LightProbeGroupSer::Export(META_NS::IExportContext& c) const
{
    auto lpg = interface_pointer_cast<SCENE_NS::ILightProbeGroup>(META_ACCESS_PROPERTY_VALUE(AttachedTo));
    if (!lpg) {
        return META_NS::GenericError::FAIL;
    }
    auto group = lpg->GetLightProbeGroup();
    // Flatten all probe data into parallel arrays of float values
    const auto count = group.size();
    BASE_NS::vector<float> positions;
    BASE_NS::vector<float> shCoeffs;
    BASE_NS::vector<float> bentNormals;
    BASE_NS::vector<float> aos;
    positions.reserve(count * NUM_VEC3);
    shCoeffs.reserve(count * SH_COEFFICIENT_COUNT * NUM_VEC3);
    bentNormals.reserve(count * NUM_VEC3);
    aos.reserve(count);

    auto storeVec3 = [](BASE_NS::vector<float>& vec, const BASE_NS::Math::Vec3& value) {
        vec.push_back(value.x);
        vec.push_back(value.y);
        vec.push_back(value.z);
    };

    for (const auto& probe : group) {
        storeVec3(positions, probe.position);
        for (size_t i = 0; i < SH_COEFFICIENT_COUNT; ++i) {
            storeVec3(shCoeffs, probe.shCoefficients[i]);
        }
        storeVec3(bentNormals, probe.bentNormal);
        aos.push_back(probe.ao);
    }
    c.ExportValue("probeCount", count);
    c.ExportValue("positions", positions);
    c.ExportValue("shCoefficients", shCoeffs);
    c.ExportValue("bentNormals", bentNormals);
    c.ExportValue("aos", aos);
    return META_NS::GenericError::SUCCESS;
}

META_NS::ReturnError LightProbeGroupSer::Import(META_NS::IImportContext& c)
{
    importedGroup_.reset();
    size_t count = 0;
    c.ImportValue("probeCount", count);
    if (count == 0) {
        // Either probeCount was not there or value was 0
        return META_NS::GenericError::SUCCESS;
    }
    BASE_NS::vector<float> positions;
    BASE_NS::vector<float> shCoeffs;
    BASE_NS::vector<float> bentNormals;
    BASE_NS::vector<float> aos;

    c.ImportValue("positions", positions);
    c.ImportValue("shCoefficients", shCoeffs);
    c.ImportValue("bentNormals", bentNormals);
    c.ImportValue("aos", aos);

    if (positions.size() != count * NUM_VEC3 || shCoeffs.size() != count * NUM_VEC3 * SH_COEFFICIENT_COUNT ||
        bentNormals.size() != count * NUM_VEC3 || aos.size() != count) {
        // Item count in one of the flat arrays doesn't match probeCount
        return META_NS::GenericError::FAIL;
    }

    auto importVec3 = [](const BASE_NS::vector<float>& vec, size_t pos) -> BASE_NS::Math::Vec3 {
        BASE_NS::Math::Vec3 value;
        value.x = vec[pos++];
        value.y = vec[pos++];
        value.z = vec[pos];
        return value;
    };

    SCENE_NS::LightProbeGroup group;
    group.resize(count);
    for (size_t i = 0; i < count; i++) {
        auto& probe = group[i];
        const auto floatVecIndex = i * 3;
        probe.position = importVec3(positions, floatVecIndex);
        for (size_t j = 0; j < SH_COEFFICIENT_COUNT; j++) {
            probe.shCoefficients[j] = importVec3(shCoeffs, floatVecIndex * SH_COEFFICIENT_COUNT + j * 3);
        }
        probe.bentNormal = importVec3(bentNormals, floatVecIndex);
        probe.ao = aos[i];
    }
    importedGroup_ = BASE_NS::move(group);
    return META_NS::GenericError::SUCCESS;
}

bool LightProbeGroupSer::AttachTo(const META_NS::IAttach::Ptr& target, const META_NS::IObject::Ptr&)
{
    if (importedGroup_) {
        if (auto lpg = interface_cast<SCENE_NS::ILightProbeGroup>(target)) {
            lpg->SetLightProbeGroup(*importedGroup_);
        }
        importedGroup_.reset();
    }
    return true;
}

bool LightProbeGroupSer::DetachFrom(const META_NS::IAttach::Ptr&)
{
    return true;
}

SCENE_MIMP_END_NAMESPACE()
