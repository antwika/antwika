#include "antwika/ecs/World.hpp"

#include "EntityManager.hpp"

namespace antwika::ecs
{

    World::World(ILogger &logger, std::uint64_t maxEntities)
        : entityManager(
              std::make_unique<detail::EntityManager>(logger, maxEntities))
    {
    }

    World::~World() = default;

    Entity World::create()
    {
        return entityManager->create();
    }

    void World::destroy(Entity entity)
    {
        if (!alive(entity))
        {
            throw EcsError("World: entity is not alive");
        }

        pendingOps.push_back([this, entity] { retire(entity); });
    }

    bool World::alive(Entity entity) const noexcept
    {
        return entityManager->alive(entity);
    }

    void World::commit()
    {
        for (const auto &op : pendingOps)
        {
            op();
        }
        pendingOps.clear();

        for (const auto &commitCallback : commitCallbacks)
        {
            commitCallback();
        }
    }

    void World::retire(Entity entity)
    {
        for (const auto &purge : removeFromAllPools)
        {
            purge(entity);
        }

        entityManager->destroy(entity);
    }

} // namespace antwika::ecs
