/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef RENDER_RENDER__NODE__CORE_RENDER_NODE_FACTORY_H
#define RENDER_RENDER__NODE__CORE_RENDER_NODE_FACTORY_H

#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
class RenderNodeManager;

void RegisterCoreRenderNodes(RenderNodeManager& renderNodeManager);
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__NODE__CORE_RENDER_NODE_FACTORY_H
