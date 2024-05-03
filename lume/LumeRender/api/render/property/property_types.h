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

#ifndef API_RENDER_PROPERTY_PROPERTY_TYPES_H
#define API_RENDER_PROPERTY_PROPERTY_TYPES_H

#include <cstdint>

#include <core/property/property.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/namespace.h>

CORE_BEGIN_NAMESPACE()
namespace PropertyType {
inline constexpr CORE_NS::PropertyTypeDecl BINDABLE_BUFFER_WITH_HANDLE_REFERENCE_T =
    PROPERTYTYPE(RENDER_NS::BindableBufferWithHandleReference);
inline constexpr CORE_NS::PropertyTypeDecl BINDABLE_IMAGE_WITH_HANDLE_REFERENCE_T =
    PROPERTYTYPE(RENDER_NS::BindableImageWithHandleReference);
inline constexpr CORE_NS::PropertyTypeDecl BINDABLE_SAMPLER_WITH_HANDLE_REFERENCE_T =
    PROPERTYTYPE(RENDER_NS::BindableSamplerWithHandleReference);
} // namespace PropertyType

#ifdef DECLARE_PROPERTY_TYPE
DECLARE_PROPERTY_TYPE(RENDER_NS::BindableBufferWithHandleReference);
DECLARE_PROPERTY_TYPE(RENDER_NS::BindableImageWithHandleReference);
DECLARE_PROPERTY_TYPE(RENDER_NS::BindableSamplerWithHandleReference);
#endif
CORE_END_NAMESPACE()
#endif // API_RENDER_PROPERTY_PROPERTY_TYPES_H