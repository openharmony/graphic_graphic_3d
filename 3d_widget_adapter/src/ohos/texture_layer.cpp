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

#include "texture_layer.h"

#ifdef USE_M133_SKIA
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#else
#include "include/gpu/GrBackendSurface.h"
#include "include/gpu/gl/GrGLInterface.h"
#endif
#include <native_buffer.h>
#include <render_service_base/include/pipeline/rs_recording_canvas.h>
#include <render_service_base/include/property/rs_properties_def.h>
#include <render_service_client/core/pipeline/rs_node_map.h>
#include <render_service_client/core/transaction/rs_interfaces.h>
#include <render_service_client/core/ui/rs_canvas_node.h>
#include <render_service_client/core/ui/rs_root_node.h>
#include <render_service_client/core/ui/rs_surface_node.h>
#include <render_service_client/core/ui/rs_ui_director.h>

#include <surface_buffer.h>
#include <surface_utils.h>
#include <window.h>

#include "data_type/constants.h"
#include "3d_widget_adapter_log.h"
#include "graphics_manager.h"
#include "offscreen_context_helper.h"
#include "widget_trace.h"

struct OH_NativeBuffer;
struct NativeWindowBuffer;

namespace OHOS {
namespace Render3D {

struct TextureImage {
    explicit TextureImage(const TextureInfo& textureInfo) : textureInfo_(textureInfo) {}
    TextureImage() = default;
    TextureInfo textureInfo_;
};

class TextureLayerImpl : public TextureLayer {
public:
    explicit TextureLayerImpl(int32_t key);
    ~TextureLayerImpl();

    void DestroyRenderTarget() override;
    TextureInfo GetTextureInfo() override;

    void SetParent(std::shared_ptr<Rosen::RSNode>& parent) override;
    TextureInfo OnWindowChange(float offsetX, float offsetY, float width, float height, float scale,
        bool recreateWindow, SurfaceType surfaceType = SurfaceType::SURFACE_WINDOW) override;
    TextureInfo OnWindowChange(const WindowChangeInfo& windowChangeInfo) override;
    void SetBackgroundColor(uint32_t backgroundColor) override;

private:
    void CreateNatviceWindowNode(const Rosen::RSSurfaceNodeConfig &surfaceNodeConfig);
    void* CreateNativeWindow(uint32_t width, uint32_t height);
    void ConfigWindow(float offsetX, float offsetY, float width, float height, float scale, bool recreateWindow);
    void ConfigTexture(float width, float height);
    void RemoveChild();

    int32_t offsetX_ = 0u;
    int32_t offsetY_ = 0u;
    uint32_t width_ = 0u;
    uint32_t height_ = 0u;
    int32_t key_ = INT32_MAX;
    uint32_t transform_ = 0U;
    uint32_t backgroundColor_ = 0U;

    std::shared_ptr<Rosen::RSNode> rsNode_ = nullptr;
    std::shared_ptr<Rosen::RSNode> parent_ = nullptr;
    sptr<OHOS::Surface> producerSurface_ = nullptr;
    std::shared_ptr<OHOS::Rosen::RSUIDirector> rsUIDirector_;
    SurfaceType surface_ = SurfaceType::UNDEFINE;
    TextureImage image_;
    std::string bundleName_ = "";
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
        rsNode_->SetRSUIContext(parent->GetRSUIContext());
        parent_->AddChild(rsNode_, 0); // second paramenter is added child at the index of parent's children;
    }
}

void TextureLayerImpl::RemoveChild()
{
    if (parent_ && rsNode_) {
        parent_->RemoveChild(rsNode_);
    }
}

GraphicTransformType RotationToTransform(uint32_t rotation)
{
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_ROTATE_BUTT;
    switch (rotation) {
        case 0: // rotation angle 0 degree
            transform = GraphicTransformType::GRAPHIC_ROTATE_NONE;
            break;
        case 90: // rotation angle 90 degree
            transform = GraphicTransformType::GRAPHIC_ROTATE_90;
            break;
        case 180: // rotation angle 180 degree
            transform = GraphicTransformType::GRAPHIC_ROTATE_180;
            break;
        case 270: // rotation angle 270 degree
            transform = GraphicTransformType::GRAPHIC_ROTATE_270;
            break;
        default:
            transform = GraphicTransformType::GRAPHIC_ROTATE_NONE;
            break;
    }
    return transform;
}

void TextureLayerImpl::CreateNatviceWindowNode(const Rosen::RSSurfaceNodeConfig &surfaceNodeConfig)
{
    rsUIDirector_ = OHOS::Rosen::RSUIDirector::Create();
    // Init: The first true indicates that RenderThread is created, and the second true indicates that multiple
    // instances are used. When multi-instance is not used, each process has a global RSUIDirector. When multi-instance
    // is used, each instance corresponds to one RSUIDirector.
    rsUIDirector_->Init(true, false); // the second params need to be true for adapting multi-instances
    auto rsUIContext = rsUIDirector_->GetRSUIContext();
    rsNode_ = Rosen::RSSurfaceNode::Create(surfaceNodeConfig, false, rsUIContext);
}

void* TextureLayerImpl::CreateNativeWindow(uint32_t width, uint32_t height)
{
    bundleName_ = GraphicsManager::GetInstance().GetHapInfo().bundleName_;
    struct Rosen::RSSurfaceNodeConfig surfaceNodeConfig;
    if (bundleName_.find("totemweather") != std::string::npos) {
        surfaceNodeConfig = { .SurfaceNodeName = std::string("SceneViewer Model totemweather") + std::to_string(key_) };
    } else {
        surfaceNodeConfig = { .SurfaceNodeName = std::string("SceneViewer Model") + std::to_string(key_) };
    }
    CreateNatviceWindowNode(surfaceNodeConfig);
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

    if (bundleName_.find("sceneboard") != std::string::npos) {
        surfaceNode->SetHardwareEnabled(true); // SetHardwareEnabled as a flag enable gpu bilinear interpolation
    } else if (bundleName_.find("totemweather") != std::string::npos) {
        surfaceNode->SetHardwareEnabled(true);
        surfaceNode->SetBackgroundColor(backgroundColor_);
    }

    producerSurface_ = surfaceNode->GetSurface();
    if (!producerSurface_) {
        WIDGET_LOGE("Get producer surface fail");
        return nullptr;
    }

    auto transform = RotationToTransform(transform_);
    producerSurface_->SetTransformHint(transform);

    auto ret = SurfaceUtils::GetInstance()->Add(producerSurface_->GetUniqueId(), producerSurface_);
    if (ret != SurfaceError::SURFACE_ERROR_OK) {
        WIDGET_LOGE("add surface error");
        return nullptr;
    }

    producerSurface_->SetQueueSize(5); // 5 seems ok
    producerSurface_->SetUserData("SURFACE_STRIDE_ALIGNMENT", "8");
    producerSurface_->SetUserData("SURFACE_FORMAT", std::to_string(GRAPHIC_PIXEL_FMT_RGBA_8888));
    producerSurface_->SetUserData("SURFACE_WIDTH", std::to_string(width));
    producerSurface_->SetUserData("SURFACE_HEIGHT", std::to_string(height));
    auto window = CreateNativeWindowFromSurface(&producerSurface_);
    if (!window) {
        WIDGET_LOGE("CreateNativeWindowFromSurface failed");
    }

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
        if (rsNode_ == nullptr) {
            WIDGET_LOGE("TextureLayer ConfigWindow rsNode_ is nullptr.");
            return;
        }
        rsNode_->SetBounds(offsetX, offsetY, width, height);
    }
}

