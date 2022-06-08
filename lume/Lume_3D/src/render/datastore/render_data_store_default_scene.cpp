/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "render_data_store_default_scene.h"

#include <cstdint>

#include <3d/render/intf_render_data_store_default_scene.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace RENDER_NS;

RenderDataStoreDefaultScene::RenderDataStoreDefaultScene(const string_view name) : name_(name) {}

void RenderDataStoreDefaultScene::PostRender()
{
    Clear();
}

void RenderDataStoreDefaultScene::Clear()
{
    scenes_.clear();
    nameToScene_.clear();
    nextId = 0;
}

void RenderDataStoreDefaultScene::SetScene(const RenderScene& renderScene)
{
    const size_t arrIdx = scenes_.size();
    scenes_.emplace_back(renderScene);
    if (renderScene.name.empty()) {
        nameToScene_[to_string(nextId++)] = arrIdx;
    } else {
        nameToScene_[renderScene.name] = arrIdx;
    }
}

RenderScene RenderDataStoreDefaultScene::GetScene(const string_view name) const
{
    if (const auto iter = nameToScene_.find(name); iter != nameToScene_.cend()) {
        CORE_ASSERT(iter->second < scenes_.size());
        return scenes_[iter->second];
    } else {
        return {};
    }
}

RenderScene RenderDataStoreDefaultScene::GetScene() const
{
    if (!scenes_.empty()) {
        return scenes_[0];
    } else {
        return {};
    }
}

// for plugin / factory interface
RENDER_NS::IRenderDataStore* RenderDataStoreDefaultScene::Create(RENDER_NS::IRenderContext&, char const* name)
{
    // device not needed
    return new RenderDataStoreDefaultScene(name);
}

void RenderDataStoreDefaultScene::Destroy(IRenderDataStore* instance)
{
    delete static_cast<RenderDataStoreDefaultScene*>(instance);
}
CORE3D_END_NAMESPACE()
