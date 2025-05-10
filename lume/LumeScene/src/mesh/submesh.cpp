#include "submesh.h"

#include "../entity_converting_value.h"
#include "../util.h"

SCENE_BEGIN_NAMESPACE()

bool SubMesh::InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    if (index_ == -1) {
        CORE_LOG_E("Submesh index is uninitialised");
        return false;
    }
    BASE_NS::string cpath = "MeshComponent.submeshes[" + BASE_NS::string(BASE_NS::to_string(index_)) + "]." + path;

    if (p->GetName() == "Material") {
        auto ep = object_->CreateProperty(cpath).GetResult();
        auto i = interface_cast<META_NS::IStackProperty>(p);
        return ep && i &&
               i->PushValue(META_NS::IValue::Ptr(
                   new InterfacePtrEntityValue<IMaterial>(ep, { object_->GetScene(), ClassId::Material })));
    }
    return AttachEngineProperty(p, cpath);
}

void SubMesh::OnPropertyChanged(const META_NS::IProperty& p)
{
    if (p.GetName() != "Name") {
        META_NS::Invoke<META_NS::IOnChanged>(event_);
    }
}

SCENE_END_NAMESPACE()
