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
#ifndef META_API_OBJECT_NAME_H
#define META_API_OBJECT_NAME_H

#include <meta/api/util.h>
#include <meta/base/interface_traits.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_attach.h>
#include <meta/interface/intf_container.h>
#include <meta/interface/intf_named.h>
#include <meta/interface/intf_object.h>

META_BEGIN_NAMESPACE()

template<typename Interface, typename = BASE_NS::enable_if<IsKindOfPointer_v<Interface>>>
BASE_NS::string GetName(const Interface& obj)
{
    if (auto o = interface_pointer_cast<IObject>(obj)) {
        return o->GetName();
    }
    // in case we are not an object but implement INamed
    if (auto named = interface_pointer_cast<INamed>(obj)) {
        return META_NS::GetValue(named->Name());
    }
    return "";
}

inline IObjectName::Ptr GetObjectNameAttachment(const IAttach::ConstPtr& att)
{
    if (auto cont = att->GetAttachmentContainer(false)) {
        auto res = cont->FindAny({ "", TraversalType::NO_HIERARCHY, { IObjectName::UID }, false });
        if (auto oname = interface_pointer_cast<IObjectName>(res)) {
            return oname;
        }
    }
    return nullptr;
}

template<typename Interface, typename = BASE_NS::enable_if<IsKindOfPointer_v<Interface>>>
bool SetName(const Interface& obj, BASE_NS::string_view name)
{
    if (auto named = interface_pointer_cast<INamed>(obj)) {
        META_NS::SetValue(named->Name(), BASE_NS::string(name));
        return true;
    }
    if (auto att = interface_pointer_cast<IAttach>(obj)) {
        if (auto oname = GetObjectNameAttachment(att)) {
            return oname->SetName(name);
        } else {
            oname = GetObjectRegistry().Create<IObjectName>(ClassId::ObjectName);
            if (oname && oname->SetName(name)) {
                return att->Attach(oname);
            }
        }
    }
    return false;
}

META_END_NAMESPACE()

#endif
