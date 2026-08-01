#pragma once

#include <cstdint>
#include <vector>

#include <antwika/log/ILogger.hpp>

#include "antwika/ecs/Entity.hpp"

namespace antwika::ecs::detail
{

    using antwika::log::ILogger;

    /**
     * @brief Allocates and retires Entity values.
     *
     * A retired index goes on a free list and is handed out again, so
     * what this and every ComponentStorage hold is bounded by how many
     * entities are alive at once rather than by how many have ever been
     * created. That is what keeps a session which spawns and destroys
     * steadily for hours from growing without bound; it used to hand
     * out indices from a monotonic counter and never reuse one, so
     * every sparse array grew to the highest index ever seen.
     *
     * **A bare free list would be unsafe, and the generation is what
     * makes reuse safe.** Retiring a slot bumps its generation, and a
     * handle kept from before carries the old one, so alive() reads it
     * as dead rather than as whoever holds the slot now. A stale handle
     * can only ever be dead, never somebody else — which is what a
     * caller caching a handle and treating alive() as the authority
     * relies on.
     *
     * A slot whose generation has reached maxGeneration is retired for
     * good rather than handed out with a wrapped one, since a wrapped
     * generation is exactly the aliasing the generation exists to
     * prevent. Exhausting the index space logs a fatal error and
     * throws — see create().
     */
    class EntityManager final
    {
    public:
        /**
         * @brief Construct the manager.
         * @param logger Used to log a fatal message if create() would
         * exceed maxEntities. Must outlive this object.
         * @param maxEntities Highest entity index this manager will
         * ever hand out. Defaults to every index an Entity can carry;
         * overridable so a test can force exhaustion without 2^32
         * calls.
         * @param maxGeneration Highest generation a slot may be handed
         * out with before it is retired for good. Defaults to every
         * generation an Entity can carry; overridable for the reason
         * maxEntities is, since saturating one otherwise takes 2^32
         * retirements of a single slot.
         */
        explicit EntityManager(
            ILogger &logger,
            std::uint64_t maxEntities = kMaxEntityIndex,
            std::uint64_t maxGeneration = kMaxEntityGeneration);

        EntityManager(const EntityManager &) = delete;
        EntityManager(EntityManager &&) = delete;

        EntityManager &operator=(const EntityManager &) = delete;
        EntityManager &operator=(EntityManager &&) = delete;

        /**
         * @brief Allocate the next entity value.
         * @return A live Entity: a free index carrying a generation it
         * has never been handed out with before, or a fresh one if no
         * index is free.
         * @throws EcsError if no index is free and the index space
         * configured by maxEntities is exhausted.
         *
         * Logs Level::Fatal before throwing, since exhaustion is a
         * fatal condition in practice rather than something a caller is
         * expected to retry. Throwing rather than calling std::exit
         * lets the stack unwind, so every scoped resource on it is
         * released the way it would be on any other error path.
         *
         * That unwinding only happens if something actually catches:
         * an uncaught exception may call std::terminate with the stack
         * still intact, which libstdc++ does. Every app's main()
         * therefore catches std::exception around bootstrap(), which
         * is also what lets a failed --record run still write the
         * events it recorded before the failure.
         */
        [[nodiscard]] Entity create();

        /**
         * @brief Retire an entity, freeing its index for reuse.
         * @param entity The entity to destroy.
         * @throws EcsError if entity is not currently alive.
         *
         * The index goes back on the free list with its generation
         * bumped, so every handle to the entity just retired now reads
         * as dead. A slot that has run out of generations is dropped
         * instead of freed, and never handed out again.
         */
        void destroy(Entity entity);

        /**
         * @brief Check whether an entity is alive.
         * @param entity The entity to check.
         * @return True if entity was created and not since destroyed.
         * A handle whose index has since been handed out again reads
         * as false, since it carries the generation it was created
         * with and the slot has moved on.
         */
        [[nodiscard]] bool alive(Entity entity) const noexcept;

    private:
        ILogger &logger;
        std::uint64_t maxEntities;
        std::uint64_t maxGeneration;
        std::uint64_t nextIndex{1};

        // Every vector's slot 0 stands for kNullEntity.
        // Which is never alive and never handed out.
        std::vector<bool> aliveFlags{false};
        std::vector<std::uint64_t> generations{0};

        // Taken from the back, so reuse is a pop rather than a scan.
        // Which order it hands them back in matters to no caller.
        // Only that the order is the same on every run.
        std::vector<std::uint64_t> freeIndices;
    };

} // namespace antwika::ecs::detail
