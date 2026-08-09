#pragma once

#include <cstddef>
#include <limits>
#include <optional>
#include <span>
#include <vector>

#include "antwika/ecs/Component.hpp"
#include "antwika/ecs/EcsError.hpp"
#include "antwika/ecs/Entity.hpp"

namespace antwika::ecs
{

    template <Component T>
    class ComponentStorage final
    {
    public:
        void insert(Entity entity, T value)
        {
            if (const auto index = indexOf(entity); index.has_value())
            {
                front[*index] = value;
                back[*index] = value;
                return;
            }

            growSparseFor(entity);
            sparse[rawValue(entity)] = dense.size();
            dense.push_back(entity);
            front.push_back(value);
            back.push_back(value);
        }

        void remove(Entity entity)
        {
            const auto index = indexOf(entity);
            if (!index.has_value())
            {
                throw EcsError("ComponentStorage: entity has no component");
            }

            for (auto i = *index + 1; i < dense.size(); ++i)
            {
                dense[i - 1] = dense[i];
                front[i - 1] = front[i];
                back[i - 1] = back[i];
                sparse[rawValue(dense[i - 1])] = i - 1;
            }

            dense.pop_back();
            front.pop_back();
            back.pop_back();
            sparse[rawValue(entity)] = kNotPresent;
        }

        [[nodiscard]] bool contains(Entity entity) const noexcept
        {
            return indexOf(entity).has_value();
        }

        [[nodiscard]] const T &read(Entity entity) const
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

            back[*index] = value;
        }

        [[nodiscard]] std::span<const Entity> entities() const noexcept
        {
            return dense;
        }

        void commit()
        {
            front.swap(back);
            back = front;
        }

    private:
        static constexpr std::size_t kNotPresent =
            std::numeric_limits<std::size_t>::max();

        void growSparseFor(Entity entity)
        {
            const auto value = rawValue(entity);
            if (value >= sparse.size())
            {
                sparse.resize(value + 1, kNotPresent);
            }
        }

        [[nodiscard]] std::optional<std::size_t> indexOf(
            Entity entity) const noexcept
        {
            const auto value = rawValue(entity);
            if (value >= sparse.size() || sparse[value] == kNotPresent)
            {
                return std::nullopt;
            }

            return sparse[value];
        }

        std::vector<Entity> dense;
        std::vector<T> front;
        std::vector<T> back;
        std::vector<std::size_t> sparse;
    };

}
