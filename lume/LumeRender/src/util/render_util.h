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

#ifndef RENDER_UTIL_RENDER_UTIL_H
#define RENDER_UTIL_RENDER_UTIL_H

#include <base/containers/unique_ptr.h>
#include <render/util/intf_render_util.h>

#include "util/render_frame_util.h"

RENDER_BEGIN_NAMESPACE()
class IRenderContext;

class RenderUtil : public IRenderUtil {
public:
    explicit RenderUtil(const IRenderContext& renderContext);
    ~RenderUtil() override = default;

    void BeginFrame();
    void EndFrame();

    RenderHandleDesc GetRenderHandleDesc(const RenderHandleReference& handle) const override;
    RenderHandleReference GetRenderHandle(const RenderHandleDesc& desc) const override;

    void SetRenderTimings(const RenderTimings::Times& times);
    RenderTimings GetRenderTimings() const override;

    IRenderFrameUtil& GetRenderFrameUtil() const override;

private:
    const IRenderContext& renderContext_;
    BASE_NS::unique_ptr<RenderFrameUtil> renderFrameUtil_;

    RenderTimings renderTimings_;
};
RENDER_END_NAMESPACE()

#endif // RENDER_UTIL_RENDER_UTIL_H
