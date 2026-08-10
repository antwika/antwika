#pragma once

#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <span>
#include <vector>

#include <antwika/log/ILogger.hpp>

#include "antwika/ecs/Component.hpp"
#include "antwika/ecs/ComponentId.hpp"
#include "antwika/ecs/ComponentStorage.hpp"
#include "antwika/ecs/EcsError.hpp"
#include "antwika/ecs/Entity.hpp"
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

        [[nodiscard]] bool alive(Entity entity) const noexcept;

        template <Component T>
        void add(Entity entity, T value)
        {
            if (!alive(entity))
            {
                throw EcsError("World: entity is not alive");
            }

            pendingOps.push_back(
                [this, entity, value] // GCOVR_EXCL_LINE
                {
                    if (!alive(entity))
                    {
                        return;
                    }
                    storageFor<T>().insert(entity, value);
                });
        }

        template <Component T>
        void remove(Entity entity)
        {
            if (!alive(entity))
            {
                throw EcsError("World: entity is not alive");
            }

            pendingOps.push_back(
                [this, entity]
                {
                    auto &storage = storageFor<T>();
                    if (storage.contains(entity))
                    {
                        storage.remove(entity);
                    }
                });
        }

        template <Component T>
        [[nodiscard]] bool has(Entity entity) const noexcept
        {
            if (!alive(entity))
            {
                return false;
            }

            const auto *storage = findStorage<T>();
            return storage != nullptr && storage->contains(entity);
        }

        template <Component T>
        [[nodiscard]] const T &get(Entity entity) const
        {
            if (!alive(entity))
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
            if (!alive(entity))
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

        void commit();

    private:
        template <Component T>
        [[nodiscard]] ComponentStorage<T> &storageFor()
        {
            const auto id = detail::componentId<T>();
            if (id < pools.size() && pools[id] != nullptr)
            {
                return *static_cast<ComponentStorage<T> *>(pools[id].get());
            }

            if (id >= pools.size())
            {
                pools.resize(id + 1);
            }

            auto storage = std::make_shared<ComponentStorage<T>>();
            auto *rawPtr = storage.get();
            pools[id] = std::move(storage);

            commitCallbacks.push_back(
                [rawPtr] // GCOVR_EXCL_LINE
                { rawPtr->commit(); });
            removeFromAllPools.push_back(
                [rawPtr](std::span<const Entity> batch) // GCOVR_EXCL_LINE
                { rawPtr->removeAll(batch); });

            return *rawPtr;
        }

        template <Component T>
        [[nodiscard]] const ComponentStorage<T> *findStorage() const noexcept
        {
            const auto id = detail::componentId<T>();
            if (id >= pools.size() || pools[id] == nullptr)
            {
                return nullptr;
            }

            return static_cast<const ComponentStorage<T> *>(pools[id].get());
        }

        void retireAll(std::span<const Entity> entities);

        std::unique_ptr<detail::EntityManager> entityManager;
        std::vector<std::shared_ptr<void>> pools;
        std::vector<std::function<void()>> commitCallbacks;
        std::vector<std::function<void(std::span<const Entity>)>>
            removeFromAllPools;
        std::vector<std::function<void()>> pendingOps;
        std::vector<Entity> pendingDestroys;
    };

}
