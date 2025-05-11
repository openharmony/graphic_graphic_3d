/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2024. All rights reserved.
 * Description: Exporter
 * Author: Mikael Kilpeläinen
 * Create: 2024-01-03
 */

#ifndef META_SRC_SERIALIZATION_JSON_EXPORTER_H
#define META_SRC_SERIALIZATION_JSON_EXPORTER_H

#include "../base_object.h"
#include "exporter.h"

META_BEGIN_NAMESPACE()
namespace Serialization {

class JsonExporter : public IntroduceInterfaces<BaseObject, IFileExporter> {
    META_OBJECT(JsonExporter, ClassId::JsonExporter, IntroduceInterfaces)
public:
    ReturnError Export(CORE_NS::IFile& output, const IObject::ConstPtr& object) override;
    ISerNode::Ptr Export(const IObject::ConstPtr& object) override;
    void SetInstanceIdMapping(BASE_NS::unordered_map<InstanceId, InstanceId>) override;
    void SetResourceManager(CORE_NS::IResourceManager::Ptr) override;
    void SetUserContext(IObject::Ptr) override;
    void SetMetadata(SerMetadata m) override;

private:
    Exporter exp_;
};

} // namespace Serialization
META_END_NAMESPACE()

#endif
