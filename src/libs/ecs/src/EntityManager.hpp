#pragma once

#include <cstdint>
#include <limits>
#include <vector>

#include <antwika/log/ILogger.hpp>

#include "antwika/ecs/Entity.hpp"

namespace antwika::ecs::detail
{

    using antwika::log::ILogger;

    /**
     * @brief Allocates and retires Entity values.
     *
     * Deliberately never reuses a retired index: no generation counter,
     * no free list. The only thing a generation counter guards against
     * is a stale handle aliasing a recycled index, which can't happen
     * if indices are never recycled. Exhausting the index space logs a
     * fatal error and throws — see create().
     */
    class EntityManager final
    {
    public:
        /**
         * @brief Construct the manager.
         * @param logger Used to log a fatal message if create() would
         * exceed maxEntities. Must outlive this object.
         * @param maxEntities Highest entity value this manager will ever
         * hand out. Defaults to the full range of the underlying type;
         * overridable so a test can force exhaustion without 2^64 calls.
         */
        explicit EntityManager(
            ILogger &logger,
            std::uint64_t maxEntities =
                std::numeric_limits<std::uint64_t>::max());

        EntityManager(const EntityManager &) = delete;
        EntityManager(EntityManager &&) = delete;

        EntityManager &operator=(const EntityManager &) = delete;
        EntityManager &operator=(EntityManager &&) = delete;

        /**
         * @brief Allocate the next entity value.
         * @return A newly-allocated, never-before-used Entity.
         * @throws EcsError if the index space configured by maxEntities
         * is exhausted.
         *
         * Logs Level::Fatal before throwing, since exhaustion is a
         * fatal condition in practice rather than something a caller is
         * expected to retry. It still throws rather than calling
         * std::exit, so the stack unwinds and every scoped resource on
         * it — an in-progress --record replay file, most visibly — is
         * released the way it would be on any other error path.
         */
        [[nodiscard]] Entity create();

        /**
         * @brief Permanently retire an entity's index.
         * @param entity The entity to destroy.
         * @throws EcsError if entity is not currently alive.
         */
        void destroy(Entity entity);

        /**
         * @brief Check whether an entity is alive.
         * @param entity The entity to check.
         * @return True if entity was created and not since destroyed.
         */
        [[nodiscard]] bool alive(Entity entity) const noexcept;

    private:
        ILogger &logger;
        std::uint64_t maxEntities;
        std::uint64_t nextValue{1};
        std::vector<bool> aliveFlags{false}; // index 0 == kNullEntity
    };

} // namespace antwika::ecs::detail
