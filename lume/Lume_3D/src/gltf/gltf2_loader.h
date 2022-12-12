/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
