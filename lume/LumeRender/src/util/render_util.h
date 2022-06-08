/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef RENDER_UTIL_RENDER_UTIL_H
#define RENDER_UTIL_RENDER_UTIL_H

#include <render/util/intf_render_util.h>

RENDER_BEGIN_NAMESPACE()
class IRenderContext;

class RenderUtil : public IRenderUtil {
public:
    explicit RenderUtil(const IRenderContext& renderContext);
    ~RenderUtil() override = default;

    RenderHandleDesc GetRenderHandleDesc(const RenderHandleReference& handle) const override;
    RenderHandleReference GetRenderHandle(const RenderHandleDesc& desc) const override;

    void SetRenderTimings(const RenderTimings::Times& times);
    RenderTimings GetRenderTimings() const override;

private:
    const IRenderContext& renderContext_;

    RenderTimings renderTimings_;
};
RENDER_END_NAMESPACE()

#endif // RENDER_UTIL_RENDER_UTIL_H
