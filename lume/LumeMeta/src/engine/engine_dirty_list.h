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
#ifndef META_SRC_ENGINE_ENGINE_DIRTY_LIST_H
#define META_SRC_ENGINE_ENGINE_DIRTY_LIST_H

#include <mutex>

#include <base/containers/vector.h>

#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()

class IEngineValue;
class IEngineValueManager;

namespace Internal {

/**
 * @brief Thread-safe dirty list for tracking which engine values have been modified.
 *        EngineValue pushes itself here on false->true transition of valueChanged_.
 *        EngineValueManager steals the list during Sync(TO_ENGINE) to sync only dirty values.
 */
struct EngineDirtyList {
    std::mutex mutex;
    BASE_NS::vector<IEngineValue*> values;
    IEngineValueManager* owner{};
};

}  // namespace Internal

META_END_NAMESPACE()

#endif
