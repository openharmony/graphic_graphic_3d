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

#ifndef API_3D_RENDER_IRENDER_DATA_STORE_LIGHT_PROBE_H
#define API_3D_RENDER_IRENDER_DATA_STORE_LIGHT_PROBE_H

#include <cstdint>

#include <3d/namespace.h>
#include <base/containers/array_view.h>
#include <base/math/vector.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <render/datastore/intf_render_data_store.h>

CORE3D_BEGIN_NAMESPACE()
class IRenderDataStoreLightProbe : public RENDER_NS::IRenderDataStore {
public:
    static constexpr BASE_NS::Uid UID{"94d19082-a7fa-487d-b585-7969335b4056"};

    virtual void AddLightProbes(
        uint64_t lightProbeId, BASE_NS::array_view<const LightProbeGroupComponent::LightProbe> lightProbes) = 0;
    virtual void RemoveLightProbes(uint64_t lightProbeId) = 0;
    virtual bool BuildTetrahedralMesh(uint64_t lightProbeId) = 0;
    virtual LightProbeVolumeOpt GetLightProbeVolume(uint64_t lightProbeEntity) const = 0;
    ~IRenderDataStoreLightProbe() override = default;

protected:
    IRenderDataStoreLightProbe() = default;
};
CORE3D_END_NAMESPACE()

#endif  // API_3D_RENDER_IRENDER_DATA_STORE_LIGHT_PROBE_H
