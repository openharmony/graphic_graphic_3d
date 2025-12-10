/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef API_RENDER_IRENDER_NODE_POST_PROCESS_INTERFACE_UTIL_H
#define API_RENDER_IRENDER_NODE_POST_PROCESS_INTERFACE_UTIL_H

#include <cstdint>

#include <base/util/uid.h>
#include <core/plugin/intf_interface.h>
#include <render/device/pipeline_state_desc.h>
#include <render/render_data_structures.h>

RENDER_BEGIN_NAMESPACE()
class IRenderNodeContextManager;
class IRenderCommandList;

/**
 * IRenderNodePostProcessInterfaceUtil.
 * Helper class for combined post process rendering with post process interfaces.
 * Creation:
 * CORE_NS::CreateInstance<IRenderNodePostProcessInterfaceUtil>(renderContext,
 * UID_RENDER_NODE_POST_PROCESS_INTERFACE_UTIL);
 */
class IRenderNodePostProcessInterfaceUtil : public CORE_NS::IInterface {
public:
    static constexpr BASE_NS::Uid UID { "1c811e6c-a427-4849-930b-5130da61706c" };

    using Ptr = BASE_NS::refcnt_ptr<IRenderNodePostProcessInterfaceUtil>;

    /**
     * Image data is filled automatically by the class if data is not given with info
     */
    struct ImageData {
        /** Global input */
        BindableImage input;
        BindableImage inputUpscaled;
        /** Global output */
        BindableImage output;
    };
    /**
     * Info
     * Data store prefix name
     */
    struct Info {
        /** Data store prefix which will be loaded */
        BASE_NS::string_view dataStorePrefix;
        /**/
        ImageData imageData;
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
    virtual void Execute(IRenderCommandList& cmdList) = 0;

    /**
     * Set info. Set additional info for rendering
     */
    virtual void SetInfo(const Info& info) = 0;

protected:
    IRenderNodePostProcessInterfaceUtil() = default;
    virtual ~IRenderNodePostProcessInterfaceUtil() = default;
};

inline constexpr BASE_NS::string_view GetName(const IRenderNodePostProcessInterfaceUtil*)
{
    return "IRenderNodePostProcessInterfaceUtil";
}
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_NODE_POST_PROCESS_INTERFACE_UTIL_H
