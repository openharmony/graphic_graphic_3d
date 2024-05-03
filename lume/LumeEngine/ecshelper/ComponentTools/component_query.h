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

#ifndef CORE_UTIL_ECS_COMPONENT_QUERY_H
#define CORE_UTIL_ECS_COMPONENT_QUERY_H

#include <cstddef>
#include <cstdint>

#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/ecs/entity.h>
#include <core/ecs/intf_component_manager.h>
#include <core/ecs/intf_ecs.h>
#include <core/namespace.h>

BASE_BEGIN_NAMESPACE()
template<class T>
class array_view;
BASE_END_NAMESPACE()

CORE_BEGIN_NAMESPACE()
/** Executes queries to component managers and outputs a result set that can be used to speed-up component data access.
 */
class ComponentQuery : private IEcs::EntityListener, private IEcs::ComponentListener {
public:
    ComponentQuery() = default;
    ~ComponentQuery() override;

    /** Operations for the component query that are being applied in to of the base component set. */
    struct Operation {
        /** Method of operation. */
        enum Method : uint8_t {
            /** Looks up a component of given type and filters out the entity from the base set if it doesn't contain
               such component. */
            REQUIRE,
            /** Looks up a component of given type, but never filters out the entity from the base set. */
            OPTIONAL
        };
        const IComponentManager& target;
        Method method { REQUIRE };
    };

    /** Sets up a component query to component managers.
     * @param baseComponentSet Components that are used as a base set for query, this should be the component manager
     * that has least amount of components.
     * @param operations Operations that are performed to base set when other component managers are merged to result.
     * @param enableEntityLookup If true, allows to look up result row by using entity as a key (FindResultRow), this
     * slightly slows down the look-up process.
     */
    void SetupQuery(const IComponentManager& baseComponentSet, BASE_NS::array_view<const Operation> operations,
        bool enableEntityLookup = false);

    /** Executes the query. Assumes that the query has been set up earlier.
     * @return True if there are possible changes in the query result since previous execute.
     */
    bool Execute();

    [[deprecated]] void Execute(const IComponentManager& baseComponentSet,
        BASE_NS::array_view<const Operation> operations, bool enableEntityLookup = false);

    /** Enable or disable listening to ECS events. Enabling listeners will automatically invalidate this query when
     * there are changes in the relevant component managers.
     * @param enableListeners True to enable listening to ecs events to automatically invalidate the query.
     */
    void SetEcsListenersEnabled(bool enableListeners);

    /** Check if the result of this query is still valid. Requires a call to RegisterEcsListeners after each execution.
     * @return True if there have been no changes in listened component managers since previous execute that would
     * affect the query result.
     */
    bool IsValid() const;

    /** One row in a result set, describes the entity and its component ids. */
    struct ResultRow {
        ResultRow() = default;
        ~ResultRow() = default;

        ResultRow(const ResultRow& rhs) = delete;
        ResultRow& operator=(const ResultRow& rhs) = delete;
        ResultRow(ResultRow&& rhs) noexcept = default;
        ResultRow& operator=(ResultRow&& rhs) noexcept = default;

        /** Checks whether component id in given index is valid.
         * The component id can be invalid if the component was specified optional and it is not available)
         * @param index Index of the component.
         * @return True if the component id is valid, false if the component id is invalid (optional components that is
         * not available).
         */
        bool IsValidComponentId(size_t index) const;

        /** Entity that contains the components. */
        Entity entity;

        /** List of component ids, in the same order that the component managers were specified in the Execute(...)
         * call. */
        BASE_NS::vector<IComponentManager::ComponentId> components;
    };

    /** Returns The result of the query, in form of rows.
     * @return Array of result rows, where each row describes an entity and its component ids.
     */
    BASE_NS::array_view<const ResultRow> GetResults() const;

    /** Look up result row for a given entity. To enable this functionality, the Execute() function needs to be called
     * with enableEntityLookup parameter set as true.
     * @param entity Entity to use in look-up
     * @return Pointer to result row for given entity, or nullptr if there is no such row.
     */
    const ResultRow* FindResultRow(Entity entity) const;

private:
    void RegisterEcsListeners();
    void UnregisterEcsListeners();

    // IEcs::EntityListener
    void OnEntityEvent(IEcs::EntityListener::EventType type, BASE_NS::array_view<const Entity> entities) override;

    // IEcs::ComponentListener
    void OnComponentEvent(IEcs::ComponentListener::EventType type, const IComponentManager& componentManager,
        BASE_NS::array_view<const Entity> entities) override;

    CORE_NS::IEcs* ecs_ { nullptr };
    BASE_NS::vector<ResultRow> result_;
    BASE_NS::vector<IComponentManager*> managers_;
    BASE_NS::vector<Operation::Method> operationMethods_;
    BASE_NS::unordered_map<Entity, size_t> mapping_;
    bool enableLookup_ { false };
    bool enableListeners_ { false };
    bool registered_ { false };
    bool valid_ { false };
};
CORE_END_NAMESPACE()

#endif // CORE_UTIL_ECS_COMPONENT_QUERY
