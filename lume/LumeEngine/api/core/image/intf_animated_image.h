/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_CORE_IMAGE_IANIMATED_IMAGE_H
#define API_CORE_IMAGE_IANIMATED_IMAGE_H

#include <base/containers/unique_ptr.h>
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
