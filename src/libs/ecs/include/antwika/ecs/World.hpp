#pragma once

#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <span>
#include <utility>
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

    enum class OpKind : std::uint8_t
    {
        Insert,
        Remove,
    };

    template <Component T>
    struct PendingOps final
    {
        std::vector<std::pair<Entity, T>> inserts;
        std::vector<Entity> removes;
        std::vector<OpKind> order;

        void clear() noexcept
        {
            inserts.clear();
            removes.clear();
            order.clear();
        }
    };
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

            auto &pending = pendingFor<T>();
            pending.inserts.emplace_back(entity, value);
            pending.order.push_back(detail::OpKind::Insert);
        }

        template <Component T>
        void remove(Entity entity)
        {
            if (!alive(entity))
            {
                throw EcsError("World: entity is not alive");
            }

            auto &pending = pendingFor<T>();
            pending.removes.push_back(entity);
            pending.order.push_back(detail::OpKind::Remove);
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
        [[nodiscard]] detail::PendingOps<T> &pendingFor()
        {
            const auto id = detail::componentId<T>();
            if (id < buffers.size() && buffers[id] != nullptr)
            {
                return *static_cast<detail::PendingOps<T> *>(
                    buffers[id].get());
            }

            if (id >= buffers.size())
            {
                buffers.resize(id + 1);
            }

            auto buffer = std::make_shared<detail::PendingOps<T>>();
            auto *rawPtr = buffer.get();
            buffers[id] = std::move(buffer);

            drainCallbacks.push_back(
                [this, rawPtr] // GCOVR_EXCL_LINE
                { drain<T>(*rawPtr); });
            clearCallbacks.push_back(
                [rawPtr] // GCOVR_EXCL_LINE
                { rawPtr->clear(); });

            return *rawPtr;
        }

        template <Component T>
        void drain(detail::PendingOps<T> &pending)
        {
            if (pending.order.empty())
            {
                return;
            }

            auto &storage = storageFor<T>();
            std::size_t nextInsert = 0;
            std::size_t nextRemove = 0;

            for (const auto kind : pending.order)
            {
                if (kind == detail::OpKind::Insert)
                {
                    const auto &staged = pending.inserts[nextInsert];
                    ++nextInsert;

                    if (alive(staged.first))
                    {
                        storage.insert(staged.first, staged.second);
                    }

                    continue;
                }

                const auto entity = pending.removes[nextRemove];
                ++nextRemove;

                if (storage.contains(entity))
                {
                    storage.remove(entity);
                }
            }

            pending.clear();
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

        void clearPending() noexcept;

        std::unique_ptr<detail::EntityManager> entityManager;
        std::vector<std::shared_ptr<void>> pools;
        std::vector<std::shared_ptr<void>> buffers;
        std::vector<std::function<void()>> commitCallbacks;
        std::vector<std::function<void(std::span<const Entity>)>>
            removeFromAllPools;
        std::vector<std::function<void()>> drainCallbacks;
        std::vector<std::function<void()>> clearCallbacks;
        std::vector<Entity> pendingDestroys;
    };

}
