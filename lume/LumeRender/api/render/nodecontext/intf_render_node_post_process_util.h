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

#ifndef API_RENDER_IRENDER_NODE_POST_PROCESS_UTIL_H
#define API_RENDER_IRENDER_NODE_POST_PROCESS_UTIL_H

#include <cstdint>

#include <base/containers/vector.h>
#include <base/util/uid.h>
#include <core/plugin/intf_interface.h>
#include <render/device/pipeline_state_desc.h>
#include <render/render_data_structures.h>

RENDER_BEGIN_NAMESPACE()
class IRenderNodeContextManager;
class IRenderCommandList;

/**
 * IRenderNodePostProcessUtil.
 * Helper class for combined post process rendering.
 * Creation:
 * CORE_NS::CreateInstance<IRenderNodePostProcessUtil>(renderContext, UID_RENDER_NODE_POST_PROCESS_UTIL);
 */
class IRenderNodePostProcessUtil : public CORE_NS::IInterface {
public:
    static constexpr BASE_NS::Uid UID { "84057a3c-0e0f-4752-84af-55591f746ceb" };

    using Ptr = BASE_NS::refcnt_ptr<IRenderNodePostProcessUtil>;

    /**
     * Image data is filled automatically by the class if data is not given and it's needed
     * If parsing is true, the resources are parsed from render node inputs
     */
    struct ImageData {
        /** Global input */
        BindableImage input;
        /** Global output */
        BindableImage output;

        /** Depth */
        BindableImage depth;
        /** Velocity */
        BindableImage velocity;

        /** History image */
        BindableImage history;
        /** History next image */
        BindableImage historyNext;

        /** Dirtmask */
        RenderHandle dirtMask;
    };

    struct PostProcessInfo {
        /** Parse render node resource inputs */
        bool parseRenderNodeInputs { true };
        /** Input image data */
        ImageData imageData;
    };

    /**
     * Init method. Call in render node Init()
     */
    virtual void Init(IRenderNodeContextManager& renderNodeContextMgr, const PostProcessInfo& postProcessInfo) = 0;

    /**
     * PreExecute method. Call in render node PreExecute()
     */
    virtual void PreExecute(const PostProcessInfo& postProcessInfo) = 0;

    /**
     * Execute method. Call in render node Execute()
     */
    virtual void Execute(IRenderCommandList& cmdList) = 0;

protected:
    IRenderNodePostProcessUtil() = default;
    virtual ~IRenderNodePostProcessUtil() = default;
};

inline constexpr BASE_NS::string_view GetName(const IRenderNodePostProcessUtil*)
{
    return "IRenderNodePostProcessUtil";
}
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_NODE_POST_PROCESS_UTIL_H
