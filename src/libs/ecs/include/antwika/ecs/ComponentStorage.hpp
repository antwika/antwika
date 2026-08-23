#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

#include "antwika/ecs/Component.hpp"
#include "antwika/ecs/ComponentPool.hpp"
#include "antwika/ecs/EcsError.hpp"
#include "antwika/ecs/Entity.hpp"

namespace antwika::ecs
{

    template <Component T>
    class ComponentStorage final : public detail::IComponentPool
    {
    public:
        void insert(Entity entity, T value)
        {
            if (const auto index = indexOf(entity); index.has_value())
            {
                front[*index] = value;
                dense[*index] = value;
                return;
            }

            growSparseFor(entity);
            sparse[getRawValue(entity)] = denseEntities.size();
            denseEntities.push_back(entity);
            front.push_back(value);
            dense.push_back(value);
            writtenFlags.push_back(0);
        }

        void removeAll(std::span<const Entity> batchEntities) override
        {
            toRemove.assign(denseEntities.size(), 0);
            bool any = false;

            for (const auto entity : batchEntities)
            {
                const auto index = indexOf(entity);
                if (!index.has_value())
                {
                    continue;
                }

                toRemove[*index] = 1;
                sparse[getRawValue(entity)] = kNotPresent;
                any = true;
            }

            if (!any)
            {
                return;
            }

            keptWrittenIndices.clear();
            std::size_t count = 0;

            for (std::size_t index = 0; index < denseEntities.size(); ++index)
            {
                if (toRemove[index] == 1)
                {
                    continue;
                }

                if (count != index)
                {
                    denseEntities[count] = denseEntities[index];
                    front[count] = front[index];
                    dense[count] = dense[index];
                    writtenFlags[count] = writtenFlags[index];
                    sparse[getRawValue(denseEntities[count])] = count;
                }

                if (writtenFlags[count] == 1)
                {
                    keptWrittenIndices.push_back(count);
                }

                ++count;
            }

            denseEntities.resize(count);
            front.resize(count);
            dense.resize(count);
            writtenFlags.resize(count);

            writtenIndices.swap(keptWrittenIndices);
        }

        [[nodiscard]] bool contains(Entity entity) const noexcept
        {
            return indexOf(entity).has_value();
        }

        [[nodiscard]] const T &getContents(Entity entity) const
        {
            const auto index = indexOf(entity);
            if (!index.has_value())
            {
                throw EcsError("ComponentStorage: entity has no component");
            }

            return front[*index];
        }

        void write(Entity entity, T value)
        {
            const auto index = indexOf(entity);
            if (!index.has_value())
            {
                throw EcsError("ComponentStorage: entity has no component");
            }

            dense[*index] = value;

            if (writtenFlags[*index] == 0)
            {
                writtenFlags[*index] = 1;
                writtenIndices.push_back(*index);
            }
        }

        [[nodiscard]] std::span<const Entity> getEntities() const noexcept
        {
            return denseEntities;
        }

        void commit() override
        {
            front.swap(dense);

            for (const auto index : writtenIndices)
            {
                dense[index] = front[index];
                writtenFlags[index] = 0;
            }

            writtenIndices.clear();
        }

    private:
        static constexpr std::size_t kNotPresent =
            std::numeric_limits<std::size_t>::max();

        void growSparseFor(Entity entity)
        {
            const auto value = getRawValue(entity);
            if (value >= sparse.size())
            {
                sparse.resize(value + 1, kNotPresent);
            }
        }

        [[nodiscard]] std::optional<std::size_t> indexOf(
            Entity entity) const noexcept
        {
            const auto value = getRawValue(entity);
            if (value >= sparse.size() || sparse[value] == kNotPresent)
            {
                return std::nullopt;
            }

            return sparse[value];
        }

        std::vector<Entity> denseEntities;
        std::vector<T> front;
        std::vector<T> dense;
        std::vector<std::size_t> sparse;
        std::vector<std::uint8_t> writtenFlags;
        std::vector<std::size_t> writtenIndices;
        std::vector<std::size_t> keptWrittenIndices;
        std::vector<std::uint8_t> toRemove;
    };

}
