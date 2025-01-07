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

#ifndef API_RENDER_IRENDER_NODE_COPY_UTIL_H
#define API_RENDER_IRENDER_NODE_COPY_UTIL_H

#include <cstdint>

#include <base/util/uid.h>
#include <core/plugin/intf_interface.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/namespace.h>
#include <render/render_data_structures.h>

RENDER_BEGIN_NAMESPACE()
class IRenderNodeContextManager;
class IRenderCommandList;

/**
 * IRenderNodeCopyUtil.
 * Helper class for copying images in render nodes.
 * Creation:
 * CORE_NS::CreateInstance<IRenderNodeCopyUtil>(renderContext, UID_RENDER_NODE_COPY_UTIL);
 */
class IRenderNodeCopyUtil : public CORE_NS::IInterface {
public:
    static constexpr BASE_NS::Uid UID { "cf855681-2446-42fc-bc6b-8c439871f8db" };

    using Ptr = BASE_NS::refcnt_ptr<IRenderNodeCopyUtil>;

    /** Copy type */
    enum class CopyType : uint32_t {
        /** Basic copy the 2D image */
        BASIC_COPY = 0,
        /** Layer copy */
        LAYER_COPY = 1,
    };

    /** Copy info */
    struct CopyInfo {
        /** Bindable input image */
        BindableImage input;
        /** The output where to copy */
        BindableImage output;
        /** Sampler (if not given linear clamp is used) */
        RenderHandle sampler;
        /** Copy type to be used */
        CopyType copyType { CopyType::BASIC_COPY };
    };

    /**
     * Init method. Call in render node Init()
     */
    virtual void Init(IRenderNodeContextManager& renderNodeContextMgr) = 0;

    /**
     * PreExecute method. Call in render node PreExecute()
     */
    virtual void PreExecute() = 0;

    /**
     * Execute method. Call in render node Execute()
     */
    virtual void Execute(IRenderCommandList& cmdList, const CopyInfo& copyInfo) = 0;

    /**
     * Get render descriptor counts
     */
    virtual DescriptorCounts GetRenderDescriptorCounts() const = 0;

protected:
    IRenderNodeCopyUtil() = default;
    virtual ~IRenderNodeCopyUtil() = default;
};

inline constexpr BASE_NS::string_view GetName(const IRenderNodeCopyUtil*)
{
    return "IRenderNodeCopyUtil";
}
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_NODE_COPY_UTIL_H
