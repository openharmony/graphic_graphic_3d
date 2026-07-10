/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SCENE_IMP_SRC_RESOLVE_OBJECT_H
#define SCENE_IMP_SRC_RESOLVE_OBJECT_H

#include "import_context.h"

SCENE_IMP_BEGIN_NAMESPACE()

/// Resolve `path` against `base`. When `parent` is non-null and the path
/// terminates at a property, `*parent` receives the object on which the
/// terminal property was looked up (the wrapper for forwarded properties).
ImportResult ResolveObject(ImportContext&, const META_NS::IObject::Ptr& base, BASE_NS::string_view path,
    bool onlyChildren = false, META_NS::IObject::Ptr* parent = nullptr);

SCENE_IMP_END_NAMESPACE()

#endif