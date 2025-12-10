/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef FONT_BUFFER_H
#define FONT_BUFFER_H

#include <memory>
#include <mutex>
#include <shared_mutex>

#include <base/containers/array_view.h>
#include <base/containers/refcnt_ptr.h>
#include <base/containers/vector.h>
#include <font/namespace.h>

FONT_BEGIN_NAMESPACE()
class FaceData;
class FontManager;

class FontBuffer final : public std::enable_shared_from_this<FontBuffer> {
public:
    FontBuffer(FontManager* fontMgr, uint64_t hash, BASE_NS::array_view<const uint8_t> bytes);
    ~FontBuffer();

    std::shared_ptr<FaceData> CreateFace(long index);

    void Gc();

private:
    friend class FaceData;
    friend class Font;

    uint64_t hash_;                  // path hash as map key in font mgr
    BASE_NS::vector<uint8_t> bytes_; // font file contents

    std::shared_mutex mutex_;
    // faces created from this font file. each Font instance references a FaceData instance and multiple Font instances
    // can reference the same FaceData. here we hold weak references so that FaceData is released when the last Font
    // using it is destroyed.
    BASE_NS::vector<std::weak_ptr<FaceData>> faces_;

    BASE_NS::refcnt_ptr<FontManager> fontManager;
};
FONT_END_NAMESPACE()

#endif // FONT_BUFFER_H
