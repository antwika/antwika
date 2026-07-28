#include "EntityManager.hpp"

#include <cstdlib>

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
            std::exit(EXIT_FAILURE); // GCOVR_EXCL_LINE
            // Forked-child death-test coverage isn't reliably seen by gcovr.
        }

        const auto value = nextValue;
        ++nextValue;
        aliveFlags.push_back(true);
        return Entity{value};
    }

    void EntityManager::destroy(Entity entity)
    {
        if (!alive(entity))
        {
            throw EcsError("EntityManager: entity is not alive");
        }

        aliveFlags[rawValue(entity)] = false;
    }

    bool EntityManager::alive(Entity entity) const noexcept
    {
        const auto value = rawValue(entity);
        return value < aliveFlags.size() && aliveFlags[value];
    }

} // namespace antwika::ecs::detail
