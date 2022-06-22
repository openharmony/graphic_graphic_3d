/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_SCENE_VIEWER_TOUCH_EVENT_H
#define OHOS_RENDER_3D_SCENE_VIEWER_TOUCH_EVENT_H

#include "core/event/touch_event.h"

namespace OHOS::Render3D {
class SceneViewerTouchEvent : public OHOS::Ace::TouchLocationInfo {
public:
    SceneViewerTouchEvent(int32_t pointerId) : TouchLocationInfo(pointerId) {}
    ~SceneViewerTouchEvent() = default;

    SceneViewerTouchEvent& SetDeltaChange(const OHOS::Ace::Offset& deltaChange)
    {
        deltaChange_ = deltaChange;
        return *this;
    }

    const OHOS::Ace::Offset& GetDeltaChange() const
    {
        return deltaChange_;
    }

    SceneViewerTouchEvent& SetEventType(const OHOS::Ace::TouchType& type)
    {
        type_ = type;
        return *this;
    }

    const OHOS::Ace::TouchType& GetEventType() const
    {
        return type_;
    }

private:
    OHOS::Ace::TouchType type_ = OHOS::Ace::TouchType::UNKNOWN;
    OHOS::Ace::Offset deltaChange_;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_SCENE_VIEWER_TOUCH_EVENT_H
