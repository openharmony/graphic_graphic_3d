/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */
#ifndef OHOS_RENDER_3D_TEXTURE_LAYER_H
#define OHOS_RENDER_3D_TEXTURE_LAYER_H

#include "texture_info.h"
#include "include/core/SkImage.h"
#include "include/core/SkDrawable.h"
#include "include/core/SkCanvas.h"

#include <cstdint>
#include <memory>
#include <future>
#include <shared_mutex>
#include <GLES3/gl31.h>
#include <GLES2/gl2ext.h>

namespace OHOS::Render3D {

struct TextureImage {
    explicit TextureImage(const TextureInfo& textureInfo) : textureInfo_(textureInfo) {}
    TextureImage() = default;
    TextureInfo textureInfo_;
    std::function<void()> task_ = nullptr;
    std::shared_future<void> ftr_;
    std::promise<void> pms_;
    sk_sp<SkImage> skImage_;
};

class TextureLayer : public SkDrawable {
public:
    explicit TextureLayer(const TextureInfo &info) : image_(info) {}
    virtual ~TextureLayer();
    virtual SkRect onGetBounds() override;
    virtual void onDraw(SkCanvas* canvas) override;
    void UpdateRenderFinishFuture(std::shared_future<void> &ftr);
    void SetOffset(int32_t x, int32_t y);;

private:
    void DrawTexture(SkCanvas* canvas);
    TextureImage image_;
    int32_t offsetX_;
    int32_t offsetY_;
    std::mutex ftrMut_;
    std::mutex skImageMut_;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_TEXTURE_LAYER_H
