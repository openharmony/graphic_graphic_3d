/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
