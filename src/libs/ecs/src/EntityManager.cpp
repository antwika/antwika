#include "EntityManager.hpp"

#include <antwika/log/Level.hpp>

#include "antwika/ecs/EcsError.hpp"

using antwika::ecs::EcsError;
using antwika::log::Level;

namespace antwika::ecs::detail
{

    EntityManager::EntityManager(ILogger &logger, std::uint64_t maxEntities)
        : logger(logger), maxEntities(maxEntities)
    {
    }

    Entity EntityManager::create()
    {
        if (nextValue > maxEntities)
        {
            logger.log(
                Level::Fatal,
                "EntityManager: entity index space exhausted");
            throw EcsError(
                "EntityManager: entity index space exhausted");
        }

        const auto value = nextValue;
        ++nextValue;
        aliveFlags.push_back(true);
        return Entity{value};
    }

    void EntityManager::destroy(Entity entity)
    {
        if (!isAlive(entity))
        {
            throw EcsError("EntityManager: entity is not alive");
        }

        aliveFlags[rawValue(entity)] = false;
    }

    bool EntityManager::isAlive(Entity entity) const noexcept
    {
        const auto value = rawValue(entity);
        return value < aliveFlags.size() && aliveFlags[value];
    }

}
