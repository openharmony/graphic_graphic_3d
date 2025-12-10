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

JsonImporter::JsonImporter() : transformations_ { CreateShared<MetaMigrateV1>() } {}

IObject::Ptr JsonImporter::Import(const ISerNode::ConstPtr& tree, ImportOptions opts)
{
    return imp_.Import(tree, BASE_NS::move(opts));
}

ISerNode::Ptr JsonImporter::ImportAsTree(CORE_NS::IFile& input)
{
    ISerNode::Ptr tree;
    BASE_NS::string data;
    data.resize(input.GetLength());
    if (input.Read(data.data(), data.size()) == data.size()) {
        JsonInput json;
        tree = json.Process(data);
        if (tree) {
            tree = Transform(tree, json.GetVersion());
        }
    }
    return tree;
}

IObject::Ptr JsonImporter::Import(CORE_NS::IFile& input, ImportOptions opts)
{
    if (auto tree = ImportAsTree(input)) {
        return Import(tree, BASE_NS::move(opts));
    }
    return nullptr;
}

ISerNode::Ptr JsonImporter::Transform(ISerNode::Ptr tree, const Version& ver)
{
    bool transformed = false;
    for (auto&& t : transformations_) {
        if (ver < t->ApplyForLessThanVersion()) {
            transformed = true;
            tree = t->Process(tree);
            if (!tree) {
                CORE_LOG_E("Transforming serialisation tree failed (step: %s)", t->GetName().c_str());
                return nullptr;
            }
        }
    }
#ifdef _DEBUG
    // --- debug
    if (transformed) {
        JsonOutput backend;
        auto json = backend.Process(tree);
        if (json.empty()) {
            CORE_LOG_E("Fail");
            return nullptr;
        }
        auto f = CORE_NS::GetPluginRegister().GetFileManager().CreateFile("file://./rewrite.json");
        f->Write(json.c_str(), json.size());
    }
    // ---
#endif
    return tree;
}

BASE_NS::vector<ISerTransformation::Ptr> JsonImporter::GetTransformations() const
{
    return transformations_;
}
void JsonImporter::SetTransformations(BASE_NS::vector<ISerTransformation::Ptr> t)
{
    transformations_ = BASE_NS::move(t);
}
BASE_NS::unordered_map<InstanceId, InstanceId> JsonImporter::GetInstanceIdMapping() const
{
    return imp_.GetInstanceIdMapping();
}
void JsonImporter::SetResourceManager(CORE_NS::IResourceManager::Ptr p)
{
    imp_.SetResourceManager(BASE_NS::move(p));
}
void JsonImporter::SetUserContext(IObject::Ptr p)
{
    imp_.SetUserContext(BASE_NS::move(p));
}
SerMetadata JsonImporter::GetMetadata() const
{
    return imp_.GetMetadata();
}
void JsonImporter::ResolveDeferred()
{
    imp_.ResolveDeferred();
}
} // namespace Serialization
META_END_NAMESPACE()
