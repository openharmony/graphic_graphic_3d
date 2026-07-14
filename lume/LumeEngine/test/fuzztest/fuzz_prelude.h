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

// Force-included into every fuzz translation unit (-include).
//
// The engine logger/assert macros route through CORE_NS::LogAssert / CORE_NS::Log ->
// GetInstance<ILogger> -> Core::GetPluginRegister(), which is not linked in the standalone fuzz
// build. CORE_LOG_DISABLED no-ops the CORE_LOG_* family; we then no-op the CORE_ASSERT* macros
// too. BASE_ASSERT is left untouched -- it is a plain assert() and is what surfaces the OOB reads
// we are fuzzing for.
#ifndef FUZZ_PRELUDE_H
#define FUZZ_PRELUDE_H

#define CORE_LOG_DISABLED 1
#include <core/log.h>

#undef CORE_ASSERT
#undef CORE_ASSERT_MSG
#define CORE_ASSERT(...) ((void)0)
#define CORE_ASSERT_MSG(...) ((void)0)

#endif  // FUZZ_PRELUDE_H
