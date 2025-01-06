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

#ifndef TEST_ENVIRONMENT_META_OBJECT_INITIALIZE_H
#define TEST_ENVIRONMENT_META_OBJECT_INITIALIZE_H

#include <core/log.h>

namespace environmentSetup {

void InitializeMetaObject(const char* const dllPath);
void InitializeMetaObject(const char* const dllPath, CORE_NS::ILogger::LogLevel logLevel);
void ShutdownMetaObject();

} // namespace environmentSetup

#endif
