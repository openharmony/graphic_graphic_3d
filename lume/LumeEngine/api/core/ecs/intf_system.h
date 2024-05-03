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

#ifndef API_CORE_ECS_ISYSTEM_H
#define API_CORE_ECS_ISYSTEM_H

#include <cstdint>

#include <base/containers/string_view.h>
#include <base/namespace.h>
#include <base/util/uid.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class IEcs;
class IPropertyHandle;

/** @ingroup group_ecs_isystem */
/** ISystem */
class ISystem {
public:
    /** Get class name of the system.
     */
    virtual BASE_NS::string_view GetName() const = 0;

    /** Returns the UID of the system.
     */
    virtual BASE_NS::Uid GetUid() const = 0;

    /** Get properties of the system. (Modifiable)
     */
    virtual IPropertyHandle* GetProperties() = 0;

    /** Get properties of the system. (Read only)
     */
    virtual const IPropertyHandle* GetProperties() const = 0;

    /** Set properties of the system.
     */
    virtual void SetProperties(const IPropertyHandle&) = 0;

    /** Get active status for the system.
     */
    virtual bool IsActive() const = 0;

    /** Set active status for the system.
     *  @param isActive State what is set, true for active and false for inactive.
     */
    virtual void SetActive(bool isActive) = 0;

    /** Initialize system (Application developer should not call this since its called automatically).
     */
    virtual void Initialize() = 0;

    /** Update system with current time in the ECS and delta occured between last frame (Application developer should
     * not call this since its called automatically). Delta is scaled by the ECS time scale. For actual delta the
     * system implementation should e.g. take a difference between current and previous total time.
     * @param isFrameRenderingQueued If true, the frame rendering will happen and system must update all render data.
     * @param time Elapsed total time (in us).
     * @param delta Elapsed time since last update, scaled IEcs::GetTimeScale (in us).
     * @return True if the system updated any render data, this will queue the current frame for rendering.
     */
    virtual bool Update(bool isFrameRenderingQueued, uint64_t time, uint64_t delta) = 0;

    /** Uninitialize system (Application developer should not call this since its called automatically).
     */
    virtual void Uninitialize() = 0;

    /** Get ECS from system.
     */
    virtual const IEcs& GetECS() const = 0;

protected:
    ISystem() = default;
    ISystem(const ISystem&) = delete;
    ISystem(ISystem&&) = delete;
    ISystem& operator=(const ISystem&) = delete;
    ISystem& operator=(ISystem&&) = delete;
    virtual ~ISystem() = default;
};
CORE_END_NAMESPACE()

#endif // API_CORE_ECS_ISYSTEM_H
