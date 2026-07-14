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

#ifndef SCENE_IMP_SRC_OBJECTS_BUILTIN_H
#define SCENE_IMP_SRC_OBJECTS_BUILTIN_H

#include "../import_context.h"

SCENE_IMP_BEGIN_NAMESPACE()

class IBuiltinContainer : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IBuiltinContainer, "cc3fc41e-f27b-492b-b4d9-f1305c1c4aaa")
public:
    virtual bool SetToAny(META_NS::IAny& out) = 0;
    virtual META_NS::IAny::Ptr GetAny() const = 0;
};

class ImportResourceId : public ImportBase {
public:
    ImportResult Import(ImportContext& context) override;
};

class ImportVec2 : public ImportBase {
public:
    ImportResult Import(ImportContext& context) override;
};

class ImportVec3 : public ImportBase {
public:
    ImportResult Import(ImportContext& context) override;
};

class ImportVec4 : public ImportBase {
public:
    ImportResult Import(ImportContext& context) override;
};

class ImportColor : public ImportBase {
public:
    ImportResult Import(ImportContext& context) override;
};

class ImportQuat : public ImportBase {
public:
    ImportResult Import(ImportContext& context) override;
};

class ImportUVec2 : public ImportBase {
public:
    ImportResult Import(ImportContext& context) override;
};

class ImportUVec3 : public ImportBase {
public:
    ImportResult Import(ImportContext& context) override;
};

class ImportUVec4 : public ImportBase {
public:
    ImportResult Import(ImportContext& context) override;
};

class ImportIVec2 : public ImportBase {
public:
    ImportResult Import(ImportContext& context) override;
};

class ImportIVec3 : public ImportBase {
public:
    ImportResult Import(ImportContext& context) override;
};

class ImportIVec4 : public ImportBase {
public:
    ImportResult Import(ImportContext& context) override;
};

class ImportMat4x4 : public ImportBase {
public:
    ImportResult Import(ImportContext& context) override;
};

class ImportTimeSpan : public ImportBase {
public:
    ImportResult Import(ImportContext& context) override;
};

class ImportObjectRef : public ImportBase {
public:
    ImportResult Import(ImportContext& context) override;
};

SCENE_IMP_END_NAMESPACE()

#endif