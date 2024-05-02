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
#ifndef META_SRC_PROPERTY_DEPENDENCIES_H
#define META_SRC_PROPERTY_DEPENDENCIES_H

#include "property.h"

META_BEGIN_NAMESPACE()
namespace Internal {

// changing this will disable the check in property GetValue
constexpr bool ENABLE_DEPENDENCY_CHECK = true;

class Dependencies {
public:
    bool IsActive() const;

    void Start();
    void End();

    ReturnError AddDependency(const IProperty::ConstPtr& prop);
    ReturnError GetImmediateDependencies(BASE_NS::vector<IProperty::ConstPtr>& deps) const;
    bool HasDependency(const IProperty*) const;

private:
    bool active_ {};
    uint32_t depth_ {};

    ReturnError state_ { GenericError::SUCCESS };

    struct Dependancy {
        IProperty::ConstPtr property;
        uint64_t depth {};
    };
    BASE_NS::vector<Dependancy> deps_;
    BASE_NS::vector<Dependancy> deps_branch_;
};

Dependencies& GetDeps();

} // namespace Internal
META_END_NAMESPACE()

#endif