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
              std::make_unique<detail::EntityManager>(logger, maxEntities)),
          keys(kSeedSlots),
          pools(kSeedSlots),
          pendingBuffers(kSeedSlots)
    {
    }

    World::~World() = default;

    Entity World::create()
    {
        return entityManager->create();
    }

    void World::destroy(Entity entity)
    {
        if (!isAlive(entity))
        {
            throw EcsError("World: entity is not alive");
        }

        pendingDestroyedEntities.push_back(entity);
    }

    bool World::isAlive(Entity entity) const noexcept
    {
        return entityManager->isAlive(entity);
    }

    std::vector<Entity> World::getLiveEntities() const
    {
        return entityManager->getLiveEntities();
    }

    ILogger &World::getLogger() const noexcept
    {
        return entityManager->getLogger();
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
        if (const auto foundSlot = getFindSlot(key); foundSlot != kNoSlot)
        {
            return foundSlot;
        }

        detail::claimComponentKey(key, name);

        if (filledSlots.size() >= getSlotCapacity())
        {
            growSlots();
        }

        const auto slotIndex = probedSlotFor(keys, keys.size() - 1, key);

        keys[slotIndex] = key;
        filledSlots.push_back(slotIndex);

        return slotIndex;
    }

    void World::growSlots()
    {
        const auto grownCount = keys.size() * 2;
        const auto grownMask = grownCount - 1;

        std::vector<ComponentKey> grownKeys(grownCount);
        std::vector<std::unique_ptr<detail::IComponentPool>> grownPools(
            grownCount);
        std::vector<std::unique_ptr<detail::IPendingComponents>>
            grownPendingBuffers(grownCount);
        std::vector<std::size_t> grownFilledSlots;
        grownFilledSlots.reserve(filledSlots.size());

        for (const auto index : filledSlots)
        {
            const auto slotIndex =
                probedSlotFor(grownKeys, grownMask, keys[index]);

            grownKeys[slotIndex] = keys[index];
            grownPools[slotIndex] = std::move(pools[index]);
            grownPendingBuffers[slotIndex] = std::move(pendingBuffers[index]);
            grownFilledSlots.push_back(slotIndex);
        }

        keys = std::move(grownKeys);
        pools = std::move(grownPools);
        pendingBuffers = std::move(grownPendingBuffers);
        filledSlots = std::move(grownFilledSlots);
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
            if (!isAlive(entity))
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
