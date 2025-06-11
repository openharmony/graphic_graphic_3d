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

#include "mesh_creator.h"

#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/util.h>

#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/implementation_uids.h>
#include <3d/render/default_material_constants.h>
#include <3d/util/intf_mesh_builder.h>
#include <3d/util/intf_mesh_util.h>
#include <core/plugin/intf_class_factory.h>

SCENE_BEGIN_NAMESPACE()

bool MeshCreator::Build(const META_NS::IMetadata::Ptr& d)
{
    IInternalScene::Ptr p;
    if (Super::Build(d)) {
        p = GetInterfaceBuildArg<IInternalScene>(d, "Scene");
        scene_ = p;
    }
    return p != nullptr;
}

static CORE_NS::Entity GetMaterial(const MeshConfig& c)
{
    if (auto i = interface_cast<IEcsObjectAccess>(c.material)) {
        if (auto obj = i->GetEcsObject()) {
            return obj->GetEntity();
        }
    }
    return {};
}

static IMesh::Ptr CreateMesh(const IInternalScene::Ptr& scene, CORE_NS::Entity ent)
{
    if (!CORE_NS::EntityUtil::IsValid(ent)) {
        return nullptr;
    }
    auto ecsobj = scene->GetEcsContext().GetEcsObject(ent);
    if (!ecsobj) {
        return nullptr;
    }
    auto mesh = META_NS::GetObjectRegistry().Create<IMesh>(ClassId::Mesh);
    if (!mesh) {
        return nullptr;
    }
    if (auto acc = interface_cast<IEcsObjectAccess>(mesh)) {
        if (!acc->SetEcsObject(ecsobj)) {
            return nullptr;
        }
    }
    return mesh;
}

template<typename T>
constexpr static CORE3D_NS::IMeshBuilder::DataBuffer FillData(const BASE_NS::vector<T>& c) noexcept
{
    using namespace BASE_NS;
    Format format = BASE_FORMAT_UNDEFINED;
    if constexpr (is_same_v<T, Math::Vec2>) {
        format = BASE_FORMAT_R32G32_SFLOAT;
    } else if constexpr (is_same_v<T, Math::Vec3>) {
        format = BASE_FORMAT_R32G32B32_SFLOAT;
    } else if constexpr (is_same_v<T, Math::Vec4>) {
        format = BASE_FORMAT_R32G32B32A32_SFLOAT;
    } else if constexpr (is_same_v<T, uint16_t>) {
        format = BASE_FORMAT_R16_UINT;
    } else if constexpr (is_same_v<T, uint32_t>) {
        format = BASE_FORMAT_R32_UINT;
    }
    return CORE3D_NS::IMeshBuilder::DataBuffer { format, sizeof(T),
        { reinterpret_cast<const uint8_t*>(c.data()), c.size() * sizeof(T) } };
}

static CORE3D_NS::IMeshBuilder::Ptr CreateMeshBuilder(
    RENDER_NS::IRenderContext& context, const CORE3D_NS::IMeshBuilder::Submesh& submesh)
{
    CORE3D_NS::IMeshBuilder::Ptr builder;
    RENDER_NS::IShaderManager& shaderManager = context.GetDevice().GetShaderManager();
    const RENDER_NS::VertexInputDeclarationView vertexInputDeclaration =
        shaderManager.GetVertexInputDeclarationView(shaderManager.GetVertexInputDeclarationHandle(
            CORE3D_NS::DefaultMaterialShaderConstants::VERTEX_INPUT_DECLARATION_FORWARD));
    builder = CORE_NS::CreateInstance<CORE3D_NS::IMeshBuilder>(context, CORE3D_NS::UID_MESH_BUILDER);
    builder->Initialize(vertexInputDeclaration, 1);

    builder->AddSubmesh(submesh);
    builder->Allocate();

    return builder;
}

