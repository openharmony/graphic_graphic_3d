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

#ifndef SCENE_INTERFACE_IIMAGE_H
#define SCENE_INTERFACE_IIMAGE_H

#include <scene/base/namespace.h>

#include <meta/api/future.h>
#include <meta/base/interface_macros.h>
#include <meta/interface/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

class IImage : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IImage, "a9459966-38c4-46bb-bec5-3b5d96843b5e")
public:
    META_READONLY_PROPERTY(BASE_NS::Math::UVec2, Size)
};

META_REGISTER_CLASS(Image, "77b38a92-8182-4562-bd3f-deab7b40cedc", META_NS::ObjectCategoryBits::NO_CATEGORY)

// compatibility with old
using IBitmap = IImage;
namespace ClassId {
[[maybe_unused]] inline constexpr META_NS::ClassInfo Bitmap = Image;
}

class IExternalImage : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IExternalImage, "c1841740-1308-460e-a995-66ed630a206d")
public:
    /**
     * @brief Input parameters for SetExternalBuffer.
     */
    struct ExternalBufferInfo {
        /**
         * @brief The platform-dependant hardware buffer.
         *        The buffer must be AHardwareBuffer in Android or OH_NativeBuffer in OHOS.
         *        Other platforms are not supported and calling this function returns false always.
         */
        uintptr_t buffer {};
        /**
         * @brief Size of the buffer in pixels.
         *        Automatically detected from buffer if {}.
         */
        BASE_NS::Math::UVec2 size {};
    };

    /**
     * @brief Sets given hardware buffer to be rendered as image
     * @return true if setting the gotten render handle to this image succeeds.
     */
    virtual bool SetExternalBuffer(const ExternalBufferInfo& buffer) = 0;
};

META_REGISTER_CLASS(ExternalImage, "b065b505-8d89-41e5-97ab-48085eac2a50", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IImage)
META_INTERFACE_TYPE(SCENE_NS::IExternalImage)

#endif
