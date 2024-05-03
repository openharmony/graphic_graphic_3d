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
