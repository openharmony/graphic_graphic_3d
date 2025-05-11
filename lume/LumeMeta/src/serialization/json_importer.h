/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2024. All rights reserved.
 * Description: Exporter
 * Author: Mikael Kilpeläinen
 * Create: 2024-01-03
 */

#ifndef META_SRC_SERIALIZATION_JSON_IMPORTER_H
#define META_SRC_SERIALIZATION_JSON_IMPORTER_H

#include "../base_object.h"
#include "importer.h"

META_BEGIN_NAMESPACE()
namespace Serialization {

class JsonImporter : public IntroduceInterfaces<BaseObject, IFileImporter> {
    META_OBJECT(JsonImporter, ClassId::JsonImporter, IntroduceInterfaces)
public:
    JsonImporter();

    IObject::Ptr Import(const ISerNode::ConstPtr& tree) override;
    IObject::Ptr Import(CORE_NS::IFile& input) override;
    ISerNode::Ptr ImportAsTree(CORE_NS::IFile& input) override;

    BASE_NS::vector<ISerTransformation::Ptr> GetTransformations() const override;
    void SetTransformations(BASE_NS::vector<ISerTransformation::Ptr>) override;

    BASE_NS::unordered_map<InstanceId, InstanceId> GetInstanceIdMapping() const override;
    void SetResourceManager(CORE_NS::IResourceManager::Ptr) override;
    void SetUserContext(IObject::Ptr) override;
    SerMetadata GetMetadata() const override;

private:
    ISerNode::Ptr Transform(ISerNode::Ptr tree, const Version& ver);

private:
    Importer imp_;
    BASE_NS::vector<ISerTransformation::Ptr> transformations_;
};

} // namespace Serialization
META_END_NAMESPACE()

#endif
