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

#ifndef SCENE_INTERFACE_IBITMAP_H
#define SCENE_INTERFACE_IBITMAP_H

#include <scene/base/namespace.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

class IBitmap : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IBitmap, "a9459966-38c4-46bb-bec5-3b5d96843b5e")
public:
    META_READONLY_PROPERTY(BASE_NS::Math::UVec2, Size)

};

META_REGISTER_CLASS(Bitmap, "77b38a92-8182-4562-bd3f-deab7b40cedc", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IBitmap)

#endif
