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

#ifndef API_RENDER_NODECONTEXT_IRENDER_POST_PROCESS_H
#define API_RENDER_NODECONTEXT_IRENDER_POST_PROCESS_H

#include <base/util/uid.h>
#include <core/plugin/intf_interface.h>
#include <core/property/intf_property_handle.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()

class IRenderNodeContextManager;
class IRenderCommandList;

/** @ingroup group_render_IRenderPostProcess */
/**
 * Provides interface to access post process properties.
 */
class IRenderPostProcess : public CORE_NS::IInterface {
public:
    static constexpr auto UID = BASE_NS::Uid("f0a2ac94-f117-4abe-a564-362b37251e26");

    using Ptr = BASE_NS::refcnt_ptr<IRenderPostProcess>;

    /** Get property handle for built-in properties for this effect. Check the pointer always.
     * These properties are global values which are usually set once or changed sparingly.
     * For example quality parameters for the post process.
     * @return Pointer to property handle if properties present, nullptr otherwise.
     */
    virtual CORE_NS::IPropertyHandle* GetProperties() = 0;

    /** Return UID of the render post process node implementation.
     * Based on this new instances are created per render node.
     * @return UID of the render post process node.
     */
    virtual BASE_NS::Uid GetRenderPostProcessNodeUid() = 0;

protected:
    IRenderPostProcess() = default;
    virtual ~IRenderPostProcess() = default;

    IRenderPostProcess(const IRenderPostProcess&) = delete;
    IRenderPostProcess& operator=(const IRenderPostProcess&) = delete;
    IRenderPostProcess(IRenderPostProcess&&) = delete;
    IRenderPostProcess& operator=(IRenderPostProcess&&) = delete;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_NODECONTEXT_IRENDER_POST_PROCESS_H
