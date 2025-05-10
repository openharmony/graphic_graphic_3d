
#include "postprocess_type.h"

#include <scene/ext/intf_render_resource.h>
#include <scene/ext/util.h>
#include <scene/interface/intf_scene.h>

#include <meta/api/metadata_util.h>

#include "../../serialization/util.h"

SCENE_BEGIN_NAMESPACE()

bool PostProcessResourceType::Build(const META_NS::IMetadata::Ptr& d)
{
    bool res = Super::Build(d);
    if (res) {
        auto context = GetInterfaceBuildArg<IRenderContext>(d, "RenderContext");
        if (!context) {
            CORE_LOG_E("Invalid arguments to construct PostProcessResourceType");
            return false;
        }
        context_ = context;
    }
    return res;
}
BASE_NS::string PostProcessResourceType::GetResourceName() const
{
    return "PostProcess";
}
BASE_NS::Uid PostProcessResourceType::GetResourceType() const
{
    return ClassId::PostProcessResource.Id().ToUid();
}

CORE_NS::IResource::Ptr PostProcessResourceType::LoadResource(const StorageInfo& s) const
{
    auto scene = interface_pointer_cast<IScene>(s.context);
    if (!scene) {
        CORE_LOG_W("missing context, cannot load postprocess resource");
        return nullptr;
    }
    CORE_NS::IResource::Ptr res = scene->CreateObject<CORE_NS::IResource>(ClassId::PostProcess).GetResult();
    if (res && s.options) {
        if (auto opts = META_NS::GetObjectRegistry().Create<META_NS::IObjectResourceOptions>(
                META_NS::ClassId::ObjectResourceOptions)) {
            opts->Load(*s.options, s.self, s.context);
            if (auto i = interface_cast<META_NS::IDerivedFromResource>(res)) {
                auto base = opts->GetBaseResource();
                if (base.IsValid()) {
                    auto r = s.self->GetResource(base, s.context);
                    if (!r) {
                        CORE_LOG_W("Could not load base resource for postprocess %s", s.id.ToString().c_str());
                    }
                    i->SetResource(r);
                }
            }
            auto in = interface_cast<META_NS::IMetadata>(opts);
            auto out = interface_cast<META_NS::IMetadata>(res);
            if (in && out) {
                SerCopy(*in, *out);
            }
        }
    }
    return res;
}
bool PostProcessResourceType::SaveResource(const CORE_NS::IResource::ConstPtr& p, const StorageInfo& s) const
{
    if (s.options) {
        if (auto opts = META_NS::GetObjectRegistry().Create<META_NS::IObjectResourceOptions>(
                META_NS::ClassId::ObjectResourceOptions)) {
            auto in = interface_cast<META_NS::IMetadata>(p);
            auto out = interface_cast<META_NS::IMetadata>(opts);
            if (auto i = interface_cast<META_NS::IDerivedFromResource>(p)) {
                opts->SetBaseResource(i->GetResource());
            }
            if (in && out) {
                SerCloneAllToDefaultIfSet(*in, *out);
                opts->Save(*s.options, s.self);
            }
        }
    }
    return true;
}
bool PostProcessResourceType::ReloadResource(const StorageInfo&, const CORE_NS::IResource::Ptr&) const
{
    return false;
}

SCENE_END_NAMESPACE()