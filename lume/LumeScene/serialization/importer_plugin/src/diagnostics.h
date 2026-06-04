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

#ifndef SCENE_IMP_SRC_DIAGNOSTICS_H
#define SCENE_IMP_SRC_DIAGNOSTICS_H

#include <scene_importer/interface/intf_diagnostics.h>

#include <meta/ext/object.h>
#include <meta/interface/interface_helpers.h>

SCENE_IMP_BEGIN_NAMESPACE()

META_REGISTER_CLASS(Diagnostics, "f0bdd63b-adfb-4ad6-9b88-2efcaaff0fae", META_NS::ObjectCategoryBits::NO_CATEGORY)

class IDiagnosticsPrivate : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IDiagnosticsPrivate, "e0f1b54a-9fa0-47e7-a036-76ad5e03e695")
public:
    virtual void SetErrors(BASE_NS::vector<Error>) = 0;
};

class Diagnostics : public META_NS::IntroduceInterfaces<META_NS::BaseObject, IDiagnostics, IDiagnosticsPrivate> {
    META_OBJECT(Diagnostics, ClassId::Diagnostics, IntroduceInterfaces)
public:
    BASE_NS::vector<Error> GetErrors() const override
    {
        return errors_;
    }
    void SetErrors(BASE_NS::vector<Error> errs) override
    {
        errors_ = BASE_NS::move(errs);
    }

private:
    BASE_NS::vector<Error> errors_;
};

inline IDiagnostics::Ptr MakeSimpleError(
    BASE_NS::string error, BASE_NS::string file, BASE_NS::vector<ErrorInfo> trace = {})
{
    auto d = META_NS::GetObjectRegistry().Create<IDiagnostics>(ClassId::Diagnostics);
    if (auto p = interface_cast<IDiagnosticsPrivate>(d)) {
        Error err;
        err.message = BASE_NS::move(error);
        err.file = BASE_NS::move(file);
        err.trace = BASE_NS::move(trace);
        p->SetErrors({BASE_NS::move(err)});
    }
    return d;
}

inline void AppendDiagnostics(IDiagnostics::Ptr& target, const IDiagnostics::Ptr& source)
{
    if (auto p = interface_cast<IDiagnosticsPrivate>(target)) {
        auto errors = target->GetErrors();
        auto sourceErrors = source->GetErrors();
        errors.insert(errors.end(), sourceErrors.begin(), sourceErrors.end());
        p->SetErrors(BASE_NS::move(errors));
    }
}

inline void MergeDiagnostics(IDiagnostics::Ptr& target, const IDiagnostics::Ptr& source)
{
    if (!source) {
        return;
    }
    if (!target) {
        target = source;
        return;
    }
    AppendDiagnostics(target, source);
}

SCENE_IMP_END_NAMESPACE()

#endif