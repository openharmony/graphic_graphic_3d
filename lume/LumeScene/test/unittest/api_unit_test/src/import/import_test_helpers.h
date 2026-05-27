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

#ifndef IMPORT_TEST_HELPERS_H
#define IMPORT_TEST_HELPERS_H

#include <scene_importer/interface/intf_diagnostics.h>

#include <gtest/gtest.h>

#include <base/containers/string.h>
#include <base/containers/string_view.h>

SCENE_BEGIN_NAMESPACE()
namespace UTest {

inline BASE_NS::string PrintErrors(const SCENE_IMP_NS::IDiagnostics::Ptr& d)
{
    BASE_NS::string str;
    if (!d) {
        return str;
    }
    for (auto&& e : d->GetErrors()) {
        str += "Error: " + e.file + ": " + e.message + "\n";
        if (!e.trace.empty()) {
            str += "\tTrace:\n";
            for (auto it = e.trace.rbegin(); it != e.trace.rend(); ++it) {
                str += "\t" + it->file + ": " + it->message + "\n";
            }
        }
    }
    return str;
}

inline size_t CountErrorsContaining(const SCENE_IMP_NS::IDiagnostics::Ptr& d, BASE_NS::string_view needle)
{
    if (!d) {
        return 0;
    }
    size_t count = 0;
    for (auto&& e : d->GetErrors()) {
        if (e.message.find(needle) != BASE_NS::string::npos) {
            ++count;
        }
    }
    return count;
}

inline bool HasErrorContaining(const SCENE_IMP_NS::IDiagnostics::Ptr& d, BASE_NS::string_view needle)
{
    return CountErrorsContaining(d, needle) > 0;
}

}  // namespace UTest
SCENE_END_NAMESPACE()

// Asserts that the given IDiagnostics::Ptr is non-null, has at least one error, and at least one
// error message contains `needle`. On failure, prints all collected diagnostics for debugging.
#define EXPECT_DIAGNOSTIC_CONTAINS(diag, needle)                                                      \
    do {                                                                                              \
        const auto& _d = (diag);                                                                      \
        ASSERT_TRUE(_d) << "expected diagnostics, got null";                                          \
        EXPECT_FALSE(_d->GetErrors().empty()) << "expected at least one error, diagnostics is empty"; \
        EXPECT_TRUE(::Scene::UTest::HasErrorContaining(_d, needle))                                   \
            << "expected an error containing: " << (needle) << "\nActual diagnostics:\n"              \
            << ::Scene::UTest::PrintErrors(_d).c_str();                                               \
    } while (0)

#endif
