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

#ifndef SCENE_IMP_SRC_IMPORT_ANY_H
#define SCENE_IMP_SRC_IMPORT_ANY_H

#include <core/json/json.h>

#include <meta/base/ids.h>

#include "config.h"
#include "import_helpers.h"

SCENE_IMP_BEGIN_NAMESPACE()

class ImportContext;

/// Set value of an existing IAny from a JSON value (with type coercion).
IDiagnostics::Ptr SetAnyValue(ImportContext& context, META_NS::IAny& any, const CORE_NS::json::value& value);

/// Import a JSON object into a live IObject via subtype dispatch.
ImportResult ImportObject(ImportContext& context);

/// Create a new IAny from a JSON value, allocating the matching type.
/// Null JSON values produce a null `result` without error.
IDiagnostics::Ptr GetAnyValue(ImportContext& context, const CORE_NS::json::value& value, META_NS::IAny::Ptr& result);

/// Optional "type" + "objectUid" pair carried by a JSON object describing an
/// IObject. On the regular create path the pair drives object construction.
/// At a readonly-property modify-in-place site the `objectUid` is verified
/// against the live object's class id (see MatchClassId).
struct ObjectIdent {
    BASE_NS::string type;
    OptValue<META_NS::ObjectId> objectUid;
};

/// Pull "type" + "objectUid" from `context`. Both optional. Reports parse errors
/// (e.g. malformed uid string) via the returned diagnostics.
IDiagnostics::Ptr ReadObjectIdent(ImportContext& context, ObjectIdent& out);

/// If `ident.objectUid` is set, compare it to `obj.GetClassId()` and return a
/// diagnostic on mismatch. The `type` string is accepted but not checked —
/// importer "type" strings (e.g. "extensionNode") don't map 1:1 to class ids.
IDiagnostics::Ptr MatchClassId(ImportContext& context, const META_NS::IObject& obj, const ObjectIdent& ident);

SCENE_IMP_END_NAMESPACE()

#endif
