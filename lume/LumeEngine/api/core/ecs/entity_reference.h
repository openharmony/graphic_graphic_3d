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

#ifndef API_CORE_ECS_ENTITY_REFERENCE_H
#define API_CORE_ECS_ENTITY_REFERENCE_H

#include <cstdint>

#include <base/containers/refcnt_ptr.h>
#include <base/containers/type_traits.h>
#include <base/namespace.h>
#include <core/ecs/entity.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class EntityReference;

/** Entity reference counter counter. */
class IEntityReferenceCounter {
public:
    using Ptr = BASE_NS::refcnt_ptr<IEntityReferenceCounter>;
    virtual int32_t GetRefCount() const noexcept = 0;

protected:
    virtual void Ref() noexcept = 0;
    virtual void Unref() noexcept = 0;

    friend Ptr;
    friend EntityReference;

    IEntityReferenceCounter() = default;
    virtual ~IEntityReferenceCounter() = default;
    IEntityReferenceCounter(const IEntityReferenceCounter&) = delete;
    IEntityReferenceCounter& operator=(const IEntityReferenceCounter&) = delete;
    IEntityReferenceCounter(IEntityReferenceCounter&&) = delete;
    IEntityReferenceCounter& operator=(IEntityReferenceCounter&&) = delete;
};

/**
 * Helper for updating reference counts of shared entities.
 */
class EntityReference {
public:
    /** Destructor releases the reference from the owning entity manager.
     */
    ~EntityReference() = default;

    /** Construct an empty reference. */
    EntityReference() = default;

    /** Construct a reference for tracking the given entity.
     * @param entity referenced entity
     * @param counter Reference counter instance.
     */
    EntityReference(Entity entity, const IEntityReferenceCounter::Ptr& counter) noexcept
        : entity_(entity), counter_(counter)
    {}

    /** Copy a reference. Reference count will be increased. */
    EntityReference(const EntityReference& other) noexcept : entity_(other.entity_), counter_(other.counter_) {}

    /** Move a reference. Moved from reference will be empty and reusable. */
    EntityReference(EntityReference&& other) noexcept
        : entity_(BASE_NS::exchange(other.entity_, Entity {})), counter_(BASE_NS::exchange(other.counter_, nullptr))
    {}

    /** Copy a reference. Previous reference will be released and the one aquired. */
    EntityReference& operator=(const EntityReference& other) noexcept
    {
        if (&other != this) {
            entity_ = other.entity_;
            counter_ = other.counter_;
        }
        return *this;
    }

    /** Move a reference. Previous reference will be released and and the moved from reference will be empty and
     * reusable. */
    EntityReference& operator=(EntityReference&& other) noexcept
    {
        if (&other != this) {
            entity_ = BASE_NS::exchange(other.entity_, Entity {});
            counter_ = BASE_NS::exchange(other.counter_, nullptr);
        }
        return *this;
    }

    /** Get the referenced entity. */
    operator Entity() const noexcept
    {
        return entity_;
    }

    /** Check validity of the reference.
     * @return true if the reference is valid.
     */
    explicit operator bool() const noexcept
    {
        return EntityUtil::IsValid(entity_) && (counter_);
    }

    /** Get ref count. Return 0, if invalid.
     * @return Get reference count of render handle reference.
     */
    int32_t GetRefCount() const noexcept
    {
        if (counter_) {
            return counter_->GetRefCount();
        }
        return 0;
    }

private:
    Entity entity_;
    IEntityReferenceCounter::Ptr counter_;
};
CORE_END_NAMESPACE()

#endif // API_CORE_ECS_ENTITY_REFERENCE_H
