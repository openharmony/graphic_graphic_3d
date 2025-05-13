/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef RENDER_POSTPROCESS_RENDER_POSTPROCESS_UPSCALE_H
#define RENDER_POSTPROCESS_RENDER_POSTPROCESS_UPSCALE_H

#include <base/math/vector.h>
#include <base/util/uid.h>
#include <core/plugin/intf_interface.h>
#include <core/plugin/intf_interface_helper.h>
#include <core/property/intf_property_handle.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/nodecontext/intf_render_post_process.h>
#include <render/nodecontext/intf_render_post_process_node.h>

#include "render_post_process_upscale_node.h"

RENDER_BEGIN_NAMESPACE()
class RenderPostProcessUpscale : public CORE_NS::IInterfaceHelper<RENDER_NS::IRenderPostProcess> {
public:
    static constexpr auto UID = BASE_NS::Uid("b16c7e85-1c1d-4d6c-809e-afdf9978071b");

    RenderPostProcessUpscale();
    ~RenderPostProcessUpscale() override = default;

    CORE_NS::IPropertyHandle* GetProperties() override;

    BASE_NS::Uid GetRenderPostProcessNodeUid() override
    {
        return RenderPostProcessUpscaleNode::UID;
    }

    void SetData(BASE_NS::array_view<const uint8_t> data) override;
    BASE_NS::array_view<const uint8_t> GetData() const override;

    // direct access in render nodes
    RenderPostProcessUpscaleNode::EffectProperties propertiesData;

private:
    CORE_NS::PropertyApiImpl<RenderPostProcessUpscaleNode::EffectProperties> properties_;
};
RENDER_END_NAMESPACE()

#endif // RENDER_POSTPROCESS_RENDER_POSTPROCESS_UPSCALE_H