Future<IMesh::Ptr> MeshCreator::Create(const MeshConfig& c, CustomMeshData d)
{
    if (auto scene = scene_.lock()) {
        return scene->AddTask([=, data = BASE_NS::move(d)] {
            CORE3D_NS::IMeshBuilder::Submesh submesh;
            submesh.inputAssembly =
                RENDER_NS::GraphicsState::InputAssembly { false, RENDER_NS::PrimitiveTopology(data.topology) };
            submesh.material = GetMaterial(c);
            submesh.vertexCount = static_cast<uint32_t>(data.vertices.size());
            submesh.indexCount = static_cast<uint32_t>(data.indices.size());
            submesh.colors = true;

            auto builder = CreateMeshBuilder(scene->GetRenderContext(), submesh);

            auto positionData = FillData(data.vertices);
            auto normalData = FillData(data.normals);
            auto uvData = FillData(data.uvs);
            // Convert from BASE_NS::Color->BASE_NS::Math::Vec4
            BASE_NS::vector<BASE_NS::Math::Vec4> colors;
            colors.reserve(data.colors.size());
            for (auto&& color : data.colors) {
                colors.emplace_back(color.r, color.g, color.b, color.a);
            }
            auto colorData = FillData(colors);
            CORE3D_NS::IMeshBuilder::DataBuffer dummy {};

            builder->SetVertexData(0, positionData, normalData, uvData, dummy, dummy, colorData);
            builder->CalculateAABB(0, positionData);

            auto indexData = FillData(data.indices);
            builder->SetIndexData(0, indexData);

            auto ent = builder->CreateMesh(*scene->GetEcsContext().GetNativeEcs());
            if (!c.name.empty()) {
                CORE_NS::GetManager<CORE3D_NS::IUriComponentManager>(*scene->GetEcsContext().GetNativeEcs())
                    ->Set(ent, { c.name });
                CORE_NS::GetManager<CORE3D_NS::INameComponentManager>(*scene->GetEcsContext().GetNativeEcs())
                    ->Set(ent, { c.name });
            }
            return CreateMesh(scene, ent);
        });
    }
    return {};
}
Future<IMesh::Ptr> MeshCreator::CreateCube(const MeshConfig& c, float width, float height, float depth)
{
    if (auto scene = scene_.lock()) {
        return scene->AddTask([=] {
            auto& util = scene->GetGraphicsContext().GetMeshUtil();
            auto ent = util.GenerateCubeMesh(
                *scene->GetEcsContext().GetNativeEcs(), c.name, GetMaterial(c), width, height, depth);
            return CreateMesh(scene, ent);
        });
    }
    return {};
}
Future<IMesh::Ptr> MeshCreator::CreatePlane(const MeshConfig& c, float width, float depth)
{
    if (auto scene = scene_.lock()) {
        return scene->AddTask([=] {
            auto& util = scene->GetGraphicsContext().GetMeshUtil();
            auto ent =
                util.GeneratePlaneMesh(*scene->GetEcsContext().GetNativeEcs(), c.name, GetMaterial(c), width, depth);
            return CreateMesh(scene, ent);
        });
    }
    return {};
}
Future<IMesh::Ptr> MeshCreator::CreateSphere(const MeshConfig& c, float radius, uint32_t rings, uint32_t sectors)
{
    if (auto scene = scene_.lock()) {
        return scene->AddTask([=] {
            auto& util = scene->GetGraphicsContext().GetMeshUtil();
            auto ent = util.GenerateSphereMesh(
                *scene->GetEcsContext().GetNativeEcs(), c.name, GetMaterial(c), radius, rings, sectors);
            return CreateMesh(scene, ent);
        });
    }
    return {};
}
Future<IMesh::Ptr> MeshCreator::CreateCone(const MeshConfig& c, float radius, float length, uint32_t sectors)
{
    if (auto scene = scene_.lock()) {
        return scene->AddTask([=] {
            auto& util = scene->GetGraphicsContext().GetMeshUtil();
            auto ent = util.GenerateConeMesh(
                *scene->GetEcsContext().GetNativeEcs(), c.name, GetMaterial(c), radius, length, sectors);
            return CreateMesh(scene, ent);
        });
    }
    return {};
}

SCENE_END_NAMESPACE()