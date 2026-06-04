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

#ifndef SCENE_MIMP_SRC_LIGHT_PROBE_GROUP_SER_H
#define SCENE_MIMP_SRC_LIGHT_PROBE_GROUP_SER_H

#include <optional>
#include <scene/interface/intf_light_probe_group.h>
#include <scene_metadata_importer/base/namespace.h>

#include <meta/ext/attachment/attachment.h>
#include <meta/interface/serialization/intf_serializable.h>

SCENE_MIMP_BEGIN_NAMESPACE()

META_REGISTER_CLASS(
    LightProbeGroupSer, "a7c3e1d2-8f45-4b9a-b2c7-3d1e5f6a8b0c", META_NS::ObjectCategoryBits::NO_CATEGORY)

class LightProbeGroupSer : public META_NS::IntroduceInterfaces<META_NS::AttachmentFwd, META_NS::ISerializable> {
    META_OBJECT(LightProbeGroupSer, ClassId::LightProbeGroupSer, IntroduceInterfaces)

public:
    bool Build(const META_NS::IMetadata::Ptr& d) override
    {
        if (!Super::Build(d)) {
            return false;
        }
        META_NS::SetObjectFlags(GetSelf(), META_NS::ObjectFlagBits::SERIALIZE, true);
        return true;
    }

    META_NS::ReturnError Export(META_NS::IExportContext& c) const override;
    META_NS::ReturnError Import(META_NS::IImportContext& c) override;

protected:
    bool AttachTo(const META_NS::IAttach::Ptr& target, const META_NS::IObject::Ptr&) override;
    bool DetachFrom(const META_NS::IAttach::Ptr&) override;

private:
    std::optional<SCENE_NS::LightProbeGroup> importedGroup_;
};

SCENE_MIMP_END_NAMESPACE()

#endif  // SCENE_MIMP_SRC_LIGHT_PROBE_GROUP_SER_H
