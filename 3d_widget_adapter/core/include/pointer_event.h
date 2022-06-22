/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_POINTER_EVENT_H
#define OHOS_RENDER_3D_POINTER_EVENT_H
namespace OHOS::Render3D {
enum class PointerEventType {
    PRESSED,
    RELEASED,
    CANCELLED,
    MOVED,
    SCROLL,
};

struct PointerEvent {
    PointerEventType eventType_;
    int pointerId_ {};
    int buttonIndex_ {};
    float x_ {};
    float y_ {};
    float deltaX_ {};
    float deltaY_ {};
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_POINTER_EVENT_H
