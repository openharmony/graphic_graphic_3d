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

#include "render_data_store_light_probe.h"

#include <algorithm>
#include <cstdint>

#include <3d/render/default_material_constants.h>
#include <3d/render/render_data_defines_3d.h>

#include "util/bowyer_watson_delaunay_3d.h"
#include "util/light_probe_util.h"
#include "util/log.h"

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace RENDER_NS;

RenderDataStoreLightProbe::RenderDataStoreLightProbe(const string_view name) : name_(name)
{}

void RenderDataStoreLightProbe::Ref()
{
    AtomicIncrementRelaxed(&refcnt_);
}

void RenderDataStoreLightProbe::Unref()
{
    if (AtomicDecrementRelease(&refcnt_) == 0) {
        AtomicFenceAcquire();
        delete this;
    }
}

int32_t RenderDataStoreLightProbe::GetRefCount()
{
    return AtomicReadRelaxed(&refcnt_);
}

bool RenderDataStoreLightProbe::BuildTetrahedralMesh(uint64_t lightProbeId)
{
    auto l = lightProbeVolume_.find(lightProbeId);
    if (l == lightProbeVolume_.end()) {
        PLUGIN_LOG_W("Do not find light probe volume with id");
        return false;
    }

    BowyerWatsonDelaunay3D::LightProbeVolume bwLightProbeDelaunay{l->second.volume.lightProbes, l->second.tetVec};
    BowyerWatsonDelaunay3D delaunay3D;
    delaunay3D.BuildTetrahedralMesh(bwLightProbeDelaunay);
    l->second.volume.tetrahedrons = l->second.tetVec;
    return !l->second.volume.tetrahedrons.empty();
}

void RenderDataStoreLightProbe::AddLightProbes(
    uint64_t lightProbeId, BASE_NS::array_view<const LightProbeGroupComponent::LightProbe> lightProbes)
{
    auto l = lightProbeVolume_.find(lightProbeId);
    if (l != lightProbeVolume_.end()) {
        PLUGIN_LOG_W("override light probe with id");
        l->second.volume.lightProbes = lightProbes;
        l->second.volume.tetrahedrons = {};
        return;
    }

    lightProbeVolume_[lightProbeId].volume.lightProbes = lightProbes;
    lightProbeVolume_[lightProbeId].tetVec = {};
}

void RenderDataStoreLightProbe::RemoveLightProbes(uint64_t lightProbeId)
{
    auto l = lightProbeVolume_.find(lightProbeId);
    if (l != lightProbeVolume_.end()) {
        lightProbeVolume_.erase(l);
    } else {
        PLUGIN_LOG_W("light probe with id not found");
    }
}

LightProbeVolumeOpt RenderDataStoreLightProbe::GetLightProbeVolume(uint64_t lightProbeEntity) const
{
    LightProbeVolumeOpt result;
    auto it = lightProbeVolume_.find(lightProbeEntity);
    if (it != lightProbeVolume_.end()) {
        result.lightProbeVolume = it->second.volume;
        result.valid = true;
    }
    return result;
}

// for plugin / factory interface
refcnt_ptr<RENDER_NS::IRenderDataStore> RenderDataStoreLightProbe::Create(RENDER_NS::IRenderContext&, const char* name)
{
    return refcnt_ptr<IRenderDataStore>(new RenderDataStoreLightProbe(name));
}
CORE3D_END_NAMESPACE()
