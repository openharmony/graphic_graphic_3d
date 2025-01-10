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

#include "spirv_opt_strip_extensions.h"

#include "source/opt/ir_context.h"
#include "source/util/string_utils.h"

namespace spvtools {

namespace opt {

Pass::Status StripPreprocessorDebugInfoPass::Process()
{
    std::vector<Instruction*> to_kill;

    for (auto& dbg : context()->debugs1()) {
        if (dbg.opcode() == spv::Op::OpSourceExtension) {
            const std::string ext_name = dbg.GetInOperand(0).AsString();
            if (ext_name == "GL_GOOGLE_cpp_style_line_directive" || ext_name == "GL_GOOGLE_include_directive") {
                to_kill.push_back(&dbg);
            }
        }
    }

    bool modified = !to_kill.empty();

    for (auto* inst : to_kill) {
        context()->KillInst(inst);
    }

    return modified ? Status::SuccessWithChange : Status::SuccessWithoutChange;
}

} // namespace opt
} // namespace spvtools
