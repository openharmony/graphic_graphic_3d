/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef LOADER_RENDER_NODE_GRAPH_LOADER_H
#define LOADER_RENDER_NODE_GRAPH_LOADER_H

#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <core/namespace.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>

CORE_BEGIN_NAMESPACE()
class IFileManager;
CORE_END_NAMESPACE()
RENDER_BEGIN_NAMESPACE()
class RenderNodeGraphLoader final : public IRenderNodeGraphLoader {
public:
    explicit RenderNodeGraphLoader(CORE_NS::IFileManager&);
    ~RenderNodeGraphLoader() override = default;

    LoadResult Load(const BASE_NS::string_view uri) override;
    LoadResult LoadString(const BASE_NS::string_view jsonString) override;

private:
    LoadResult LoadString(const BASE_NS::string_view uri, const BASE_NS::string_view jsonString);
    CORE_NS::IFileManager& fileManager_;
};
RENDER_END_NAMESPACE()

#endif // LOADER_RENDER_NODE_GRAPH_LOADER_H
