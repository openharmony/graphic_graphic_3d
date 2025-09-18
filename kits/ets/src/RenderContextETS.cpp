/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "RenderContextETS.h"

#include <base/util/uid.h>

#include <core/image/intf_image_loader_manager.h>
#include <core/intf_engine.h>

#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object_context.h>

#include <scene/ext/intf_render_resource.h>
#include <scene/interface/intf_shader.h>
#include <scene/interface/intf_render_context.h>

#include <render/device/intf_gpu_resource_manager.h>

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

META_TYPE(BASE_NS::shared_ptr<CORE_NS::IImageLoaderManager::LoadResult>);

namespace OHOS::Render3D {
RenderContextETS::RenderContextETS()
{
    auto &r = META_NS::GetObjectRegistry();
    auto obj = r.Create<META_NS::IMetadata>(META_NS::ClassId::Object);
    if (obj) {
        auto doc = interface_cast<META_NS::IMetadata>(r.GetDefaultObjectContext());
        auto renderContext = doc->GetProperty<SCENE_NS::IRenderContext::Ptr>("RenderContext")->GetValue();
        obj->AddProperty(META_NS::ConstructProperty<SCENE_NS::IRenderContext::Ptr>("RenderContext", renderContext));
        renderResourceManager_ = interface_pointer_cast<SCENE_NS::IRenderResourceManager>(
            r.Create(SCENE_NS::ClassId::RenderResourceManager, obj));
    }
}

RenderContextETS::~RenderContextETS()
{
    renderResourceManager_.reset();
}

std::shared_ptr<RenderResourcesETS> RenderContextETS::GetResources()
{
    if (auto resources = resources_.lock()) {
        return resources;
    }

    auto resources = std::make_shared<RenderResourcesETS>();
    resources_ = resources;
    return resources;
}

bool RenderContextETS::LoadPlugin(const std::string &name)
{
    if (!BASE_NS::IsUidString(name.c_str())) {
        WIDGET_LOGE("%{public}s is not a Uid string", name.c_str());
        return false;
    }

    BASE_NS::Uid uid(*(char(*)[37])name.data());
    const auto engineQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD);
    META_NS::AddFutureTaskOrRunDirectly(engineQ, [uid]() {
        Core::GetPluginRegister().LoadPlugins({uid});
    });
    return true;
}

InvokeReturn<std::shared_ptr<ShaderETS>> RenderContextETS::CreateShader(const std::string &name, const std::string &uri)
{
    if (!renderResourceManager_) {
        return InvokeReturn<std::shared_ptr<ShaderETS>>(nullptr, "render resource manager is not ready");
    }
    if (uri.empty()) {
        return InvokeReturn<std::shared_ptr<ShaderETS>>(nullptr, "Invalid shader uri given");
    }
    SCENE_NS::IShader::Ptr shader = renderResourceManager_->LoadShader(uri.c_str()).GetResult();
    return InvokeReturn<std::shared_ptr<ShaderETS>>(std::make_shared<ShaderETS>(shader, name, uri));
}

