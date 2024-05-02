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

#include "local_matrix_system.h"

#include <ComponentTools/component_query.h>
#include <PropertyTools/property_api_impl.inl>

#include <3d/ecs/components/local_matrix_component.h>
#include <3d/ecs/components/transform_component.h>
#include <base/math/matrix_util.h>
#include <core/ecs/intf_ecs.h>
#include <core/namespace.h>

#include "property/property_handle.h"

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;

void LocalMatrixSystem::SetActive(bool state)
{
    active_ = state;
}

bool LocalMatrixSystem::IsActive() const
{
    return active_;
}

LocalMatrixSystem::LocalMatrixSystem(IEcs& ecs) : active_(true), ecs_(ecs)
{
    localMatrixManager_ = GetManager<ILocalMatrixComponentManager>(ecs);
    transformManager_ = GetManager<ITransformComponentManager>(ecs);
}

string_view LocalMatrixSystem::GetName() const
{
    return CORE3D_NS::GetName(this);
}

Uid LocalMatrixSystem::GetUid() const
{
    return UID;
}

const IPropertyHandle* LocalMatrixSystem::GetProperties() const
{
    return nullptr;
}

IPropertyHandle* LocalMatrixSystem::GetProperties()
{
    return nullptr;
}

void LocalMatrixSystem::SetProperties(const IPropertyHandle&) {}

const IEcs& LocalMatrixSystem::GetECS() const
{
    return ecs_;
}

void LocalMatrixSystem::Initialize()
{
    const ComponentQuery::Operation operations[] = {
        { *localMatrixManager_, ComponentQuery::Operation::REQUIRE },
    };
    componentQuery_.SetEcsListenersEnabled(true);
    componentQuery_.SetupQuery(*transformManager_, operations);
}

bool LocalMatrixSystem::Update(bool frameRenderingQueued, uint64_t, uint64_t)
{
    if (!active_) {
        return false;
    }

    if (transformGeneration_ == transformManager_->GetGenerationCounter()) {
        return false;
    }

    transformGeneration_ = transformManager_->GetGenerationCounter();

    bool transformComponentGenerationValid = true;
    if (componentQuery_.Execute()) {
        transformComponentGenerationValid = false;
        transformComponentGenerations_.resize(transformManager_->GetComponentCount());
    }

    for (const auto& row : componentQuery_.GetResults()) {
        const auto transformComponentId = row.components[0];

        // Resolve component generation.
        uint32_t currentComponentGenerationId = transformManager_->GetComponentGeneration(transformComponentId);

        // See if we need to re-calculate the matrix.
        bool recalculateMatrix = true;
        if (transformComponentGenerationValid) {
            if (currentComponentGenerationId == transformComponentGenerations_[transformComponentId]) {
                // Generation not changed.
                recalculateMatrix = false;
            }
        }

        if (recalculateMatrix) {
            const TransformComponent& tm = transformManager_->Get(transformComponentId);

            LocalMatrixComponent localMatrixData;
            localMatrixData.matrix = Math::Trs(tm.position, tm.rotation, tm.scale);
            localMatrixManager_->Set(row.components[1], localMatrixData);

            // Store generation.
            transformComponentGenerations_[transformComponentId] = currentComponentGenerationId;
        }
    }

    return true;
}

void LocalMatrixSystem::Uninitialize()
{
    componentQuery_.SetEcsListenersEnabled(false);
}

ISystem* LocalMatrixSystemInstance(IEcs& ecs)
{
    return new LocalMatrixSystem(ecs);
}
void LocalMatrixSystemDestroy(ISystem* instance)
{
    delete static_cast<LocalMatrixSystem*>(instance);
}

CORE3D_END_NAMESPACE()
