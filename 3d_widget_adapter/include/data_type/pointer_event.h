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
    PointerEventType eventType_ { PointerEventType::RELEASED };
    int pointerId_ {};
    int buttonIndex_ {};
    float x_ {};
    float y_ {};
    float deltaX_ {};
    float deltaY_ {};
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_SCENE_VIEWER_TOUCH_EVENT_H
