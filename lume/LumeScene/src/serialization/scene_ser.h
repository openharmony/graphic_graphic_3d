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

#ifndef SCENE_SRC_SERIALIZATION_SCENE_SER_H
#define SCENE_SRC_SERIALIZATION_SCENE_SER_H

#include <scene/interface/intf_scene.h>

#include <core/resources/intf_resource_manager.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/minimal_object.h>
#include <meta/ext/object.h>
#include <meta/ext/object_container.h>
#include <meta/ext/serialization/serializer.h>
#include <meta/interface/serialization/intf_serializable.h>

SCENE_BEGIN_NAMESPACE()

class ISceneNodeSer : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISceneNodeSer, "183fa566-f548-4257-949f-0ae873785264")
public:
    virtual BASE_NS::string GetName() const = 0;
    virtual void SetName(BASE_NS::string_view name) = 0;
    virtual META_NS::ObjectId GetId() const = 0;
    virtual void SetId(META_NS::ObjectId id) = 0;
    virtual void SetAttachments(BASE_NS::vector<META_NS::IObject::Ptr> attas) = 0;
    virtual BASE_NS::vector<META_NS::IObject::Ptr> GetAttachments() const = 0;
};

class ISceneSer : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISceneSer, "a3c99c1d-483c-4642-a90b-c7d2f8475e68")
public:
    virtual void SetResourceGroups(BASE_NS::vector<BASE_NS::string>) = 0;
    virtual BASE_NS::vector<BASE_NS::string> GetResourceGroups() const = 0;
};

class IExternalAttachment : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IExternalAttachment, "a0fe2a52-eff5-4ad7-b846-ac438032699f")
public:
    virtual BASE_NS::string GetPath() const = 0;
    virtual void SetPath(BASE_NS::string_view path) = 0;
    virtual META_NS::IObject::Ptr GetAttachment() const = 0;
    virtual void SetAttachment(META_NS::IObject::Ptr) = 0;
};

class ISceneExternalNodeSer : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISceneExternalNodeSer, "981df612-a670-493b-987a-76180303c888")
public:
    virtual BASE_NS::string GetName() const = 0;
    virtual void SetName(BASE_NS::string_view name) = 0;
    virtual CORE_NS::ResourceId GetResourceId() const = 0;
    virtual void SetResourceId(CORE_NS::ResourceId id) = 0;
    virtual BASE_NS::vector<IExternalAttachment::Ptr> GetAttachments() const = 0;
    virtual void AddAttachment(BASE_NS::string_view path, META_NS::IObject::Ptr) = 0;
    virtual void AddSubresourceGroup(BASE_NS::string_view group) = 0;
    virtual BASE_NS::vector<BASE_NS::string> GetSubresourceGroups() const = 0;
};

