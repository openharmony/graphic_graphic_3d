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

// Force-included (-include) into every fuzz translation unit.
//
// The engine logger/assert macros route through CORE_NS::Log / CORE_NS::LogAssert ->
// GetInstance<ILogger> -> Core::GetPluginRegister(), none of which are linked into the
// standalone fuzz build. CORE_LOG_DISABLED no-ops the CORE_LOG_* family; we then no-op the
// CORE_ASSERT* macros too so a stray defensive assert inside the loader cannot abort a
// long fuzz run for a non-bug condition. BASE_ASSERT is intentionally left alone -- it is a
// plain assert() and is exactly the kind of OOB/UAF signal we want to surface.
//
// Same prelude is used by the OHOS GN build (via -include cflag) and the standalone
// CMake build (via -include compile option). Safe to include in any translation unit.

#ifndef LUMEJPG_FUZZ_PRELUDE_H
#define LUMEJPG_FUZZ_PRELUDE_H

#ifndef CORE_LOG_DISABLED
#define CORE_LOG_DISABLED 1
#endif

#include <core/log.h>

#undef CORE_ASSERT
#undef CORE_ASSERT_MSG
#define CORE_ASSERT(...) ((void)0)
#define CORE_ASSERT_MSG(...) ((void)0)

#endif  // LUMEJPG_FUZZ_PRELUDE_H
