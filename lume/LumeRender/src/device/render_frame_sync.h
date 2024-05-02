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

#ifndef DEVICE_RENDER_FRAME_SYNC_H
#define DEVICE_RENDER_FRAME_SYNC_H

#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
class RenderFrameSync {
public:
    RenderFrameSync() = default;
    virtual ~RenderFrameSync() = default;

    /* Begin frame, i.e. advance to next buffer. */
    virtual void BeginFrame() = 0;
    /* Wait for current frame's fence and reset. */
    virtual void WaitForFrameFence() = 0;
};
RENDER_END_NAMESPACE()

#endif // DEVICE_RENDER_FRAME_SYNC_H
