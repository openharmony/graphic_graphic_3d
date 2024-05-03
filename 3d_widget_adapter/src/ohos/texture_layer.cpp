/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "texture_layer.h"

#include <native_buffer.h>
#include <render_service_base/include/pipeline/rs_recording_canvas.h>
#include <render_service_base/include/property/rs_properties_def.h>
#include <render_service_client/core/pipeline/rs_node_map.h>
#include <render_service_client/core/transaction/rs_interfaces.h>
#include <render_service_client/core/ui/rs_canvas_node.h>
#include <render_service_client/core/ui/rs_root_node.h>
#include <render_service_client/core/ui/rs_surface_node.h>
#include <surface_buffer.h>
#include <surface_utils.h>
#include <window.h>

#include "data_type/constants.h"
#include "3d_widget_adapter_log.h"
#include "graphics_manager.h"
#include "offscreen_context_helper.h"
#include "widget_trace.h"

namespace OHOS {
namespace Render3D {
struct TextureImage {
    explicit TextureImage(const TextureInfo& textureInfo): textureInfo_(textureInfo) {}
    TextureImage() = default;
    TextureInfo textureInfo_;
};

class TextureLayerImpl : public TextureLayer {
public:
    explicit TextureLayerImpl(int32_t key);
    virtual ~TextureLayerImpl();

    void DestroyRenderTarget() override;
    TextureInfo GetTextureInfo() override;

    void SetParent(std::shared_ptr<Rosen::RSNode>& parent) override;
    TextureInfo OnWindowChange(float offsetX, float offsetY, float width, float height, float scale,
        bool recreateWindow, SurfaceType surfaceType = SurfaceType::SURFACE_WINDOW) override;
    TextureInfo OnWindowChange(const WindowChangeInfo& windowChangeInfo) override;

private:
    void* CreateNativeWindow(uint32_t width, uint32_t height);
    void ConfigWindow(float offsetX, float offsetY, float width, float height, float scale,
        bool recreateWindow);
    void ConfigTexture(float width, float height);
    void RemoveChild();

    int32_t offsetX_ = 0u;
    int32_t offsetY_ = 0u;
    int32_t width_ = 0u;
    int32_t height_ = 0u;
    int32_t key_ = INT32_MAX;

