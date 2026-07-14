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

#include "attachment.h"

#include <meta/api/object_name.h>
#include <meta/api/util.h>
#include <meta/interface/intf_attach.h>

#include "../import_helpers.h"
#include "index.h"
#include "property.h"

SCENE_IMP_BEGIN_NAMESPACE()

IDiagnostics::Ptr ImportAttachment(ImportContext& context, const META_NS::IObject::Ptr& attachee)
{
    ErrorHandler h(context);
    auto id = GetOptObjectId(context, "objectUid");
    if (h.Handle(id.error)) {
        return id.error;
    }
    if (!id.value) {
        CORE_LOG_E("Attachment missing 'objectUid'");
        return context.CreateDiagnostics("Attachment missing 'objectUid'");
    }
    auto trace = context.Trace("Importing attachment [uid=" + id.value.value_or(META_NS::ObjectId{}).ToString() + "]");

    auto obj = META_NS::GetObjectRegistry().Create(*id.value);
    if (!obj) {
        CORE_LOG_E("Failed to create attachment (id: %s)", id.value.value_or(META_NS::ObjectId{}).ToString().c_str());
        return context.CreateDiagnostics(
            "Failed to create attachment: " + id.value.value_or(META_NS::ObjectId{}).ToString());
    }
    if (context.IsTemplateContext()) {
        META_NS::SetObjectFlags(obj, IMPORTED_FROM_TEMPLATE_BIT, true);
    }
    auto name = GetOptString(context, "name");
    if (h.HandleOptValue(name)) {
        if (name.error) {
            return name.error;
        }
    }
    if (name.value) {
        META_NS::SetName(obj, *name.value);
    }
    if (auto err = ImportProperties(context, *obj); h.Handle(err)) {
        return err;
    }
    if (auto attach = interface_cast<META_NS::IAttach>(attachee)) {
        attach->Attach(obj);
    }
    return h;
}

IDiagnostics::Ptr ImportAttachments(ImportContext& context, const META_NS::IObject::Ptr& attachee)
{
    ErrorHandler h(context);
    auto arr = context.GetOptArray("attachments");
    for (size_t i = 0; i < arr.size(); ++i) {
        auto ncont = context.CreateContext(arr[i]);
        auto entryTrace = ncont.TraceIndex("attachments", i);
        if (auto err = ImportAttachment(ncont, attachee); h.Handle(err)) {
            return err;
        }
    }
    return h;
}

SCENE_IMP_END_NAMESPACE()
