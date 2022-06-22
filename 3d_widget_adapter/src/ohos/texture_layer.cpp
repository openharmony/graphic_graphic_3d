/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "texture_layer.h"
#include "3d_widget_adapter_log.h"

#include "include/gpu/GrContext.h"
#include "include/gpu/GrBackendSurface.h"
#include "include/gpu/gl/GrGLInterface.h"

namespace OHOS {
namespace Render3D {
TextureLayer::~TextureLayer()
{
    image_.skImage_ = nullptr;
}

void TextureLayer::UpdateRenderFinishFuture(std::shared_future<void> &ftr)
{
    std::lock_guard<std::mutex> lk(ftrMut_);
    image_.ftr_ = ftr;
}

void TextureLayer::SetOffset(int32_t x, int32_t y)
{
    offsetX_ = x;
    offsetY_ = y;
}

SkRect TextureLayer::onGetBounds()
{
    return SkRect::MakeWH(image_.textureInfo_.width_, image_.textureInfo_.height_);
}

void TextureLayer::onDraw(SkCanvas* canvas)
{
    std::lock_guard<std::mutex> lk(ftrMut_);
    if (image_.ftr_.valid()) {
        image_.ftr_.get();
        DrawTexture(canvas);
    }
}

void TextureLayer::DrawTexture(SkCanvas* canvas)
{
    if (image_.textureInfo_.textureId_ <= 0) {
        WIDGET_LOGE("%s invalid texture %d", __func__, __LINE__);
        return;
    }

    if (image_.skImage_ == nullptr) {
        GrGLTextureInfo textureInfo = { GL_TEXTURE_2D, image_.textureInfo_.textureId_, GL_RGBA8_OES };
        GrBackendTexture backendTexture(image_.textureInfo_.width_, image_.textureInfo_.height_,
            GrMipMapped::kNo, textureInfo);

        image_.skImage_ = SkImage::MakeFromTexture(canvas->getGrContext(), backendTexture, kTopLeft_GrSurfaceOrigin,
            kRGBA_8888_SkColorType, kPremul_SkAlphaType, SkColorSpace::MakeSRGB());
    }

    canvas->drawImage(image_.skImage_, offsetX_, offsetY_);
}
}
}
