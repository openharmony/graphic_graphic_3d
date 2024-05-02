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
#include <meta/base/namespace.h>
#include <meta/interface/intf_object_registry.h>

META_BEGIN_NAMESPACE()

namespace Internal {

void RegisterBuiltInAnimations(IObjectRegistry&);
void RegisterBuiltInObjects(IObjectRegistry&);
void UnRegisterBuiltInObjects(IObjectRegistry&);
void UnRegisterBuiltInProperties(IObjectRegistry&);
void UnRegisterBuiltInSerializers(IObjectRegistry&);
void UnRegisterBuiltInAnimations(IObjectRegistry&);

void RegisterEntities(IObjectRegistry& registry)
{
    Internal::RegisterBuiltInObjects(registry);
    Internal::RegisterBuiltInAnimations(registry);
}

void UnRegisterEntities(IObjectRegistry& registry)
{
    Internal::UnRegisterBuiltInAnimations(registry);
    Internal::UnRegisterBuiltInObjects(registry);
}

} // namespace Internal
META_END_NAMESPACE()
