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

#ifndef SCENE_SRC_UTIL_INTERFACES_H
#define SCENE_SRC_UTIL_INTERFACES_H

#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

class IArrayElementIndex : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IArrayElementIndex, "57d7d2e3-0367-45c2-8d45-1a4cc71cdd17")
public:
    virtual void SetIndex(size_t index) = 0;
};

class ArrayElementIndex : public META_NS::IntroduceInterfaces<IArrayElementIndex> {
public:
    void SetIndex(size_t index) override
    {
        index_ = index;
    }

protected:
    size_t index_ = -1;
};

SCENE_END_NAMESPACE()

#endif