TextureInfo TextureLayerImpl::OnWindowChange(float offsetX, float offsetY, float width, float height, float scale,
    bool recreateWindow, SurfaceType surfaceType)
{
    // no DestroyRenderTarget will not cause memory leak / render issue
    surface_ = surfaceType;
    offsetX_ = offsetX;
    offsetY_ = offsetY;

    image_.textureInfo_.width_ = static_cast<uint32_t>(width);
    image_.textureInfo_.height_ = static_cast<uint32_t>(height);

    ConfigWindow(offsetX, offsetY, width, height, scale, recreateWindow);

    WIDGET_LOGD("TextureLayer OnWindowChange offsetX %f, offsetY %f, width %d, height %d, float scale %f,"
        "recreateWindow %d window empty %d", offsetX, offsetY, image_.textureInfo_.width_, image_.textureInfo_.height_,
        scale, recreateWindow, image_.textureInfo_.nativeWindow_ == nullptr);
    return image_.textureInfo_;
}

TextureInfo TextureLayerImpl::OnWindowChange(const WindowChangeInfo& windowChangeInfo)
{
    // no DestroyRenderTarget will not cause memory leak / render issue
    switch (windowChangeInfo.surfaceType) {
        case SurfaceType::SURFACE_WINDOW:
        case SurfaceType::SURFACE_TEXTURE:
        case SurfaceType::SURFACE_BUFFER:
            surface_ = windowChangeInfo.surfaceType;
            break;
        default:
            surface_ = SurfaceType::SURFACE_WINDOW;
            break;
    }
    offsetX_ = (int)windowChangeInfo.offsetX;
    offsetY_ = (int)windowChangeInfo.offsetY;
    transform_ = windowChangeInfo.transformType;
    backgroundColor_ = windowChangeInfo.backgroundColor;

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
    // clean cache routine should be in rs node deconstruct
    auto rsSurfaceNode = OHOS::Rosen::RSBaseNode::ReinterpretCast<OHOS::Rosen::RSSurfaceNode>(rsNode_);
    if (rsSurfaceNode && rsSurfaceNode->GetSurface()) {
        rsSurfaceNode->GetSurface()->CleanCache(true);
    }

    rsNode_ = nullptr;
    parent_ = nullptr;
    image_.textureInfo_ = {};
}

void TextureLayerImpl::SetBackgroundColor(uint32_t backgroundColor)
{
    backgroundColor_ = backgroundColor;
    auto surfaceNode = OHOS::Rosen::RSBaseNode::ReinterpretCast<OHOS::Rosen::RSSurfaceNode>(rsNode_);
    if (surfaceNode && (bundleName_.find("totemweather") != std::string::npos)) {
        surfaceNode->SetBackgroundColor(backgroundColor_);
    }
}

TextureLayerImpl::TextureLayerImpl(int32_t key) : key_(key)
{
}

TextureLayerImpl::~TextureLayerImpl()
{
    // explicit release resource before destructor
}

// old implement
TextureInfo TextureLayer::GetTextureInfo()
{
    return textureLayer_->GetTextureInfo();
}

void TextureLayer::SetParent(std::shared_ptr<Rosen::RSNode>& parent)
{
    textureLayer_->SetParent(parent);
}

void TextureLayer::SetBackgroundColor(uint32_t backgroundColor)
{
    textureLayer_->SetBackgroundColor(backgroundColor);
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
