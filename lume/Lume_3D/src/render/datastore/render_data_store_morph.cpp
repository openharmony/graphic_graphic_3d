/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "render_data_store_morph.h"

#include <cstdint>

#include <3d/render/intf_render_data_store_morph.h>
#include <base/containers/array_view.h>
#include <render/resource_handle.h>

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace RENDER_NS;

RenderDataStoreMorph::RenderDataStoreMorph(const string_view name) : name_(name) {}

void RenderDataStoreMorph::Init(const IRenderDataStoreMorph::ReserveSize& reserveSize)
{
    submeshes_.reserve(reserveSize.submeshCount);
}

void RenderDataStoreMorph::PostRender()
{
    Clear();
}

void RenderDataStoreMorph::Clear()
{
    submeshes_.clear();
}

void RenderDataStoreMorph::AddSubmesh(const RenderDataMorph::Submesh& submesh)
{
    submeshes_.emplace_back(submesh);
}

array_view<const RenderDataMorph::Submesh> RenderDataStoreMorph::GetSubmeshes() const
{
    return submeshes_;
}

// for plugin / factory interface
RENDER_NS::IRenderDataStore* RenderDataStoreMorph::Create(RENDER_NS::IRenderContext&, char const* name)
{
    return new RenderDataStoreMorph(name);
}

void RenderDataStoreMorph::Destroy(IRenderDataStore* instance)
{
    delete static_cast<RenderDataStoreMorph*>(instance);
}
CORE3D_END_NAMESPACE()
