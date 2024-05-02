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

#ifndef CORE__GLTF__GLTF2_LOADER_H
#define CORE__GLTF__GLTF2_LOADER_H

#include <3d/namespace.h>
#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unique_ptr.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class IFileManager;
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
namespace GLTF2 {
class Data;
/** Describes result of the parsing operation. */
struct LoadResult {
    LoadResult() = default;
    explicit LoadResult(BASE_NS::string&& error) : success(false), error(error) {}

    /** Indicates, whether the loading operation is successful. */
    bool success { true };

    /** In case of parsing error, contains the description of the error. */
    BASE_NS::string error;

    /* Loaded data. */
    BASE_NS::unique_ptr<Data> data;
};
LoadResult LoadGLTF(CORE_NS::IFileManager& fileManager, BASE_NS::string_view uri);
LoadResult LoadGLTF(CORE_NS::IFileManager& fileManager, const BASE_NS::array_view<uint8_t const> data);
} // namespace GLTF2
CORE3D_END_NAMESPACE()

#endif // CORE__GLTF__GLTF2_LOADER_H
