#include "antwika/ecs/World.hpp"

#include <functional>
#include <utility>
#include <vector>

#include "EntityManager.hpp"

namespace
{

    // Runs its action however the enclosing scope is left.
    // commit() needs the world whole after a throwing staged operation.
    // A guard is the one form of that which cannot be jumped over.
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

} // namespace

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
                // Nothing is applied until commit().
                // So a second destroy() in one phase saw it alive too.
                // Retiring twice would throw rather than mean anything.
                // This is a no-op for the reason remove<T>() is one.
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
        // Taken off the member before any of it runs.
        // An op that staged another would grow this very vector.
        // The loop below holds a reference into it.
        // A reallocation there invalidates that reference.
        // No op does that today, so this guards rather than fixes.
        // Structural beats a rule every future op has to remember.
        std::vector<std::function<void()>> running;
        running.swap(pendingOps);

        // Leaving early would keep surviving ops staged for next time.
        // It would also skip every buffer swap.
        // The world would then be half-applied, and wrong much later.
        // The swap above already emptied pendingOps.
        // So this clears only what a running op staged onto it.
        // Which is dropped rather than carried to the next commit.
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

} // namespace antwika::ecs
