/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE__GLTF__GLTF2_EXPORTER_H
#define CORE__GLTF__GLTF2_EXPORTER_H

#include <3d/namespace.h>
#include <base/containers/string.h>
#include <base/containers/type_traits.h>
#include <base/containers/unique_ptr.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class IEcs;
class IEngine;
class IFile;
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
namespace GLTF2 {
class Data;

/** Describes result of the export operation. */
struct ExportResult final {
    ExportResult() = default;
    ~ExportResult();
    ExportResult(ExportResult&&) = default;
    ExportResult& operator=(ExportResult&&) = default;
    explicit ExportResult(BASE_NS::string&& error) : success(false), error(error) {}
    explicit ExportResult(BASE_NS::unique_ptr<Data>&& data) : data(BASE_NS::move(data)) {}

    /** Indicates, whether the export operation is successful. */
    bool success { true };

    /** The description of the error. */
    BASE_NS::string error;

    /* Loaded data. */
    BASE_NS::unique_ptr<Data> data;
};

void SaveGLB(Data const& data, CORE_NS::IFile& file, BASE_NS::string_view versionString);
void SaveGLTF(Data const& data, CORE_NS::IFile& file, BASE_NS::string_view versionString);
ExportResult ExportGLTF(CORE_NS::IEngine& engine, const CORE_NS::IEcs& ecs);
} // namespace GLTF2
CORE3D_END_NAMESPACE()

#endif // CORE__GLTF__GLTF2_EXPORTER_H
