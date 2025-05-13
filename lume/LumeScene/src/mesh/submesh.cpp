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
