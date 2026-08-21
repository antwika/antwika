#include "antwika/ecs/World.hpp"

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

        pendingDestroyedEntities.push_back(entity);
    }

    bool World::alive(Entity entity) const noexcept
    {
        return entityManager->alive(entity);
    }

    void World::commit()
    {
        std::vector<Entity> destroyingEntities;
        destroyingEntities.swap(pendingDestroyedEntities);

        const ScopeGuard guard(
            [this]
            {
                clearPending();

                for (std::size_t index = 0; index < filledSlots.size(); ++index)
                {
                    pools[filledSlots[index]]->commit();
                }
            });

        for (std::size_t index = 0; index < filledSlots.size(); ++index)
        {
            if (auto *pendingBuffer = pendingBuffers[filledSlots[index]].get();
                pendingBuffer != nullptr)
            {
                pendingBuffer->apply();
            }
        }

        releaseEntities(destroyingEntities);
    }

    std::size_t World::slotFor(
        const ComponentKey key, const std::string_view name)
    {
        if (const auto foundSlot = findSlot(key); foundSlot != kNoSlot)
        {
            return foundSlot;
        }

        if (filledSlots.size() == kMaxComponents)
        {
            throw EcsError("World: too many component types");
        }

        detail::claimComponentKey(key, name);

        auto slotIndex = static_cast<std::size_t>(key) & kSlotMask;

        while (keys[slotIndex] != 0)
        {
            slotIndex = (slotIndex + 1) & kSlotMask;
        }

        keys[slotIndex] = key;
        filledSlots.push_back(slotIndex);

        return slotIndex;
    }

    void World::forgetComponents() noexcept
    {
        for (const auto index : filledSlots)
        {
            pendingBuffers[index].reset();
            pools[index].reset();
            keys[index] = 0;
        }

        filledSlots.clear();
    }

    void World::clearPending() noexcept
    {
        pendingDestroyedEntities.clear();

        for (const auto index : filledSlots)
        {
            if (auto *pendingBuffer = pendingBuffers[index].get();
                pendingBuffer != nullptr)
            {
                pendingBuffer->clear();
            }
        }
    }

    void World::releaseEntities(std::span<const Entity> entities)
    {
        std::vector<Entity> doomedEntities;
        doomedEntities.reserve(entities.size());

        for (const auto entity : entities)
        {
            if (!alive(entity))
            {
                continue;
            }

            entityManager->destroy(entity);
            doomedEntities.push_back(entity);
        }

        if (doomedEntities.empty())
        {
            return;
        }

        for (const auto index : filledSlots)
        {
            pools[index]->removeAll(doomedEntities);
        }
    }

}
