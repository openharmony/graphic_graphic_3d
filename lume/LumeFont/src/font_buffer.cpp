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

#include "font_buffer.h"

#include <algorithm>

#include "face_data.h"
#include "font_manager.h"

FONT_BEGIN_NAMESPACE()
FontBuffer::FontBuffer(FontManager* fontMgr, const uint64_t hash, BASE_NS::array_view<const uint8_t> bytes)
    : hash_(hash), bytes_(bytes.data(), bytes.data() + bytes.size()),
      fontManager(BASE_NS::refcnt_ptr<FontManager>(fontMgr))
{}

FontBuffer::~FontBuffer()
{
    fontManager->Gc(hash_, this);
}

std::shared_ptr<FaceData> FontBuffer::CreateFace(long index)
{
    {
        std::shared_lock readerLock(mutex_);

        for (const auto& faceData : faces_) {
            if (auto fromWeak = faceData.lock(); fromWeak && fromWeak->face_->face_index == index) {
                return fromWeak;
            }
        }
    }
    std::lock_guard writerLock(mutex_);

    for (const auto& faceData : faces_) {
        if (auto fromWeak = faceData.lock(); fromWeak && fromWeak->face_->face_index == index) {
            return fromWeak;
        }
    }

    auto ftFace = fontManager->OpenFtFace(bytes_, index);
    if (!ftFace) {
        return nullptr;
    }

    auto face = std::make_shared<FaceData>(shared_from_this(), ftFace);

    faces_.push_back(face);
    CORE_LOG_N("create FaceData %p", this);
    return face;
}

void FontBuffer::Gc()
{
    std::lock_guard writerLock(mutex_);
    faces_.erase(
        std::remove_if(faces_.begin(), faces_.end(), [](const std::weak_ptr<FaceData>& ptr) { return ptr.expired(); }),
        faces_.cend());
}
FONT_END_NAMESPACE()
