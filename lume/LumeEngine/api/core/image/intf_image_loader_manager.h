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

#ifndef API_CORE_IMAGE_IIMAGE_LOADER_MANAGER_H
#define API_CORE_IMAGE_IIMAGE_LOADER_MANAGER_H

#include <cstdint>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/image/intf_animated_image.h>
#include <core/image/intf_image_container.h>
#include <core/namespace.h>
#include <core/plugin/intf_plugin.h>

BASE_BEGIN_NAMESPACE()
template<class T>
class array_view;
BASE_END_NAMESPACE()

CORE_BEGIN_NAMESPACE()
class IFile;
const int MAX_ERR_MSG_LEN = 128;

// NOTE: Nicer way to return error strings.
/** @ingroup group_image_iimagemanager */
/** IImage manager */
class IImageLoaderManager {
public:
    /** Describes result of the loading operation. */
    struct LoadResult {
        /** Load result was successful? */
        bool success { false };
        /** Load result error message if load was not successful. */
        char error[MAX_ERR_MSG_LEN];

        /** Pointer to image container */
        IImageContainer::Ptr image;
    };

    /** Describes result of the animation loading operation. */
    struct LoadAnimatedResult {
        /** Load result was successful? */
        bool success { false };
        /** Load result error message if load was not successful. */
        char error[MAX_ERR_MSG_LEN];

        /** Pointer to image container */
        IAnimatedImage::Ptr image;
    };

    /** Describes supported format. */
    struct ImageType {
        /** Media type (Multipurpose Internet Mail Extensions or MIME). */
        BASE_NS::string mimeType;
        /** File extension typically used for this media type. */
        BASE_NS::string fileExtension;
    };

    /** Image loader flags */
    enum ImageLoaderFlags : uint32_t {
        /** Generate mipmaps if not already included in the image file. */
        IMAGE_LOADER_GENERATE_MIPS = 0x00000001,
        /** Force linear RGB. */
        IMAGE_LOADER_FORCE_LINEAR_RGB_BIT = 0x00000002,
        /** Force SRGB. */
        IMAGE_LOADER_FORCE_SRGB_BIT = 0x00000004,
        /** Force grayscale. */
        IMAGE_LOADER_FORCE_GRAYSCALE_BIT = 0x00000008,
        /** Flip image vertically when loaded. */
        IMAGE_LOADER_FLIP_VERTICALLY_BIT = 0x00000010,
        /** Premultiply color values with the alpha value if an alpha channel is present. */
        IMAGE_LOADER_PREMULTIPLY_ALPHA = 0x00000020,
        /** Only load image metadata (size and format) */
        IMAGE_LOADER_METADATA_ONLY = 0x00000040,
    };

    /** Interface for defining loaders for different image formats. */
    class IImageLoader {
    public:
        /** Detect whether current loader can load given image
         * @param imageFileBytes Image data.
         * @return True if this loader supports the image format.
         */
        virtual bool CanLoad(BASE_NS::array_view<const uint8_t> imageFileBytes) const = 0;

        /** Load Image file from provided file with passed flags
         * @param file File where to load from
         * @param loadFlags Load flags. Combination of #ImageLoaderFlags
         * @return Result of the loading operation.
         */
        virtual LoadResult Load(IFile& file, uint32_t loadFlags) const = 0;

        /** Load image file from given data bytes
         * @param imageFileBytes Image data.
         * @param loadFlags Load flags. Combination of #ImageLoaderFlags
         * @return Result of the loading operation.
         */
        virtual LoadResult Load(BASE_NS::array_view<const uint8_t> imageFileBytes, uint32_t loadFlags) const = 0;

        /** Load animated image with given parameters
         * @param file File to load image from
         * @param loadFlags Load flags. Combination of #ImageLoaderFlags
         * @return Result of the loading operation.
         */
        virtual LoadAnimatedResult LoadAnimatedImage(IFile& file, uint32_t loadFlags) = 0;

