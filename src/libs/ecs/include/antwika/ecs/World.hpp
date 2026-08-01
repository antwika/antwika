#pragma once

#include <cstdint>
#include <functional>
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
     * commit(). Writes (add, remove, destroy, set) never become
     * visible immediately — the state a read returns only changes
     * inside commit(), which is also when each component type's double
     * buffer swaps. This is what lets SystemScheduler use commit() as
     * the boundary between phases: everything a phase's systems do is
     * invisible to each other, and visible together to the next phase.
     *
     * **Two different mechanisms produce that one guarantee, and the
     * difference is observable.** add(), remove() and destroy() are
     * *deferred*: they append a closure to an internal list, and
     * nothing they describe has happened until commit() runs it. set()
     * is *immediate*: it writes the value straight into the component's
     * back buffer, and commit() only reveals it by swapping the
     * buffers. Both are invisible until commit(), so both read as
     * "staged" from the outside, but only the deferred three are.
     * See set() for the one place a caller can tell them apart.
     *
     * create() is the third mechanism — it hands back a usable Entity
     * immediately and visibly, so a system can create an entity and add
     * components to it inside the same update() call.
     */
    class World final
    {
    public:
        /**
         * @brief Construct an empty world.
         * @param logger Forwarded to the internal EntityManager; used to
         * log a fatal message if the entity index space is exhausted.
         * Must outlive this object.
         * @param maxEntities Highest entity index ever handed out, and
         * so the most entities that may be alive at once. Defaults to
         * every index an Entity can carry. It is not a cap on how many
         * entities a session may create, since a destroyed entity's
         * index is handed out again.
         */
        explicit World(
            ILogger &logger, std::uint64_t maxEntities = kMaxEntityIndex);

        ~World();

        World(const World &) = delete;
        World(World &&) = delete;

        World &operator=(const World &) = delete;
        World &operator=(World &&) = delete;

        /**
         * @brief Allocate a new entity, usable immediately.
         * @return A newly-allocated Entity.
         * @throws EcsError if the entity index space is exhausted.
         */
        [[nodiscard]] Entity create();

        /**
         * @brief Stage an entity for destruction.
         * @param entity The entity to destroy.
         * @throws EcsError if entity is not currently alive.
         *
         * Takes effect at the next commit(): every component it has is
         * removed from its storage, and its index goes back on the free
         * list for a later create() to hand out again. The generation
         * that index carries is bumped on the way, so this handle — and
         * every copy of it anyone kept — reads as dead from alive()
         * rather than as whatever entity claims the index next.
         * Staging the same entity twice in one phase is allowed, and
         * retires it once — nothing is applied until commit(), so the
         * second call sees it alive just like the first.
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
         * @brief Write a new value into an entity's existing component,
         * visible after commit().
         * @param entity The entity to write.
         * @param value The value to write, visible after commit().
         * @throws EcsError if entity is dead or has no such component
         * as of the last commit().
         *
         * **This does not go through the staging list add(), remove()
         * and destroy() use.** The value lands in the component's back
         * buffer here and now; commit() only makes it visible, by
         * swapping the buffers. Two consequences follow from that, and
         * neither applies to the deferred three.
         *
         * The component has to exist *already*, as of the last
         * commit(). So add() followed by set() in the same phase throws
         * rather than writing: the add is still sitting on the staging
         * list and has put nothing in any buffer for set() to find. Add
         * the value you want, or commit() between the two.
         *
         * And the write is ordered by when set() was called rather than
         * by where it sits among the staged operations, so a set() and
         * a deferred remove() of the same component in one phase always
         * resolve as the remove — whichever came first.
         *
         * Both are the price of the immediate write, and staging set()
         * to remove them would change when every write is ordered, so
         * it is a decision of its own rather than a fix to make in
         * passing.
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
         * Should one of them throw, the exception propagates, but the
         * staging list is still emptied and every buffer still swapped:
         * a commit either happens or doesn't, never halfway.
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