std::shared_ptr<ImageETS> RenderContextETS::CreateImage(const std::string &name, const std::string &uri)
{
    // Create an image in four steps:
    // 1. Parse args in JS thread (this function body)
    // 2. Load image data in IO thread
    // 3. Create GPU resource in engine thread
    using namespace RENDER_NS;
    if (uri.empty()) {
        WIDGET_LOGE("Invalid scene resource Image parameters given");
        return nullptr;
    }

    if (auto resources = resources_.lock()) {
        if (const auto bitmap = resources->FetchBitmap(uri.c_str())) {
            // no aliasing.. so the returned bitmaps name is.. the old one.
            WIDGET_LOGI("Image uri already exist");
            return std::make_shared<ImageETS>(name, uri, bitmap);
        }
    }

    auto& obr = META_NS::GetObjectRegistry();
    auto doc = interface_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto renderContext = doc->GetProperty<SCENE_NS::IRenderContext::Ptr>("RenderContext")->GetValue()->GetRenderer();

    using LoadResult = CORE_NS::IImageLoaderManager::LoadResult;
    auto loadImage = [uri, renderContext]() {
        uint32_t imageLoaderFlags = CORE_NS::IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS;
        auto& imageLoaderMgr = renderContext->GetEngine().GetImageLoaderManager();
        // LoadResult contains a unique pointer, so can't copy. Move it to the heap and pass a pointer instead.
        return BASE_NS::shared_ptr<LoadResult>{new LoadResult{imageLoaderMgr.LoadImage(uri.c_str(), imageLoaderFlags)}};
    };

    auto createGpuResource = [uri, renderContext](
                                 BASE_NS::shared_ptr<LoadResult> loadResult) -> SCENE_NS::IBitmap::Ptr {
        if (!loadResult->success) {
            WIDGET_LOGE("%{public}s", BASE_NS::string { "Failed to load image: " }.append(loadResult->error).c_str());
            return {};
        }

        auto& gpuResourceMgr = renderContext->GetDevice().GetGpuResourceManager();
        GpuImageDesc gpuDesc = gpuResourceMgr.CreateGpuImageDesc(loadResult->image->GetImageDesc());
        gpuDesc.usageFlags = CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
        if (gpuDesc.engineCreationFlags & EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_GENERATE_MIPS) {
            gpuDesc.usageFlags |= CORE_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        gpuDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        const auto imageHandle = gpuResourceMgr.Create(uri.c_str(), gpuDesc, std::move(loadResult->image));

        auto& obr = META_NS::GetObjectRegistry();
        auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
        auto bitmap = META_NS::GetObjectRegistry().Create<SCENE_NS::IBitmap>(SCENE_NS::ClassId::Bitmap, doc);
        if (auto m = interface_cast<META_NS::IMetadata>(bitmap)) {
            m->AddProperty(META_NS::ConstructProperty<BASE_NS::string>("Uri", uri.c_str()));
        }
        if (auto i = interface_cast<SCENE_NS::IRenderResource>(bitmap)) {
            i->SetRenderHandle(imageHandle);
        }
        return bitmap;
    };

    const auto ioQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(IO_QUEUE);
    const auto engineQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD);
    auto bitmap = META_NS::AddFutureTaskOrRunDirectly(ioQ, BASE_NS::move(loadImage))
        .Then(BASE_NS::move(createGpuResource), engineQ).GetResult();
    if (!bitmap) {
        WIDGET_LOGE("Failed to load image from URI %{public}s", uri.c_str());
        return nullptr;
    }
    GetResources()->StoreBitmap(uri.c_str(), bitmap);
    return std::make_shared<ImageETS>(name, uri, bitmap);
}

bool RenderContextETS::RegisterResourcePath(const std::string &protocol, const std::string &uri)
{
    if (protocol.empty() || uri.empty()) {
        CORE_LOG_E("Invalid protocol or uri");
        return false;
    }
    auto &obr = META_NS::GetObjectRegistry();
    auto doc = interface_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    if (!doc) {
        CORE_LOG_E("Get default object context failed");
        return false;
    }
    auto &fileManager = doc->GetProperty<SCENE_NS::IRenderContext::Ptr>("RenderContext")
                            ->GetValue()->GetRenderer()->GetEngine().GetFileManager();
    // Check if the proxy protocol exists already.
    if (!fileManager.CheckExistence(protocol.c_str())) {
        CORE_LOG_E("Protocol %s does not exist", protocol.c_str());
        return false;
    }
    // Check if the protocol is already registered. | Register now!
    if (!fileManager.RegisterPath(protocol.c_str(), uri.c_str(), false)) {
        CORE_LOG_E("Register protocol %s with %s failed", protocol.c_str(), uri.c_str());
        return false;
    }
    return true;
}
}  // namespace OHOS::Render3D