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

#ifndef META_SRC_SERIALIZATION_JSON_EXPORTER_H
#define META_SRC_SERIALIZATION_JSON_EXPORTER_H

#include "../base_object.h"
#include "exporter.h"

META_BEGIN_NAMESPACE()
namespace Serialization {

class JsonExporter : public Internal::BaseObjectFwd<JsonExporter, META_NS::ClassId::JsonExporter, IFileExporter> {
public:
    ReturnError Export(CORE_NS::IFile& output, const IObject::ConstPtr& object) override;
};

} // namespace Serialization
META_END_NAMESPACE()

#endif