META_REGISTER_CLASS(SceneSer, "ae6addfc-5e8e-4c2d-8653-2bd1dca4be6c", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(SceneNodeSer, "f339e04b-317d-4527-8d3c-460467ff43da", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(
    ExternalAttachment, "781963d3-85d7-4a79-8a1b-90377d61ef8a", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(
    SceneExternalNodeSer, "08c7759d-72f4-40a2-a437-04b00e0dead5", META_NS::ObjectCategoryBits::NO_CATEGORY)

class SceneNodeSer
    : public META_NS::IntroduceInterfaces<META_NS::CommonObjectContainerFwd, ISceneNodeSer, META_NS::ISerializable> {
    META_OBJECT(SceneNodeSer, ClassId::SceneNodeSer, IntroduceInterfaces)
public:
    BASE_NS::string GetName() const override
    {
        return name_;
    }
    void SetName(BASE_NS::string_view name) override
    {
        name_ = name;
    }
    META_NS::ObjectId GetId() const override
    {
        return id_;
    }
    void SetId(META_NS::ObjectId id) override
    {
        id_ = id;
    }
    void SetAttachments(BASE_NS::vector<META_NS::IObject::Ptr> attas) override
    {
        attas_ = BASE_NS::move(attas);
    }
    BASE_NS::vector<META_NS::IObject::Ptr> GetAttachments() const override
    {
        return attas_;
    }

    META_NS::ReturnError Export(META_NS::IExportContext& c) const override
    {
        // the name is automatically serialised for objects using the GetName in IObject
        return META_NS::Serializer(c) & META_NS::AutoSerialize() & META_NS::NamedValue("id", id_) &
               META_NS::NamedValue("attachments", attas_);
    }
    META_NS::ReturnError Import(META_NS::IImportContext& c) override
    {
        META_NS::Serializer res(c);
        if (res & META_NS::AutoSerialize() & META_NS::NamedValue("id", id_) &
            META_NS::NamedValue("attachments", attas_)) {
            name_ = c.GetName();
        }
        return res;
    }

private:
    BASE_NS::string name_;
    META_NS::ObjectId id_;
    BASE_NS::vector<META_NS::IObject::Ptr> attas_;
};

class SceneSer : public META_NS::IntroduceInterfaces<SceneNodeSer, ISceneSer> {
    META_OBJECT(SceneSer, ClassId::SceneSer, IntroduceInterfaces)
public:
    void SetResourceGroups(BASE_NS::vector<BASE_NS::string> g) override
    {
        groups_ = BASE_NS::move(g);
    }
    BASE_NS::vector<BASE_NS::string> GetResourceGroups() const override
    {
        return groups_;
    }

    META_NS::ReturnError Export(META_NS::IExportContext& c) const override
    {
        auto res = Super::Export(c);
        if (res) {
            res = META_NS::Serializer(c) & META_NS::NamedValue("resourceGroups", groups_);
        }
        return res;
    }
    META_NS::ReturnError Import(META_NS::IImportContext& c) override
    {
        auto res = Super::Import(c);
        if (res) {
            res = META_NS::Serializer(c) & META_NS::NamedValue("resourceGroups", groups_);
        }
        return res;
    }

private:
    BASE_NS::vector<BASE_NS::string> groups_;
};

class SceneExternalNodeSer
    : public META_NS::IntroduceInterfaces<META_NS::MetaObject, ISceneExternalNodeSer, META_NS::ISerializable> {
    META_OBJECT(SceneExternalNodeSer, ClassId::SceneExternalNodeSer, IntroduceInterfaces)
public:
    BASE_NS::string GetName() const override
    {
        return name_;
    }
    void SetName(BASE_NS::string_view name) override
    {
        name_ = name;
    }
    CORE_NS::ResourceId GetResourceId() const override
    {
        return id_;
    }
    void SetResourceId(CORE_NS::ResourceId id) override
    {
        id_ = id;
    }

    BASE_NS::vector<IExternalAttachment::Ptr> GetAttachments() const override
    {
        return attachments_;
    }
    void AddAttachment(BASE_NS::string_view path, META_NS::IObject::Ptr obj) override
    {
        auto ea = META_NS::GetObjectRegistry().Create<IExternalAttachment>(ClassId::ExternalAttachment);
        if (ea) {
            ea->SetPath(path);
            ea->SetAttachment(obj);
            attachments_.push_back(ea);
        }
    }

    void AddSubresourceGroup(BASE_NS::string_view group) override
    {
        subresourceGroups_.push_back(BASE_NS::string(group));
    }
    BASE_NS::vector<BASE_NS::string> GetSubresourceGroups() const override
    {
        return subresourceGroups_;
    }

    META_NS::ReturnError Export(META_NS::IExportContext& c) const override
    {
        // the name is automatically serialised for objects using the GetName in IObject
        return META_NS::Serializer(c) & META_NS::AutoSerialize() & META_NS::NamedValue("resourceId.name", id_.name) &
               META_NS::NamedValue("resourceId.group", id_.group) & META_NS::NamedValue("attachments", attachments_) &
               META_NS::NamedValue("subresourceGroups", subresourceGroups_);
    }
    META_NS::ReturnError Import(META_NS::IImportContext& c) override
    {
        META_NS::Serializer res(c);
        if (res & META_NS::AutoSerialize() & META_NS::NamedValue("resourceId.name", id_.name) &
            META_NS::NamedValue("resourceId.group", id_.group) & META_NS::NamedValue("attachments", attachments_)) {
            name_ = c.GetName();
            if (c.HasMember("subresourceGroups")) {
                res& META_NS::NamedValue("subresourceGroups", subresourceGroups_);
            }
        }
        return res;
    }

private:
    BASE_NS::string name_;
    CORE_NS::ResourceId id_;
    BASE_NS::vector<IExternalAttachment::Ptr> attachments_;
    BASE_NS::vector<BASE_NS::string> subresourceGroups_;
};

class ExternalAttachment
    : public META_NS::IntroduceInterfaces<META_NS::MinimalObject, IExternalAttachment, META_NS::ISerializable> {
    META_OBJECT(ExternalAttachment, ClassId::ExternalAttachment, IntroduceInterfaces)
public:
    BASE_NS::string GetName() const override
    {
        return path_;
    }
    BASE_NS::string GetPath() const override
    {
        return path_;
    }
    void SetPath(BASE_NS::string_view path) override
    {
        path_ = path;
    }
    META_NS::IObject::Ptr GetAttachment() const override
    {
        return obj_;
    }
    void SetAttachment(META_NS::IObject::Ptr obj) override
    {
        obj_ = obj;
    }
    META_NS::ReturnError Export(META_NS::IExportContext& c) const override
    {
        return META_NS::Serializer(c) & META_NS::NamedValue("object", obj_);
    }
    META_NS::ReturnError Import(META_NS::IImportContext& c) override
    {
        META_NS::Serializer res(c);
        if (res & META_NS::NamedValue("object", obj_)) {
            path_ = c.GetName();
        }
        return res;
    }

private:
    BASE_NS::string path_;
    META_NS::IObject::Ptr obj_;
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IExternalAttachment)

#endif
