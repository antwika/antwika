#pragma once

#include <cstdint>
#include <limits>
#include <vector>

#include <antwika/log/ILogger.hpp>

#include "antwika/ecs/Entity.hpp"

namespace antwika::ecs::detail
{

    using antwika::log::ILogger;

    class EntityManager final
    {
    public:
        explicit EntityManager(
            ILogger &logger,
            std::uint64_t maxEntities =
                std::numeric_limits<std::uint64_t>::max());

        EntityManager(const EntityManager &) = delete;
        EntityManager(EntityManager &&) = delete;

        EntityManager &operator=(const EntityManager &) = delete;
        EntityManager &operator=(EntityManager &&) = delete;

        [[nodiscard]] Entity create();

        void destroy(Entity entity);

        [[nodiscard]] bool isAlive(Entity entity) const noexcept;

        [[nodiscard]] std::vector<Entity> getLiveEntities() const;

        [[nodiscard]] ILogger &getLogger() const noexcept;

    private:
        ILogger &logger;
        std::uint64_t maxEntities;
        std::uint64_t nextValue{1};
        std::vector<bool> aliveFlags{false};
    };

}
