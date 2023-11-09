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

#ifndef API_RENDER_IRENDER_NODE_INTERFACE_H
#define API_RENDER_IRENDER_NODE_INTERFACE_H

#include <base/containers/string_view.h>
#include <base/util/uid.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
/** @ingroup group_render_irendernodeinterface */
/**
 * Base interface for render node interfaces.
 */
class IRenderNodeInterface {
public:
    /** Get the type name of the render node interface.
     */
    virtual BASE_NS::string_view GetTypeName() const = 0;

    /** Get Uid of the interface.
     */
    virtual BASE_NS::Uid GetUid() const = 0;

protected:
    IRenderNodeInterface() = default;
    virtual ~IRenderNodeInterface() = default;

    IRenderNodeInterface(const IRenderNodeInterface&) = delete;
    IRenderNodeInterface& operator=(const IRenderNodeInterface&) = delete;
    IRenderNodeInterface(IRenderNodeInterface&&) = delete;
    IRenderNodeInterface& operator=(IRenderNodeInterface&&) = delete;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_NODE_INTERFACE_H
