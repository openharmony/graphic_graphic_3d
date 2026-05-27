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

#ifndef CORE3D_RENDER_DATA_STORE_LIGHT_PROBE_H
#define CORE3D_RENDER_DATA_STORE_LIGHT_PROBE_H

#include <cstdint>
#include <util/light_probe_util.h>

#include <3d/light_probe_types/light_probe.h>
#include <3d/light_probe_types/light_probe_constants.h>
#include <3d/render/intf_render_data_store_light_probe.h>
#include <base/containers/array_view.h>
#include <base/containers/atomics.h>
#include <base/containers/refcnt_ptr.h>
#include <base/containers/string.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/util/uid.h>

RENDER_BEGIN_NAMESPACE()
class IRenderContext;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class RenderDataStoreLightProbe final : public IRenderDataStoreLightProbe {
public:
    explicit RenderDataStoreLightProbe(const BASE_NS::string_view name);
    ~RenderDataStoreLightProbe() override = default;

    // IRenderDataStore
    void PreRender() override{};
    // clear in post render
    void PostRender() override{};
    void PreRenderBackend() override
    {}
    void PostRenderBackend() override
    {}
    void Clear() override{};
    uint32_t GetFlags() const override
    {
        return 0;
    }

    BASE_NS::string_view GetTypeName() const override
    {
        return TYPE_NAME;
    }

    BASE_NS::string_view GetName() const override
    {
        return name_;
    }

    const BASE_NS::Uid& GetUid() const override
    {
        return UID;
    }

    void Ref() override;
    void Unref() override;
    int32_t GetRefCount() override;

    void AddLightProbes(
        uint64_t lightProbeId, BASE_NS::array_view<const LightProbeGroupComponent::LightProbe> lightProbes) override;
    void RemoveLightProbes(uint64_t lightProbeId) override;
    bool BuildTetrahedralMesh(uint64_t lightProbeId) override;

    LightProbeVolumeOpt GetLightProbeVolume(uint64_t lightProbeEntity) const override;
    static constexpr const char* const TYPE_NAME = "RenderDataStoreLightProbe";
    static BASE_NS::refcnt_ptr<RENDER_NS::IRenderDataStore> Create(
        RENDER_NS::IRenderContext& renderContext, const char* name);

private:
    const BASE_NS::string name_;
    struct LightProbeVolumeBindTetVec {
        LightProbeVolume volume;
        BASE_NS::vector<Tetrahedron> tetVec;
    };

    BASE_NS::unordered_map<uint64_t, LightProbeVolumeBindTetVec> lightProbeVolume_;

    int32_t refcnt_{0};
};
CORE3D_END_NAMESPACE()

#endif  // CORE3D_RENDER_DATA_STORE_LIGHT_PROBE_H
