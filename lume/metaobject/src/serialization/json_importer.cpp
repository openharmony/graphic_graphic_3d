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
#include "json_importer.h"

#include <core/io/intf_file_manager.h>

#include <meta/api/util.h>

#include "backend/json_input.h"
#include "backend/json_output.h"
#include "metav1_compat.h"

META_BEGIN_NAMESPACE()
namespace Serialization {

ISerNode::Ptr JsonImporter::ImportAsTree(CORE_NS::IFile& input)
{
    ISerNode::Ptr tree;
    BASE_NS::string data;
    data.resize(input.GetLength());
    if (input.Read(data.data(), data.size()) == data.size()) {
        JsonInput json;
        tree = json.Process(data);
        if (tree) {
            if (json.GetJsonVersion() < Version(2, 0)) {
                tree = RewriteMetaV1NodeTree(tree);
                if (!tree) {
                    CORE_LOG_E("Converting old meta v1 serialization data failed");
                    return nullptr;
                }
#ifdef _DEBUG
                /// debug
                JsonOutput backend;
                auto json = backend.Process(tree);
                if (json.empty()) {
                    CORE_LOG_E("Fail");
                    return nullptr;
                }
                auto f = CORE_NS::GetPluginRegister().GetFileManager().CreateFile("file://./rewrite.json");
                f->Write(json.c_str(), json.size());
                ///
#endif
            }
        }
    }
    return tree;
}

IObject::Ptr JsonImporter::Import(CORE_NS::IFile& input)
{
    if (auto tree = ImportAsTree(input)) {
        Importer imp;
        return imp.Import(tree);
    }
    return nullptr;
}

} // namespace Serialization
META_END_NAMESPACE()
