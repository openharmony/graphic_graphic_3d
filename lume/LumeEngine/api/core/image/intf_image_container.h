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

#ifndef API_CORE_IMAGE_IIMAGE_CONTAINER_H
#define API_CORE_IMAGE_IIMAGE_CONTAINER_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/unique_ptr.h>
#include <base/namespace.h>
#include <base/util/formats.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
/** \addtogroup group_image_iimagecontainer
 *  @{
 */

/**
 * An interface that is used to represent a collection of bitmap image data.
 *
 * The container can be:
 *    - a single 1D/2D/3D bitmap image (e.g. loaded from a png file)
 *    - an array of 1D/2D images
 *    - a cube map
 *    - an array of cube maps
 *
 * All of the above possibly containing mipmaps.
 */
class IImageContainer {
public:
    /** Image flags */
    enum ImageFlags : uint32_t {
        /** Bit for cubemap */
        FLAGS_CUBEMAP_BIT = 0x00000001,
        /** Bit for packed */
        FLAGS_PACKED_BIT = 0x00000002,
        /** Bit for compressed */
        FLAGS_COMPRESSED_BIT = 0x00000004,
        /** Bit for request mipmaps */
        FLAGS_REQUESTING_MIPMAPS_BIT = 0x00000008,
        /** Image color data has been premultiplied with the alpha value */
        FLAGS_PREMULTIPLIED_ALPHA_BIT = 0x00000010,
        /** Image is animated */
        FLAGS_ANIMATED_BIT = 0x00000020,
    };

    /** Image type */
    enum ImageType : uint32_t {
        /** 1D */
        TYPE_1D = 0,
        /** 2D */
        TYPE_2D = 1,
        /** 3D */
        TYPE_3D = 2,
        /** Max enumeration */
        TYPE_MAX_ENUM = 0x7FFFFFFF
    };

    /** Image view type */
    enum ImageViewType : uint32_t {
        /** 1D */
        VIEW_TYPE_1D = 0,
        /** 2D */
        VIEW_TYPE_2D = 1,
        /** 3D */
        VIEW_TYPE_3D = 2,
        /** Cube */
        VIEW_TYPE_CUBE = 3,
        /** 1D array */
        VIEW_TYPE_1D_ARRAY = 4,
        /** 2D array */
        VIEW_TYPE_2D_ARRAY = 5,
        /** Cube array */
        VIEW_TYPE_CUBE_ARRAY = 6,
        /** Max enumeration */
        VIEW_TYPE_MAX_ENUM = 0x7FFFFFFF
    };

    /** Image descriptor */
    struct ImageDesc {
        /** Flags for image descriptor */
        uint32_t imageFlags { 0 };

        /** Pixel width of block */
        uint32_t blockPixelWidth { 0 };
        /** Pixel height of block */
        uint32_t blockPixelHeight { 0 };
        /** Pixel depth of block */
        uint32_t blockPixelDepth { 0 };
        /** Bits per block */
        uint32_t bitsPerBlock { 0 };

        /** Image type */
        ImageType imageType { ImageType::TYPE_MAX_ENUM };
        /** Image view type */
        ImageViewType imageViewType { ImageViewType::VIEW_TYPE_MAX_ENUM };
        /** Format */
        BASE_NS::Format format { BASE_NS::Format::BASE_FORMAT_UNDEFINED };

        /** Width */
        uint32_t width { 0 };
        /** Height */
        uint32_t height { 0 };
        /** Depth */
        uint32_t depth { 0 };

        /** Mip count */
        uint32_t mipCount { 1 };
        /** Layer count */
        uint32_t layerCount { 1 };
    };

    /** Descriptor for each subimage (mip level, cube face etc.) */
    struct SubImageDesc {
        /** Buffer offset */
        uint32_t bufferOffset { 0 };
        /** Buffer row length */
        uint32_t bufferRowLength { 0 };
        /** Buffer image height */
        uint32_t bufferImageHeight { 0 };
        /** Mip level */
        uint32_t mipLevel { 0 };
        /** Layer count */
        uint32_t layerCount { 0 };
        /** Width */
        uint32_t width { 0 };
        /** Height */
        uint32_t height { 0 };
        /** Depth */
        uint32_t depth { 0 };
    };

    /** Preventing copy construction and assign. */
    IImageContainer(IImageContainer const&) = delete;
    IImageContainer& operator=(IImageContainer const&) = delete;

    /** Return descriptor containing information about this image. */
    virtual const ImageDesc& GetImageDesc() const = 0;

    /** Return The data buffer that holds the data for all of the image elements of this container. */
    virtual BASE_NS::array_view<const uint8_t> GetData() const = 0;

    /** Return Array containing a SubImageDesc for each subimage (mipmap level, cubemap face etc.). */
    virtual BASE_NS::array_view<const SubImageDesc> GetBufferImageCopies() const = 0;

    struct Deleter {
        constexpr Deleter() noexcept = default;
        void operator()(IImageContainer* ptr) const
        {
            ptr->Destroy();
        }
    };
    using Ptr = BASE_NS::unique_ptr<IImageContainer, Deleter>;

protected:
    IImageContainer() = default;
    virtual ~IImageContainer() = default;
    virtual void Destroy() = 0;
};
/** @} */
CORE_END_NAMESPACE()

#endif // API_CORE_IMAGE_IIMAGE_CONTAINER_H