    std::shared_ptr<Rosen::RSNode> rsNode_ = nullptr;
    std::shared_ptr<Rosen::RSNode> parent_ = nullptr;
    sptr<OHOS::Surface> producerSurface_ = nullptr;
    SurfaceType surface_ = SurfaceType::UNDEFINE;
    TextureImage image_;
};

TextureInfo TextureLayerImpl::GetTextureInfo()
{
    return image_.textureInfo_;
}

void TextureLayerImpl::SetParent(std::shared_ptr<Rosen::RSNode>& parent)
{
    parent_ = parent;
    // delete previous rs node reference
    RemoveChild();

    if (parent_ && rsNode_) {
        parent_->AddChild(rsNode_, 0); // second paramenter is added child at the index of parent's children;
    }
}

void TextureLayerImpl::RemoveChild()
{
    if (parent_ && rsNode_) {
        parent_->RemoveChild(rsNode_);
    }
}

void* TextureLayerImpl::CreateNativeWindow(uint32_t width, uint32_t height)
{
    struct Rosen::RSSurfaceNodeConfig surfaceNodeConfig = { .SurfaceNodeName = std::string("SceneViewer Model") +
        std::to_string(key_)};
    rsNode_ = Rosen::RSSurfaceNode::Create(surfaceNodeConfig, false);
    if (!rsNode_) {
        WIDGET_LOGE("Create rs node fail");
        return nullptr;
    }

    auto surfaceNode = OHOS::Rosen::RSBaseNode::ReinterpretCast<OHOS::Rosen::RSSurfaceNode>(rsNode_);
    surfaceNode->SetFrameGravity(Rosen::Gravity::RESIZE);
    if (surface_ == SurfaceType::SURFACE_WINDOW) {
        surfaceNode->SetHardwareEnabled(true);
    }
    if (surface_ == SurfaceType::SURFACE_TEXTURE) {
        surfaceNode->SetHardwareEnabled(false);
    }
    std::string hapPath = GraphicsManager::GetInstance().GetHapInfo().hapPath_;
    if (hapPath.find("SceneBoard_MetaBallsTurbo") != std::string::npos) {
        surfaceNode->SetHardwareEnabled(true);
    } else if (hapPath.find("HwWeather") != std::string::npos) {
        surfaceNode->SetHardwareEnabled(true);
        uint32_t argbWhite = 0xFFFFFFFF; // set a white background color for dss
        surfaceNode->SetBackgroundColor(argbWhite);
    }

    producerSurface_ = surfaceNode->GetSurface();
    if (!producerSurface_) {
        WIDGET_LOGE("Get producer surface fail");
        return nullptr;
    }
    auto ret = SurfaceUtils::GetInstance()->Add(producerSurface_->GetUniqueId(), producerSurface_);
    if (ret != SurfaceError::SURFACE_ERROR_OK) {
        WIDGET_LOGE("add surface error");
        return nullptr;
    }

    producerSurface_->SetQueueSize(3); // 3 seems ok
    producerSurface_->SetUserData("SURFACE_STRIDE_ALIGNMENT", "8");
    producerSurface_->SetUserData("SURFACE_FORMAT", std::to_string(GRAPHIC_PIXEL_FMT_RGBA_8888));
    producerSurface_->SetUserData("SURFACE_WIDTH", std::to_string(width));
    producerSurface_->SetUserData("SURFACE_HEIGHT", std::to_string(height));
    auto window = CreateNativeWindowFromSurface(&producerSurface_);

    return reinterpret_cast<void *>(window);
}

void TextureLayerImpl::ConfigWindow(float offsetX, float offsetY, float width, float height, float scale,
    bool recreateWindow)
{
    float widthScale = image_.textureInfo_.widthScale_;
    float heightScale = image_.textureInfo_.heightScale_;
    if (surface_ == SurfaceType::SURFACE_WINDOW || surface_ == SurfaceType::SURFACE_TEXTURE) {
        image_.textureInfo_.recreateWindow_ = recreateWindow;

        if (!image_.textureInfo_.nativeWindow_) {
            image_.textureInfo_.nativeWindow_ = reinterpret_cast<void *>(CreateNativeWindow(
                static_cast<uint32_t>(width * widthScale), static_cast<uint32_t>(height * heightScale)));
        }
        // need check recreate window flag
        NativeWindowHandleOpt(reinterpret_cast<OHNativeWindow *>(image_.textureInfo_.nativeWindow_),
            SET_BUFFER_GEOMETRY, static_cast<uint32_t>(width * scale * widthScale),
            static_cast<uint32_t>(height * scale * heightScale));
        rsNode_->SetBounds(offsetX, offsetY, width, height);
    }
}

TextureInfo TextureLayerImpl::OnWindowChange(float offsetX, float offsetY, float width, float height, float scale,
    bool recreateWindow, SurfaceType surfaceType)
{
    DestroyRenderTarget();
    surface_ = surfaceType;
    offsetX_ = offsetX;
    offsetY_ = offsetY;
    image_.textureInfo_.width_ = static_cast<uint32_t>(width);
    image_.textureInfo_.height_ = static_cast<uint32_t>(height);

    image_.textureInfo_.widthScale_ = scale;
    image_.textureInfo_.heightScale_ = scale;

    ConfigWindow(offsetX, offsetY, width, height, scale, recreateWindow);
    WIDGET_LOGD("TextureLayer OnWindowChange offsetX %f, offsetY %f, width %d, height %d, float scale %f,"
        "recreateWindow %d window empty %d", offsetX, offsetY, image_.textureInfo_.width_, image_.textureInfo_.height_,
        scale, recreateWindow, image_.textureInfo_.nativeWindow_ == nullptr);
    return image_.textureInfo_;
}

TextureInfo TextureLayerImpl::OnWindowChange(const WindowChangeInfo& windowChangeInfo)
{
    DestroyRenderTarget();
    surface_ = windowChangeInfo.surfaceType;
    offsetX_ = (int)windowChangeInfo.offsetX;
    offsetY_ = (int)windowChangeInfo.offsetY;

    image_.textureInfo_.width_ = static_cast<uint32_t>(windowChangeInfo.width);
    image_.textureInfo_.height_ = static_cast<uint32_t>(windowChangeInfo.height);

    image_.textureInfo_.widthScale_ = static_cast<float>(windowChangeInfo.widthScale);
    image_.textureInfo_.heightScale_ = static_cast<float>(windowChangeInfo.heightScale);

    ConfigWindow(windowChangeInfo.offsetX, windowChangeInfo.offsetY, windowChangeInfo.width,
        windowChangeInfo.height, windowChangeInfo.scale, windowChangeInfo.recreateWindow);

    return image_.textureInfo_;
}

void TextureLayerImpl::DestroyRenderTarget()
{
    // window release
    RemoveChild();
    rsNode_ = nullptr;
    parent_ = nullptr;
    image_.textureInfo_ = {};
}

TextureLayerImpl::TextureLayerImpl(int32_t key): key_(key)
{
}

TextureLayerImpl::~TextureLayerImpl()
{
    // explicity release resource before destructor
}

TextureInfo TextureLayer::GetTextureInfo()
{
    return textureLayer_->GetTextureInfo();
}

void TextureLayer::SetParent(std::shared_ptr<Rosen::RSNode>& parent)
{
    return textureLayer_->SetParent(parent);
}

TextureInfo TextureLayer::OnWindowChange(float offsetX, float offsetY, float width, float height, float scale,
    bool recreateWindow, SurfaceType surfaceType)
{
    return textureLayer_->OnWindowChange(offsetX, offsetY, width, height, scale, recreateWindow, surfaceType);
}

TextureInfo TextureLayer::OnWindowChange(const WindowChangeInfo& windowChangeInfo)
{
    return textureLayer_->OnWindowChange(windowChangeInfo);
}

void TextureLayer::DestroyRenderTarget()
{
    textureLayer_->DestroyRenderTarget();
}

TextureLayer::TextureLayer(int32_t key)
{
    textureLayer_ = std::make_shared<TextureLayerImpl>(key);
}

TextureLayer::~TextureLayer()
{
    // explicit release resource before destructor
}

} // Render3D
} // OHOS
