#include "postprocess.h"

#include <3d/ecs/components/name_component.h>

#include "../converting_value.h"
#include "bloom.h"
#include "tonemap.h"
#include "util.h"

SCENE_BEGIN_NAMESPACE()

CORE_NS::Entity PostProcess::CreateEntity(const IInternalScene::Ptr& scene)
{
    auto& ecs = scene->GetEcsContext();
    auto pp = ecs.FindComponent<CORE3D_NS::PostProcessComponent>();
    if (!pp) {
        return {};
    }
    auto nameManager = ecs.FindComponent<CORE3D_NS::NameComponent>();
    if (!nameManager) {
        return {};
    }
    auto ent = ecs.GetNativeEcs()->GetEntityManager().Create();
    pp->Create(ent);
    nameManager->Create(ent);
    return ent;
}
bool PostProcess::SetEcsObject(const IEcsObject::Ptr& obj)
{
    auto p = META_NS::GetObjectRegistry().Create<IInternalPostProcess>(ClassId::PostProcessComponent);
    if (auto acc = interface_cast<IEcsObjectAccess>(p)) {
        if (acc->SetEcsObject(obj)) {
            pp_ = p;
        }
    }
    return pp_ != nullptr;
}
IEcsObject::Ptr PostProcess::GetEcsObject() const
{
    auto acc = interface_cast<IEcsObjectAccess>(pp_);
    return acc ? acc->GetEcsObject() : nullptr;
}
bool PostProcess::InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    auto& r = META_NS::GetObjectRegistry();
    if (p->GetName() == "Tonemap") {
        auto obj = r.Create<ITonemap>(ClassId::Tonemap);
        if (auto i = interface_cast<IPPEffectInit>(obj)) {
            i->Init(pp_->EnableFlags());
        }
        if (auto access = interface_cast<IEcsObjectAccess>(obj)) {
            access->SetEcsObject(GetEcsObject());
        }
        if (META_NS::Property<ITonemap::Ptr> prop { p }) {
            prop->SetValue(obj);
            return true;
        }
    }
    if (p->GetName() == "Bloom") {
        auto obj = r.Create<IBloom>(ClassId::Bloom);
        if (auto i = interface_cast<IPPEffectInit>(obj)) {
            i->Init(pp_->EnableFlags());
        }
        if (auto access = interface_cast<IEcsObjectAccess>(obj)) {
            access->SetEcsObject(GetEcsObject());
        }
        if (META_NS::Property<IBloom::Ptr> prop { p }) {
            prop->SetValue(obj);
            return true;
        }
    }
    return false;
}
SCENE_END_NAMESPACE()
