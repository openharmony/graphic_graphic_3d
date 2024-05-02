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

#ifndef META_API_CONTAINER_FIND_CACHE_H
#define META_API_CONTAINER_FIND_CACHE_H

#include <meta/api/event_handler.h>
#include <meta/api/internal/object_api.h>
#include <meta/api/threading/mutex.h>
#include <meta/interface/intf_container.h>

META_BEGIN_NAMESPACE()

/**
 * @brief The FindCache class is a helper class for caching the results of a FindAny/FindAll operation on an IContainer.
 */
template<class Type>
class FindCache {
public:
    FindCache() = default;
    ~FindCache() = default;
    /**
     * @brief Sets the target for FindCache.
     * @param container Target container.
     * @param options Find options for IContainer::Find operation.
     */
    void SetTarget(const META_NS::IContainer::ConstPtr& container, const META_NS::IContainer::FindOptions& options)
    {
        CORE_NS::UniqueLock lock(mutex_);
        ResetTarget();
        if (container) {
            container_ = container;
            options_ = options;
            const auto cb = MakeCallback<IOnChildChanged>([this](const ChildChangedInfo&) { Invalidate(); });
            addedHandler_.Subscribe(container->OnAdded(), cb);
            removedHandler_.Subscribe(container->OnRemoved(), cb);
        }
    }
    /**
     * @brief Returns true if a valid target has been set.
     */
    bool HasTarget() const noexcept
    {
        return !container_.expired();
    }
    /**
     * @brief Calls IContainer::FindAny, caches and returns the result. Any subsequent FindAny calls return
     *        the cached result unless changes have been made to the container.
     */
    typename Type::Ptr FindAny() const
    {
        CORE_NS::UniqueLock lock(mutex_);
        if (const auto container = container_.lock()) {
            if (!cached_.IsSet(CachedResultTypeBitsValue::FIND_ANY_CACHED)) {
                resultAny_ = container->template FindAny<Type>(options_);
                cached_.Set(CachedResultTypeBitsValue::FIND_ANY_CACHED);
            }
            return resultAny_;
        }
        return {};
    }
    /**
     * @brief Calls IContainer::FindAll, caches and returns the result. Any subsequent FindAll calls return
     *        the cached result unless changes have been made to the container.
     */
    BASE_NS::vector<typename Type::Ptr> FindAll() const
    {
        CORE_NS::UniqueLock lock(mutex_);
        if (const auto container = container_.lock()) {
            if (!cached_.IsSet(CachedResultTypeBitsValue::FIND_ALL_CACHED)) {
                resultAll_ = PtrArrayCast<Type>(container->FindAll(options_));
                cached_.Set(CachedResultTypeBitsValue::FIND_ALL_CACHED);
            }
            return resultAll_;
        }
        return {};
    }
    /**
     * @brief Invalidates the cached query results.
     */
    void Invalidate()
    {
        CORE_NS::UniqueLock lock(mutex_);
        ClearResults();
    }
    /**
     * @brief Resets the cache (results and target).
     */
    void Reset()
    {
        CORE_NS::UniqueLock lock(mutex_);
        ResetTarget();
    }

private:
    void ResetTarget()
    {
        ClearResults();
        container_.reset();
        options_ = {};
    }
    void ClearResults()
    {
        resultAny_.reset();
        resultAll_.clear();
        cached_.Clear();
    }
    enum class CachedResultTypeBitsValue : uint16_t {
        FIND_ANY_CACHED = 1,
        FIND_ALL_CACHED = 2,
    };

    mutable CORE_NS::Mutex mutex_;
    mutable EnumBitField<CachedResultTypeBitsValue> cached_;
    mutable typename Type::Ptr resultAny_;
    mutable BASE_NS::vector<typename Type::Ptr> resultAll_;
    META_NS::IContainer::ConstWeakPtr container_;
    META_NS::IContainer::FindOptions options_;
    EventHandler addedHandler_;
    EventHandler removedHandler_;
};

META_END_NAMESPACE()

#endif // META_API_CONTAINER_FIND_CACHE_H
