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

#ifndef API_RENDER_RESOURCE_HANDLE_H
#define API_RENDER_RESOURCE_HANDLE_H

#include <cstdint>

#include <base/containers/generic_iterator.h>
#include <base/containers/refcnt_ptr.h>
#include <core/property/property.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
/** \addtogroup group_resourcehandle
 *  @{
 */
constexpr const uint64_t INVALID_RESOURCE_HANDLE { 0xFFFFFFFFffffffff };

/** Render handle (used for various renderer and rendering related handles) */
struct RenderHandle {
    /** ID */
    uint64_t id { INVALID_RESOURCE_HANDLE };
};
/** @} */

/** Render handle equals comparison */
inline bool operator==(const RenderHandle& lhs, const RenderHandle& rhs) noexcept
{
    return (lhs.id == rhs.id);
}

/** Render handle not equals comparison */
inline bool operator!=(const RenderHandle& lhs, const RenderHandle& rhs) noexcept
{
    return (lhs.id != rhs.id);
}

/** Render handle hash */
inline uint64_t hash(const RenderHandle& handle)
{
    return handle.id;
}

/** Render handle types */
enum class RenderHandleType : uint8_t {
    /** Undefined. */
    UNDEFINED = 0,
    /** Generic data. Often just handle to specific data. */
    GENERIC_DATA = 1,
    /** GPU buffer. */
    GPU_BUFFER = 2,
    /** GPU image. */
    GPU_IMAGE = 3,
    /** GPU buffer. */
    GPU_SAMPLER = 4,
    /** Graphics pipe shader. */
    SHADER_STATE_OBJECT = 6,
    /** Compute shader. */
    COMPUTE_SHADER_STATE_OBJECT = 7,
    /** Pipeline layout. */
    PIPELINE_LAYOUT = 8,
    /** Vertex input declaration. */
    VERTEX_INPUT_DECLARATION = 9,
    /** Graphics state. */
    GRAPHICS_STATE = 10,
    /** Render node graph. */
    RENDER_NODE_GRAPH = 11,
    /** Graphics pso. */
    GRAPHICS_PSO = 12,
    /** Compute pso. */
    COMPUTE_PSO = 13,
    /** Descriptor set. */
    DESCRIPTOR_SET = 14,
};

/** Render handle util */
namespace RenderHandleUtil {
/** Render handle type mask */
constexpr const uint64_t RENDER_HANDLE_TYPE_MASK { 0xf };

/** Checks validity of a handle */
inline constexpr bool IsValid(const RenderHandle handle)
{
    return (handle.id != INVALID_RESOURCE_HANDLE);
}

/** Returns handle type */
inline constexpr RenderHandleType GetHandleType(const RenderHandle handle)
{
    return (RenderHandleType)(handle.id & RENDER_HANDLE_TYPE_MASK);
}
}; // namespace RenderHandleUtil

class RenderHandleReference;

/** Ref counted generic render resource counter. */
class IRenderReferenceCounter {
public:
    using Ptr = BASE_NS::refcnt_ptr<IRenderReferenceCounter>;

protected:
    virtual void Ref() = 0;
    virtual void Unref() = 0;
    virtual int32_t GetRefCount() const = 0;
    friend Ptr;
    friend RenderHandleReference;

    IRenderReferenceCounter() = default;
    virtual ~IRenderReferenceCounter() = default;
    IRenderReferenceCounter(const IRenderReferenceCounter&) = delete;
    IRenderReferenceCounter& operator=(const IRenderReferenceCounter&) = delete;
    IRenderReferenceCounter(IRenderReferenceCounter&&) = delete;
    IRenderReferenceCounter& operator=(IRenderReferenceCounter&&) = delete;
};

/** Reference to a render handle. */
class RenderHandleReference final {
public:
    /** Destructor releases the reference from the owner.
     */
    ~RenderHandleReference() = default;

    /** Construct an empty reference. */
    RenderHandleReference() = default;

    /** Copy a reference. Reference count will be increased. */
    inline RenderHandleReference(const RenderHandleReference& other) noexcept;

