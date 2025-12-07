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

#include "spirv_opt_extensions.h"

#include "source/util/make_unique.h"
#include "spirv_opt_strip_extensions.h"

namespace spvtools {

// This is a private implementation from glslang declared in optimizer.cpp reimplementation here
// allows us to register our own modified optimisation passes.
struct Optimizer::PassToken::Impl {
    explicit Impl(std::unique_ptr<opt::Pass> p) : pass(std::move(p)) {}
    std::unique_ptr<opt::Pass> pass; // Internal implementation pass.
};

} // namespace spvtools

void RegisterStripPreprocessorDebugInfoPass(std::optional<spvtools::Optimizer>& optimizer)
{
    optimizer->RegisterPass(spvtools::MakeUnique<spvtools::Optimizer::PassToken::Impl>(
        spvtools::MakeUnique<spvtools::opt::StripPreprocessorDebugInfoPass>()));
}
