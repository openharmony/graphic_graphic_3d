/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef SCENE_IMP_SRC_LOADERS_RESOURCE_TYPE_BASE_H
#define SCENE_IMP_SRC_LOADERS_RESOURCE_TYPE_BASE_H

#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/util.h>
#include <scene/interface/intf_render_context.h>
#include <scene/interface/resource/intf_resource_context.h>
#include <scene/interface/resource/types.h>
#include <scene_importer/base/namespace.h>
#include <scene_importer/interface/intf_importer.h>

#include <core/resources/intf_resource_manager.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/ext/resource/resource.h>

SCENE_IMP_BEGIN_NAMESPACE()

class ResourceTypeBase : public META_NS::IntroduceInterfaces<META_NS::BaseObject, CORE_NS::IResourceType> {
    using Super = IntroduceInterfaces;

public:
    explicit ResourceTypeBase(const META_NS::ClassInfo& info) : classInfo_(info)
    {}

    bool Build(const META_NS::IMetadata::Ptr& d) override
    {
        bool res = Super::Build(d);
        if (res) {
            auto importer = SCENE_NS::GetInterfaceBuildArg<IImporter>(d, "Importer");
            if (!importer) {
                CORE_LOG_E("Invalid arguments to construct %s-type", GetResourceName().c_str());
                return false;
            }
            importer_ = importer;
        }
        return res;
    }

    bool ReloadResource(const StorageInfo& s, const CORE_NS::IResource::Ptr& res) const override
    {
        return false;
    }

    bool SaveResource(const CORE_NS::IResource::ConstPtr& res, const StorageInfo& s) const override
    {
        return false;
    }

    BASE_NS::string GetResourceName() const override
    {
        return BASE_NS::string(classInfo_.Name());
    }
    BASE_NS::Uid GetResourceType() const override
    {
        return classInfo_.Id().ToUid();
    }

    BASE_NS::string GetVersion() const override
    {
        return {"JsonSchemaImport"};
    }

protected:
    IImporter::WeakPtr importer_;
    META_NS::ClassInfo classInfo_;
};

class SceneResourceTypeBase : public ResourceTypeBase {
public:
    explicit SceneResourceTypeBase(const META_NS::ClassInfo& info, const META_NS::ClassInfo& instance)
        : ResourceTypeBase(info), instanceInfo_(instance)
    {}

    CORE_NS::IResource::Ptr LoadResource(const StorageInfo& s) const override
    {
        auto rcontext = interface_cast<SCENE_NS::IResourceContext>(s.context);
        auto scene = rcontext ? rcontext->GetScene() : interface_pointer_cast<SCENE_NS::IScene>(s.context);
        if (!scene) {
            CORE_LOG_E("Missing scene/resource context when loading %s resource: %s",
                GetResourceName().c_str(),
                s.id.ToString().c_str());
            return nullptr;
        }
        auto is = scene->GetInternalScene();
        CORE_NS::IResource::Ptr res = is->RunDirectlyOrInTask([this, &is, &s]() {
            return interface_pointer_cast<CORE_NS::IResource>(is->CreateObject(instanceInfo_, s.id));
        });
        if (res && s.options) {
            s.options->ApplyOptions(*res, s.context);
        }
        return res;
    }
    bool ReloadResource(const StorageInfo& s, const CORE_NS::IResource::Ptr& res) const override
    {
        return res && s.options && s.options->ApplyOptions(*res, s.context);
    }
    bool SaveResource(const CORE_NS::IResource::ConstPtr& res, const StorageInfo& s) const override
    {
        return res && s.options && s.options->UpdateOptions(*res, s.context);
    }

private:
    META_NS::ClassInfo instanceInfo_;
};

class SceneResourceTemplateTypeBase : public ResourceTypeBase {
public:
    explicit SceneResourceTemplateTypeBase(const META_NS::ClassInfo& info) : ResourceTypeBase(info)
    {}

    CORE_NS::IResource::Ptr LoadResource(const StorageInfo& s) const
    {
        if (!s.payload) {
            return nullptr;
        }
        auto importer = importer_.lock();
        if (!importer) {
            CORE_LOG_E("Importer not available when loading %s resource: %s",
                GetResourceName().c_str(),
                s.id.ToString().c_str());
            return nullptr;
        }
        ImportParameters params;
        params.filename = s.path;
        auto impRes = importer->Import(*s.payload, params);
        auto res = interface_pointer_cast<CORE_NS::IResource>(impRes.object);
        if (res && s.options) {
            s.options->ApplyOptions(*res, s.context);
        }
        return res;
    }
    bool ReloadResource(const StorageInfo& s, const CORE_NS::IResource::Ptr& res) const
    {
        return res && s.options && s.options->ApplyOptions(*res, s.context);
    }
    bool SaveResource(const CORE_NS::IResource::ConstPtr& res, const StorageInfo& s) const
    {
        return res && s.options && s.options->UpdateOptions(*res, s.context);
    }
};

SCENE_IMP_END_NAMESPACE()

#endif
