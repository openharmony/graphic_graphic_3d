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
#include "gltf2.h"

#include <3d/gltf/gltf.h>
#include <3d/intf_graphics_context.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/log.h>
#include <core/plugin/intf_plugin_register.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>

#include "data.h"
#include "gltf2_exporter.h"
#include "gltf2_importer.h"
#include "gltf2_loader.h"

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;

Gltf2::Gltf2(IGraphicsContext& graphicsContext)
    : engine_(&(graphicsContext.GetRenderContext().GetEngine())), renderContext_(&graphicsContext.GetRenderContext()),
      fileManager_(engine_->GetFileManager())
{}

Gltf2::Gltf2(IFileManager& fileManager) : fileManager_(fileManager) {}

// Internal helper.
GLTFLoadResult LoadGLTF(IFileManager& fileManager, const string_view uri)
{
    auto loadResult = GLTF2::LoadGLTF(fileManager, uri);
    GLTFLoadResult result;
    result.error = move(loadResult.error);
    result.success = loadResult.success;
    result.data = IGLTFData::Ptr { loadResult.data.release() };

    return result;
}

// Api loading function.
GLTFLoadResult Gltf2::LoadGLTF(const string_view uri)
{
    return CORE3D_NS::LoadGLTF(fileManager_, uri);
}

GLTFLoadResult Gltf2::LoadGLTF(array_view<uint8_t const> data)
{
    auto loadResult = GLTF2::LoadGLTF(fileManager_, data);
    GLTFLoadResult result;
    result.error = move(loadResult.error);
    result.success = loadResult.success;
    result.data = IGLTFData::Ptr { loadResult.data.release() };
    return result;
}

// Api import functions
Entity Gltf2::ImportGltfScene(size_t sceneIndex, const IGLTFData& gltfData, const GLTFResourceData& gltfResourceData,
    IEcs& ecs, Entity rootEntity, GltfSceneImportFlags flags)
{
    CORE_ASSERT(renderContext_);
    if (renderContext_) {
        const GLTF2::Data& data = static_cast<const GLTF2::Data&>(gltfData);
        return ImportScene(renderContext_->GetDevice(), sceneIndex, data, gltfResourceData, ecs, rootEntity, flags);
    }
    return {};
}

IGLTF2Importer::Ptr Gltf2::CreateGLTF2Importer(IEcs& ecs)
{
    CORE_ASSERT(engine_ && renderContext_);
    if (engine_ && renderContext_) {
        if (auto pool = ecs.GetThreadPool(); pool) {
            return CreateGLTF2Importer(ecs, *pool);
        }
        return IGLTF2Importer::Ptr { new GLTF2::GLTF2Importer(*engine_, *renderContext_, ecs) };
    }
    return nullptr;
}

IGLTF2Importer::Ptr Gltf2::CreateGLTF2Importer(IEcs& ecs, IThreadPool& pool)
{
    CORE_ASSERT(engine_ && renderContext_);
    if (engine_ && renderContext_) {
        return IGLTF2Importer::Ptr { new GLTF2::GLTF2Importer(*engine_, *renderContext_, ecs, pool) };
    }
    return nullptr;
}

ISceneLoader::Result Gltf2::Load(string_view uri)
{
    ISceneLoader::Result sceneResult;
    auto loadResult = GLTF2::LoadGLTF(fileManager_, uri);
    sceneResult.error = loadResult.success ? 0 : 1;
    sceneResult.message = BASE_NS::move(loadResult.error);
    sceneResult.data.reset(new GLTF2::SceneData(BASE_NS::move(loadResult.data)));
    return sceneResult;
}

ISceneImporter::Ptr Gltf2::CreateSceneImporter(IEcs& ecs)
{
    CORE_ASSERT(engine_ && renderContext_);
    if (engine_ && renderContext_) {
        return ISceneImporter::Ptr { new GLTF2::Gltf2SceneImporter(*engine_, *renderContext_, ecs) };
    }
    return nullptr;
}

ISceneImporter::Ptr Gltf2::CreateSceneImporter(IEcs& ecs, IThreadPool& pool)
{
    CORE_ASSERT(engine_ && renderContext_);
    if (engine_ && renderContext_) {
        return ISceneImporter::Ptr { new GLTF2::Gltf2SceneImporter(*engine_, *renderContext_, ecs, pool) };
    }
    return nullptr;
}

array_view<const string_view> Gltf2::GetSupportedExtensions() const
{
    static constexpr string_view extensions[] = { "gltf", "glb" };
    return extensions;
}

// IInterface
const IInterface* Gltf2::GetInterface(const Uid& uid) const
{
    if (uid == ISceneLoader::UID) {
        return static_cast<const ISceneLoader*>(this);
    }
    if (uid == IInterface::UID) {
        return static_cast<const IInterface*>(this);
    }
    return nullptr;
}

IInterface* Gltf2::GetInterface(const Uid& uid)
{
    if (uid == ISceneLoader::UID) {
        return static_cast<ISceneLoader*>(this);
    }
    if (uid == IInterface::UID) {
        return static_cast<IInterface*>(this);
    }
    return nullptr;
}

void Gltf2::Ref() {}

void Gltf2::Unref() {}

// Api exporting function.
bool Gltf2::SaveGLTF(IEcs& ecs, const string_view uri)
{
    CORE_ASSERT(engine_);
    if (engine_) {
        if (auto result = GLTF2::ExportGLTF(*engine_, ecs); result.success) {
            if (auto file = fileManager_.CreateFile(uri); file) {
                auto const ext = uri.rfind('.');
                auto const extension = string_view(uri.data() + ext + 1);
                if (extension == "gltf") {
                    for (auto const& buffer : result.data->buffers) {
                        string dataFileUri = uri.substr(0, ext) + ".bin";
                        if (auto dataFile = fileManager_.CreateFile(dataFileUri); dataFile) {
                            dataFile->Write(buffer->data.data(), buffer->data.size());
                            if (auto const path = dataFileUri.rfind('/'); path != string::npos) {
                                dataFileUri.erase(0, path + 1);
                            }
                            buffer->uri = dataFileUri;
                        }
                    }
                    GLTF2::SaveGLTF(*result.data, *file, engine_->GetVersion());
                } else {
                    GLTF2::SaveGLB(*result.data, *file, engine_->GetVersion());
                }
            } else {
                result.error += "Failed to create file: " + uri;
                result.success = false;
            }
            return result.success;
        } else {
            return result.success;
        }
    }
    return false;
}
CORE3D_END_NAMESPACE()
