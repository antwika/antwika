#include "antwika/ecs/World.hpp"

#include <functional>
#include <utility>
#include <vector>

#include "EntityManager.hpp"

namespace
{

    template <typename Action>
    class ScopeGuard final
    {
    public:
        explicit ScopeGuard(Action action) : action(std::move(action))
        {
        }

        ~ScopeGuard()
        {
            action();
        }

        ScopeGuard(const ScopeGuard &) = delete;
        ScopeGuard(ScopeGuard &&) = delete;

        ScopeGuard &operator=(const ScopeGuard &) = delete;
        ScopeGuard &operator=(ScopeGuard &&) = delete;

    private:
        Action action;
    };

}

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

        pendingOps.push_back(
            [this, entity]
            {
                if (!alive(entity))
                {
                    return;
                }

                retire(entity);
            });
    }

    bool World::alive(Entity entity) const noexcept
    {
        return entityManager->alive(entity);
    }

    void World::commit()
    {
        std::vector<std::function<void()>> running;
        running.swap(pendingOps);

        const ScopeGuard guard(
            [this]
            {
                pendingOps.clear();

                for (const auto &commitCallback : commitCallbacks)
                {
                    commitCallback();
                }
            });

        for (const auto &op : running)
        {
            op();
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

}
