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

#ifndef LOADER_VERTEX_INPUT_DECLARATION_LOADER_H
#define LOADER_VERTEX_INPUT_DECLARATION_LOADER_H

#include <base/containers/string.h>
#include <core/namespace.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

CORE_BEGIN_NAMESPACE()
class IFileManager;
CORE_END_NAMESPACE()
RENDER_BEGIN_NAMESPACE()

/** Vertex input declaration loader.
 * A class that can be used to load vertex input declaration from json structure.
 */
class VertexInputDeclarationLoader final {
public:
    /** Describes result of the parsing operation. */
    struct LoadResult {
        LoadResult() = default;
        explicit LoadResult(const BASE_NS::string& error) : success(false), error(error) {}

        /** Indicates, whether the parsing operation is successful. */
        bool success { true };

        /** In case of parsing error, contains the description of the error. */
        BASE_NS::string error;
    };

    /** Retrieve uri of the vertex input declaration.
     * @return String view to uri of the vertex input declaration.
     */
    BASE_NS::string_view GetUri() const;

    /** Retrieve vertex input declaration.
     * @return A view to vertex input declaration structure, as defined in the json file.
     */
    VertexInputDeclarationView GetVertexInputDeclarationView() const;

    /** Loads vertex input declaration from json string.
     * @param jsonString A string containing valid json as content.
     * @return A structure containing result for the parsing operation.
     */
    LoadResult Load(BASE_NS::string_view jsonString);

    /** Loads vertex input declaration from given uri, using file manager.
     * @param fileManager A file manager to access the file in given uri.
     * @param uri Uri to json file.
     * @return A structure containing result for the parsing operation.
     */
    LoadResult Load(CORE_NS::IFileManager& fileManager, BASE_NS::string_view uri);

private:
    VertexInputDeclarationData vertexInputDeclarationData_;
    BASE_NS::string uri_;
};
RENDER_END_NAMESPACE()

#endif // LOADER_VERTEX_INPUT_DECLARATION_LOADER_H
