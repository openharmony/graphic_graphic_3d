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

#ifndef API_CORE_IMAGE_IANIMATED_IMAGE_H
#define API_CORE_IMAGE_IANIMATED_IMAGE_H

#include <cstdint>

#include <base/containers/unique_ptr.h>
#include <base/namespace.h>
#include <core/image/intf_image_container.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()

/** Interface for decoding animated images for different image formats. */
class IAnimatedImage {
public:
    static constexpr int MAX_ERR_MSG_LEN = 128;

    /** Single frame from animation. */
    struct AnimationFrame {
        /** frame timestamp */
        uint64_t timestamp;

        /** frame number starting from 0 */
        uint32_t frameNumber;

        /** Pointer to image container for this frame */
        IImageContainer::Ptr image;
    };

    /** Restart animation from begining */
    virtual void Restart() = 0;

    /** Get loop count */
    virtual uint32_t GetLoopCount() = 0;

    /** Get total number of frames */
    virtual uint32_t GetNumberOfFrames() = 0;

    /** Get the next animation frame. */
    virtual AnimationFrame GetNextFrame() = 0;

    /** Preventing copy construction and assign. */
    IAnimatedImage(IAnimatedImage const&) = delete;
    IAnimatedImage& operator=(IAnimatedImage const&) = delete;

    struct Deleter {
        constexpr Deleter() noexcept = default;
        void operator()(IAnimatedImage* ptr) const
        {
            ptr->Destroy();
        }
    };
    using Ptr = BASE_NS::unique_ptr<IAnimatedImage, Deleter>;

protected:
    IAnimatedImage() = default;
    virtual ~IAnimatedImage() = default;
    virtual void Destroy() = 0;
};
CORE_END_NAMESPACE()

#endif // API_CORE_IMAGE_IANIMATED_IMAGE_H
