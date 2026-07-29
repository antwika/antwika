#pragma once

#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include <antwika/log/ILogger.hpp>

#include "antwika/ecs/Component.hpp"
#include "antwika/ecs/ComponentStorage.hpp"
#include "antwika/ecs/EcsError.hpp"
#include "antwika/ecs/Entity.hpp"
#include "antwika/ecs/View.hpp"

namespace antwika::ecs::detail
{
    class EntityManager;
} // namespace antwika::ecs::detail

namespace antwika::ecs
{

    using antwika::log::ILogger;

    /**
     * @brief Owns every entity and component in a simulation.
     *
     * Reads (get, has, view) always see the state as of the last
     * commit(). Writes (add, remove, destroy, set) never take effect
     * immediately — they're staged and only applied inside commit(),
     * which is also when each component type's double buffer swaps.
     * This is what lets SystemScheduler use commit() as the boundary
     * between phases: everything a phase's systems do is invisible to
     * each other, and visible together to the next phase.
     *
     * create() is the one exception — it hands back a usable Entity
     * immediately, so a system can create an entity and add components
     * to it inside the same update() call.
     */
    class World final
    {
    public:
        /**
         * @brief Construct an empty world.
         * @param logger Forwarded to the internal EntityManager; used to
         * log a fatal message if the entity index space is exhausted.
         * @param maxEntities Highest entity value ever handed out.
         * Defaults to the full range of the underlying type.
         */
        explicit World(
            ILogger &logger,
            std::uint64_t maxEntities =
                std::numeric_limits<std::uint64_t>::max());

        ~World();

        World(const World &) = delete;
        World(World &&) = delete;

        World &operator=(const World &) = delete;
        World &operator=(World &&) = delete;

        /**
         * @brief Allocate a new entity, usable immediately.
         * @return A newly-allocated Entity.
         */
        [[nodiscard]] Entity create();

        /**
         * @brief Stage an entity for destruction.
         * @param entity The entity to destroy.
         * @throws EcsError if entity is not currently alive.
         *
         * Takes effect at the next commit(): every component it has is
         * removed from its storage and its index is permanently retired.
         */
        void destroy(Entity entity);

        /**
         * @brief Check whether an entity is alive.
         * @param entity The entity to check.
         * @return True if entity was created and not since destroyed.
         */
        [[nodiscard]] bool alive(Entity entity) const noexcept;

        /**
         * @brief Stage a component value for an entity.
         * @param entity The entity to attach the component to.
         * @param value The value to store, visible after commit().
         * @throws EcsError if entity is not currently alive.
         */
        template <Component T>
        void add(Entity entity, T value)
        {
            if (!alive(entity))
            {
                throw EcsError("World: entity is not alive");
            }

            pendingOps.push_back(
                [this, entity, value]
                {
                    // A destroy() staged earlier may have run first.
                    // Inserting now would orphan a component forever.
                    // retire() only purges pools existing when it ran.
                    if (!alive(entity))
                    {
                        return;
                    }
                    storageFor<T>().insert(entity, value);
                });
        }

        /**
         * @brief Stage a component's removal from an entity.
         * @param entity The entity to remove the component from.
         * @throws EcsError if entity is not currently alive.
         */
        template <Component T>
        void remove(Entity entity)
        {
            if (!alive(entity))
            {
                throw EcsError("World: entity is not alive");
            }

            pendingOps.push_back(
                [this, entity]
                {
                    auto &storage = storageFor<T>();
                    if (storage.contains(entity))
                    {
                        storage.remove(entity);
                    }
                });
        }

        /**
         * @brief Check whether an entity currently has a component.
         * @param entity The entity to check.
         * @return True if entity is alive and has a value for T as of
         * the last commit().
         */
        template <Component T>
        [[nodiscard]] bool has(Entity entity) const noexcept
        {
            if (!alive(entity))
            {
                return false;
            }

            const auto *storage = findStorage<T>();
            return storage != nullptr && storage->contains(entity);
        }

        /**
         * @brief Read an entity's current component value.
         * @param entity The entity to read.
         * @return The value as of the last commit().
         * @throws EcsError if entity is dead or has no such component.
         */
        template <Component T>
        [[nodiscard]] const T &get(Entity entity) const
        {
            if (!alive(entity))
            {
                throw EcsError("World: entity is not alive");
            }

            const auto *storage = findStorage<T>();
            if (storage == nullptr || !storage->contains(entity))
            {
                throw EcsError("World: entity has no such component");
            }

            return storage->read(entity);
        }

        /**
         * @brief Stage a new value for an entity's existing component.
         * @param entity The entity to write.
         * @param value The value to stage, visible after commit().
         * @throws EcsError if entity is dead or has no such component
         * as of the last commit().
         */
        template <Component T>
        void set(Entity entity, T value)
        {
            if (!alive(entity))
            {
                throw EcsError("World: entity is not alive");
            }

            auto &storage = storageFor<T>();
            if (!storage.contains(entity))
            {
                throw EcsError("World: entity has no such component");
            }

            storage.write(entity, value);
        }

        /**
         * @brief Query every entity that currently has every Ts....
         * @return A read-only snapshot view, taken as of the last
         * commit().
         */
        template <Component... Ts>
        [[nodiscard]] View<Ts...> view() const
        {
            return View<Ts...>(findStorage<Ts>()...);
        }

        /**
         * @brief Apply every staged change, then swap every component
         * storage's front and back buffers.
         *
         * Staged operations run in the exact order they were called, so
         * the result never depends on how they're stored internally.
         */
        void commit();

    private:
        template <Component T>
        [[nodiscard]] ComponentStorage<T> &storageFor()
        {
            const auto key = std::type_index(typeid(T));
            const auto found = pools.find(key);
            if (found != pools.end())
            {
                return *static_cast<ComponentStorage<T> *>(
                    found->second.get());
            }

            auto storage = std::make_shared<ComponentStorage<T>>();
            auto *rawPtr = storage.get();
            pools.emplace(key, std::move(storage));

            commitCallbacks.push_back([rawPtr] { rawPtr->commit(); });
            removeFromAllPools.push_back(
                [rawPtr](Entity entity)
                {
                    if (rawPtr->contains(entity))
                    {
                        rawPtr->remove(entity);
                    }
                });

            return *rawPtr;
        }

        template <Component T>
        [[nodiscard]] const ComponentStorage<T> *findStorage() const noexcept
        {
            const auto key = std::type_index(typeid(T));
            const auto found = pools.find(key);
            if (found == pools.end())
            {
                return nullptr;
            }

            return static_cast<const ComponentStorage<T> *>(
                found->second.get());
        }

        void retire(Entity entity);

        std::unique_ptr<detail::EntityManager> entityManager;
        std::unordered_map<std::type_index, std::shared_ptr<void>> pools;
        std::vector<std::function<void()>> commitCallbacks;
        std::vector<std::function<void(Entity)>> removeFromAllPools;
        std::vector<std::function<void()>> pendingOps;
    };

} // namespace antwika::ecs
