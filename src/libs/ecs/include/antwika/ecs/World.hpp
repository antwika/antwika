#pragma once

#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include <antwika/log/ILogger.hpp>

#include "antwika/ecs/Component.hpp"
#include "antwika/ecs/ComponentKey.hpp"
#include "antwika/ecs/ComponentPool.hpp"
#include "antwika/ecs/ComponentStorage.hpp"
#include "antwika/ecs/EcsError.hpp"
#include "antwika/ecs/Entity.hpp"
#include "antwika/ecs/OpKind.hpp"
#include "antwika/ecs/PendingOps.hpp"
#include "antwika/ecs/View.hpp"

namespace antwika::ecs::detail
{
    class EntityManager;

}

namespace antwika::ecs
{

    using antwika::log::ILogger;

    class World final
    {
    public:
        explicit World(
            ILogger &logger,
            std::uint64_t maxEntities =
                std::numeric_limits<std::uint64_t>::max());

        ~World();

        World(const World &) = delete;
        World(World &&) = delete;

        World &operator=(const World &) = delete;
        World &operator=(World &&) = delete;

        [[nodiscard]] Entity create();

        void destroy(Entity entity);

        [[nodiscard]] bool isAlive(Entity entity) const noexcept;

        template <Component T>
        void add(Entity entity, T value)
        {
            if (!isAlive(entity))
            {
                throw EcsError("World: entity is not alive");
            }

            pendingFor<T>().add(entity, value);
        }

        template <Component T>
        void remove(Entity entity)
        {
            if (!isAlive(entity))
            {
                throw EcsError("World: entity is not alive");
            }

            pendingFor<T>().remove(entity);
        }

        template <Component T>
        [[nodiscard]] bool has(Entity entity) const noexcept
        {
            if (!isAlive(entity))
            {
                return false;
            }

            const auto *storage = findStorage<T>();
            return storage != nullptr && storage->contains(entity);
        }

        template <Component T>
        [[nodiscard]] const T &get(Entity entity) const
        {
            if (!isAlive(entity))
            {
                throw EcsError("World: entity is not alive");
            }

            const auto *storage = findStorage<T>();
            if (storage == nullptr || !storage->contains(entity))
            {
                throw EcsError("World: entity has no such component");
            }

            return storage->read(entity);
        }

        template <Component T>
        void set(Entity entity, T value)
        {
            if (!isAlive(entity))
            {
                throw EcsError("World: entity is not alive");
            }

            auto &storage = storageFor<T>();
            if (!storage.contains(entity))
            {
                throw EcsError("World: entity has no such component");
            }

            storage.write(entity, value);
        }

        template <Component... Ts>
        [[nodiscard]] View<Ts...> view() const
        {
            return View<Ts...>(findStorage<Ts>()...);
        }

        template <Component T>
        void claim()
        {
            static_cast<void>(storageFor<T>());
            static_cast<void>(pendingFor<T>());
        }

        void forgetComponents() noexcept;

    private:
        friend class OpenPhase;

        void commit();

        static constexpr std::size_t kSeedSlots = 8;

        static_assert((kSeedSlots & (kSeedSlots - 1)) == 0);

        [[nodiscard]] std::size_t slotFor(
            ComponentKey key, std::string_view name);

        [[nodiscard]] std::size_t slotCapacity() const noexcept
        {
            return keys.size() / 2;
        }

        void growSlots();

        [[nodiscard]] std::size_t findSlot(
            const ComponentKey key) const noexcept
        {
            const auto slotMask = keys.size() - 1;
            auto slotIndex = static_cast<std::size_t>(key) & slotMask;

            while (keys[slotIndex] != 0)
            {
                if (keys[slotIndex] == key)
                {
                    return slotIndex;
                }

                slotIndex = (slotIndex + 1) & slotMask;
            }

            return kNoSlot;
        }

        template <Component T>
        [[nodiscard]] ComponentStorage<T> &storageFor()
        {
            const auto id = slotFor(
                detail::componentKey<T>(), detail::typeName<T>());
            if (pools[id] != nullptr)
            {
                return *static_cast<ComponentStorage<T> *>(pools[id].get());
            }

            auto pool = std::make_unique<ComponentStorage<T>>();
            auto *madePool = pool.get();
            pools[id] = std::move(pool);

            return *madePool;
        }

        template <Component T>
        [[nodiscard]] detail::PendingOps<T> &pendingFor()
        {
            const auto id = slotFor(
                detail::componentKey<T>(), detail::typeName<T>());
            if (pendingBuffers[id] != nullptr)
            {
                return *static_cast<detail::PendingOps<T> *>(
                    pendingBuffers[id].get());
            }

            auto buffer =
                std::make_unique<detail::PendingOps<T>>(storageFor<T>());
            auto *madeBuffer = buffer.get();
            pendingBuffers[id] = std::move(buffer);

            return *madeBuffer;
        }

        template <Component T>
        [[nodiscard]] const ComponentStorage<T> *findStorage() const noexcept
        {
            const auto id = findSlot(detail::componentKey<T>());
            if (id == kNoSlot || pools[id] == nullptr)
            {
                return nullptr;
            }

            return static_cast<const ComponentStorage<T> *>(pools[id].get());
        }

        void releaseEntities(std::span<const Entity> entities);

        void clearPending() noexcept;

        static constexpr std::size_t kNoSlot =
            std::numeric_limits<std::size_t>::max();

        std::unique_ptr<detail::EntityManager> entityManager;

        std::vector<ComponentKey> keys;
        std::vector<std::unique_ptr<detail::IComponentPool>> pools;
        std::vector<std::unique_ptr<detail::IPendingComponents>>
            pendingBuffers;

        std::vector<std::size_t> filledSlots;
        std::vector<Entity> pendingDestroyedEntities;
    };

}
