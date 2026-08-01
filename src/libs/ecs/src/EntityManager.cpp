#include "EntityManager.hpp"

#include <antwika/log/Level.hpp>

#include "antwika/ecs/EcsError.hpp"

using antwika::ecs::EcsError;
using antwika::log::Level;

namespace antwika::ecs::detail
{

    EntityManager::EntityManager(
        ILogger &logger,
        std::uint64_t maxEntities,
        std::uint64_t maxGeneration)
        : logger(logger),
          maxEntities(maxEntities),
          maxGeneration(maxGeneration)
    {
    }

    Entity EntityManager::create()
    {
        // A freed index is preferred over a fresh one.
        // Otherwise nothing would ever bound what the vectors grow to.
        if (!freeIndices.empty())
        {
            const auto index = freeIndices.back();
            freeIndices.pop_back();
            aliveFlags[index] = true;
            return makeEntity(index, generations[index]);
        }

        if (nextIndex > maxEntities)
        {
            logger.log(
                Level::Fatal,
                "EntityManager: entity index space exhausted");
            throw EcsError(
                "EntityManager: entity index space exhausted");
        }

        const auto index = nextIndex;
        ++nextIndex;
        aliveFlags.push_back(true);
        generations.push_back(0);
        return makeEntity(index, 0);
    }

    void EntityManager::destroy(Entity entity)
    {
        if (!alive(entity))
        {
            throw EcsError("EntityManager: entity is not alive");
        }

        const auto index = entityIndex(entity);
        aliveFlags[index] = false;

        // A wrapped generation would name a live successor.
        // Which is the aliasing the generation exists to prevent.
        // So a slot out of generations is dropped rather than freed.
        // It costs one slot's worth of memory and keeps the promise.
        if (generations[index] >= maxGeneration)
        {
            return;
        }

        ++generations[index];
        freeIndices.push_back(index);
    }

    bool EntityManager::alive(Entity entity) const noexcept
    {
        const auto index = entityIndex(entity);
        return index < aliveFlags.size() && aliveFlags[index]
            && generations[index] == entityGeneration(entity);
    }

} // namespace antwika::ecs::detail