    /** Copy a reference. Previous reference will be released and the new one aquired. */
    inline RenderHandleReference& operator=(const RenderHandleReference& other) noexcept;

    /** Construct a reference for tracking the given handle.
     * @param handle referenced handle
     * @param counter owner of the referenced handle.
     */
    inline RenderHandleReference(const RenderHandle handle, const IRenderReferenceCounter::Ptr& counter) noexcept;

    /** Move a reference. Moved from reference will be empty and reusable. */
    inline RenderHandleReference(RenderHandleReference&& other) noexcept;

    /** Move a reference. Previous reference will be released and the moved from reference will be empty and
     * reusable. */
    inline RenderHandleReference& operator=(RenderHandleReference&& other) noexcept;

    /** Check validity of the reference.
     * @return true if the reference is valid.
     */
    inline explicit operator bool() const noexcept;

    /** Get raw handle.
     * @return RenderHandle.
     */
    inline RenderHandle GetHandle() const noexcept;

    /** Check validity of the reference.
     * @return RenderHandleType.
     */
    inline RenderHandleType GetHandleType() const noexcept;

    /** Get ref count. Return 0, if invalid.
     * @return Get reference count of render handle reference.
     */
    inline int32_t GetRefCount() const noexcept;

private:
    RenderHandle handle_ {};
    IRenderReferenceCounter::Ptr counter_;
};

RenderHandleReference::RenderHandleReference(
    const RenderHandle handle, const IRenderReferenceCounter::Ptr& counter) noexcept
    : handle_(handle), counter_(counter)
{}

RenderHandleReference::RenderHandleReference(const RenderHandleReference& other) noexcept
    : handle_(other.handle_), counter_(other.counter_)
{}

RenderHandleReference& RenderHandleReference::operator=(const RenderHandleReference& other) noexcept
{
    if (&other != this) {
        handle_ = other.handle_;
        counter_ = other.counter_;
    }
    return *this;
}

RenderHandleReference::RenderHandleReference(RenderHandleReference&& other) noexcept
    : handle_(BASE_NS::exchange(other.handle_, {})), counter_(BASE_NS::exchange(other.counter_, nullptr))
{}

RenderHandleReference& RenderHandleReference::operator=(RenderHandleReference&& other) noexcept
{
    if (&other != this) {
        handle_ = BASE_NS::exchange(other.handle_, {});
        counter_ = BASE_NS::exchange(other.counter_, nullptr);
    }
    return *this;
}

RenderHandleReference::operator bool() const noexcept
{
    return (handle_.id != INVALID_RESOURCE_HANDLE) && (counter_);
}

RenderHandle RenderHandleReference::GetHandle() const noexcept
{
    return handle_;
}

RenderHandleType RenderHandleReference::GetHandleType() const noexcept
{
    return RenderHandleUtil::GetHandleType(handle_);
}

int32_t RenderHandleReference::GetRefCount() const noexcept
{
    if (counter_) {
        return counter_->GetRefCount();
    } else {
        return 0u;
    }
}
RENDER_END_NAMESPACE()

CORE_BEGIN_NAMESPACE()
namespace PropertyType {
inline constexpr PropertyTypeDecl RENDER_HANDLE_T = PROPERTYTYPE(RENDER_NS::RenderHandle);
inline constexpr PropertyTypeDecl RENDER_HANDLE_ARRAY_T = PROPERTYTYPE_ARRAY(RENDER_NS::RenderHandle);
inline constexpr PropertyTypeDecl RENDER_HANDLE_REFERENCE_T = PROPERTYTYPE(RENDER_NS::RenderHandleReference);
inline constexpr PropertyTypeDecl RENDER_HANDLE_REFERENCE_ARRAY_T =
    PROPERTYTYPE_ARRAY(RENDER_NS::RenderHandleReference);
} // namespace PropertyType
#ifdef DECLARE_PROPERTY_TYPE
DECLARE_PROPERTY_TYPE(RENDER_NS::RenderHandle);
DECLARE_PROPERTY_TYPE(RENDER_NS::RenderHandleReference);
#endif
CORE_END_NAMESPACE()
#endif // API_RENDER_RESOURCE_HANDLE_H