        /** Load animated image with given parameters
         * @param imageFileBytes Image data
         * @param loadFlags Load flags. Combination of #ImageLoaderFlags
         * @return Result of the loading operation.
         */
        virtual LoadAnimatedResult LoadAnimatedImage(
            BASE_NS::array_view<const uint8_t> imageFileBytes, uint32_t loadFlags) = 0;

        /** Return a list of supported image formats.
         * @return List of supported image formats.
         */
        virtual BASE_NS::vector<ImageType> GetSupportedTypes() const = 0;

        struct Deleter {
            constexpr Deleter() noexcept = default;
            void operator()(IImageLoader* ptr) const
            {
                ptr->Destroy();
            }
        };
        using Ptr = BASE_NS::unique_ptr<IImageLoader, Deleter>;

    protected:
        IImageLoader() = default;
        virtual ~IImageLoader() = default;
        virtual void Destroy() = 0;
    };

    /** Information needed from the plugin for managing ImageLoaders. */
    struct ImageLoaderTypeInfo : public ITypeInfo {
        /** TypeInfo UID for image loader type info. */
        static constexpr BASE_NS::Uid UID { "d6846818-5083-43fc-b9ff-28905a2b4ae2" };

        using CreateLoaderFn = IImageLoader::Ptr (*)(PluginToken);
        /* Token passed to loader creation (e.g. plugin specific data). */
        const PluginToken token;
        /** Unique ID of the image loader. */
        const BASE_NS::Uid uid;
        /** Pointer to function which is used to create loader instances. */
        const CreateLoaderFn createLoader;
        /** List of formats the loader supports. */
        const BASE_NS::array_view<const ImageType> imageTypes;
    };

    // Prevent copy construction and assign.
    IImageLoaderManager(IImageLoaderManager const&) = delete;
    IImageLoaderManager& operator=(IImageLoaderManager const&) = delete;

    /** Register image loader for image manager
     * @param imageLoader Image loader to be registered
     */
    [[deprecated("Use IPluginRegister::Register/UnregisterTypeInfo with ImageLoaderTypeInfo.")]] virtual void
    RegisterImageLoader(IImageLoader::Ptr imageLoader) = 0;

    /** Load image with given parameters
     * @param uri Uri to image
     * @param loadFlags Load flags. Combination of #ImageLoaderFlags
     */
    virtual LoadResult LoadImage(const BASE_NS::string_view uri, uint32_t loadFlags) = 0;

    /** Load image with given parameters
     * @param file File to load image from
     * @param loadFlags Load flags. Combination of #ImageLoaderFlags
     */
    virtual LoadResult LoadImage(IFile& file, uint32_t loadFlags) = 0;

    /** Load image with given parameters
     * @param imageFileBytes Image data
     * @param loadFlags Load flags. Combination of #ImageLoaderFlags
     */
    virtual LoadResult LoadImage(BASE_NS::array_view<const uint8_t> imageFileBytes, uint32_t loadFlags) = 0;

    /** Load animated image with given parameters
     * @param uri Uri to image
     * @param loadFlags Load flags. Combination of #ImageLoaderFlags
     */
    virtual LoadAnimatedResult LoadAnimatedImage(const BASE_NS::string_view uri, uint32_t loadFlags) = 0;

    /** Load animated image with given parameters
     * @param file File to load image from
     * @param loadFlags Load flags. Combination of #ImageLoaderFlags
     */
    virtual LoadAnimatedResult LoadAnimatedImage(IFile& file, uint32_t loadFlags) = 0;

    /** Load animated image with given parameters
     * @param imageFileBytes Image data
     * @param loadFlags Load flags. Combination of #ImageLoaderFlags
     */
    virtual LoadAnimatedResult LoadAnimatedImage(
        BASE_NS::array_view<const uint8_t> imageFileBytes, uint32_t loadFlags) = 0;

    /** Return a list of supported image formats.
     * @return List of supported image formats.
     */
    virtual BASE_NS::vector<ImageType> GetSupportedTypes() const = 0;

protected:
    IImageLoaderManager() = default;
    virtual ~IImageLoaderManager() = default;
};
CORE_END_NAMESPACE()

#endif // API_CORE_IMAGE_IIMAGE_LOADER_MANAGER_H
