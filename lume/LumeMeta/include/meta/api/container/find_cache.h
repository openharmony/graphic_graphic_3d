/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Container query cache object
 * Author: Lauri Jaaskela
 * Create: 2023-08-02
 */

#ifndef META_API_CONTAINER_FIND_CACHE_H
#define META_API_CONTAINER_FIND_CACHE_H

#include <meta/api/event_handler.h>
#include <meta/api/threading/mutex.h>
#include <meta/interface/intf_container.h>

META_BEGIN_NAMESPACE()

/**
 * @brief FindCache base class
 */
class FindCacheBase {
public:
    FindCacheBase() = default;
    virtual ~FindCacheBase() = default;
    /**
     * @brief Returns true if a valid target has been set.
     */
    bool HasTarget() const noexcept
    {
        return !container_.expired();
    }
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
            changedHandler_.Subscribe(container->OnContainerChanged(), cb);
        }
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

protected:
    virtual void ClearResults() = 0;
    void ResetTarget()
    {
        ClearResults();
        container_.reset();
        options_ = {};
    }
    enum class CachedResultTypeBitsValue : uint16_t {
        FIND_ANY_CACHED = 1 << 0,
        FIND_ALL_CACHED = FIND_ANY_CACHED | 1 << 1,
    };

    mutable CORE_NS::Mutex mutex_;
    mutable EnumBitField<CachedResultTypeBitsValue> state_;
    META_NS::IContainer::ConstWeakPtr container_;
    META_NS::IContainer::FindOptions options_;
    EventHandler changedHandler_;
};

/**
 * @brief The FindCache class is a helper class for caching the results of a FindAny/FindAll operation on an IContainer.
 */
template<class Type>
class FindCache : public FindCacheBase {
public:
    FindCache() = default;
    ~FindCache() override = default;

    /**
     * @brief Calls IContainer::FindAny, caches and returns the result. Any subsequent FindAny calls return
     *        the cached result unless changes have been made to the container.
     */
    typename Type::Ptr FindAny() const
    {
        CORE_NS::UniqueLock lock(mutex_);
        if (const auto container = container_.lock()) {
            if (!state_.IsSet(CachedResultTypeBitsValue::FIND_ANY_CACHED)) {
                result_ = container->template FindAny<Type>(options_);
                state_.Set(CachedResultTypeBitsValue::FIND_ANY_CACHED);
            }
        }
        return result_.empty() ? nullptr : result_.front();
    }
    /**
     * @brief Calls IContainer::FindAll, caches and returns the result. Any subsequent FindAll calls return
     *        the cached result unless changes have been made to the container.
     */
    BASE_NS::vector<typename Type::Ptr> FindAll() const
    {
        CORE_NS::UniqueLock lock(mutex_);
        if (const auto container = container_.lock()) {
            if (!state_.IsSet(CachedResultTypeBitsValue::FIND_ALL_CACHED)) {
                result_ = PtrArrayCast<Type>(container->FindAll(options_));
                state_.Set(CachedResultTypeBitsValue::FIND_ALL_CACHED);
            }
        }
        return result_;
    }

private:
    void ClearResults() override
    {
        result_.clear();
        state_.Clear();
    }
    mutable BASE_NS::vector<typename Type::Ptr> result_;
};

META_END_NAMESPACE()

#endif // META_API_CONTAINER_FIND_CACHE_H
