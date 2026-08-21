#pragma once

#include <span>
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
#include "antwika/ecs/View.hpp"

namespace antwika::ecs::detail
{

    template <Component T>
    class PendingOps final : public IPendingComponents
    {
    public:
        explicit PendingOps(ComponentStorage<T> &poolStorage) noexcept
            : pool(&poolStorage)
        {
        }

        void add(const Entity entity, T value)
        {
            inserts.emplace_back(entity, value);
            orderKinds.push_back(OpKind::Insert);
        }

        void remove(const Entity entity)
        {
            removedEntities.push_back(entity);
            orderKinds.push_back(OpKind::Remove);
        }

        void apply() override
        {
            if (orderKinds.empty())
            {
                return;
            }

            std::size_t nextInsert = 0;
            std::size_t nextRemove = 0;

            for (const auto kind : orderKinds)
            {
                if (kind == OpKind::Insert)
                {
                    const auto &stagedInsert = inserts[nextInsert];
                    ++nextInsert;
                    pool->insert(stagedInsert.first, stagedInsert.second);
                    continue;
                }

                const auto entity = removedEntities[nextRemove];
                ++nextRemove;
                pool->removeAll(std::span<const Entity>(&entity, 1));
            }

            clear();
        }

        void clear() noexcept override
        {
            inserts.clear();
            removedEntities.clear();
            orderKinds.clear();
        }

    private:
        ComponentStorage<T> *pool;
        std::vector<std::pair<Entity, T>> inserts;
        std::vector<Entity> removedEntities;
        std::vector<OpKind> orderKinds;
    };

}
