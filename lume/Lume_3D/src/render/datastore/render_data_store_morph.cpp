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
    submeshes_.push_back(submesh);
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
