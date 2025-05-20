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

#ifndef SCENE_INTERFACE_ITEXTURE_H
#define SCENE_INTERFACE_ITEXTURE_H

#include <scene/base/namespace.h>
#include <scene/interface/intf_image.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

/** Filter */
enum class SamplerFilter : uint32_t {
    /** Nearest */
    NEAREST = 0,
    /** Linear */
    LINEAR = 1,
};

/** Sampler address mode */
enum class SamplerAddressMode : uint32_t {
    /** Repeat */
    REPEAT = 0,
    /** Mirrored repeat */
    MIRRORED_REPEAT = 1,
    /** Clamp to edge */
    CLAMP_TO_EDGE = 2,
    /** Clamp to border */
    CLAMP_TO_BORDER = 3,
    /** Mirror clamp to edge */
    MIRROR_CLAMP_TO_EDGE = 4,
};

/**
 * @brief The ISampler interface defines properties for a sampler.
 */
class ISampler : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISampler, "e4ecf173-e07f-4534-a02d-a5eee9b43a97")
public:
    /**
     * @brief Magnification filter
     */
    META_PROPERTY(SamplerFilter, MagFilter)
    /**
     * @brief Minification filter
     */
    META_PROPERTY(SamplerFilter, MinFilter)
    /**
     * @brief Mip map filter filter
     */
    META_PROPERTY(SamplerFilter, MipMapMode)
    /**
     * @brief Address mode U
     */
    META_PROPERTY(SamplerAddressMode, AddressModeU)
    /**
     * @brief Address mode V
     */
    META_PROPERTY(SamplerAddressMode, AddressModeV)
    /**
     * @brief Address mode W
     */
    META_PROPERTY(SamplerAddressMode, AddressModeW)
};

/**
 * @brief The ITexture interface defines material property properties.
 */
class ITexture : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITexture, "38c66860-d325-4256-becd-ffd2f94fbd8a")
public:
    /**
     * @brief Texture Image
     */
    META_PROPERTY(IImage::Ptr, Image)
    /**
     * @brief Sampler to be used with the texture.
     */
    META_READONLY_PROPERTY(ISampler::Ptr, Sampler)
    /**
     * @brief Factor
     */
    META_PROPERTY(BASE_NS::Math::Vec4, Factor)
    /**
     * @brief Translation of the texture.
     */
    META_PROPERTY(BASE_NS::Math::Vec2, Translation)
    /**
     * @brief Rotation of the texture.
     */
    META_PROPERTY(float, Rotation)
    /**
     * @brief Scale of the texture.
     */
    META_PROPERTY(BASE_NS::Math::Vec2, Scale)
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::SamplerFilter)
META_TYPE(SCENE_NS::SamplerAddressMode)
META_INTERFACE_TYPE(SCENE_NS::ITexture)
META_INTERFACE_TYPE(SCENE_NS::ISampler)

#endif
