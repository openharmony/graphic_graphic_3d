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

#ifndef SCENE_BASE_TYPES_H
#define SCENE_BASE_TYPES_H

#include <scene/base/namespace.h>

#include <meta/api/future.h>
#include <meta/base/interface_macros.h>
#include <meta/base/types.h>
#include <meta/interface/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

using META_NS::Future;

enum class RenderMode {
    /// Render a frame if something changed in the scene (When calling internal scene update)
    IF_DIRTY,
    /// Always render new frame even if nothing changed
    ALWAYS,
    /// Only render a frame if explicitly requested
    MANUAL
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::RenderMode);

#endif
