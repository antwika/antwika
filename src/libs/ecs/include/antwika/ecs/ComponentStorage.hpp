#pragma once

#include <cstddef>
#include <cstdint>
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
            written.push_back(0);
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
                written[i - 1] = written[i];
                sparse[rawValue(dense[i - 1])] = i - 1;
            }

            dense.pop_back();
            front.pop_back();
            back.pop_back();
            written.pop_back();
            sparse[rawValue(entity)] = kNotPresent;
            rebuildWrittenIndices();
        }

        void removeAll(std::span<const Entity> batch)
        {
            doomed.assign(dense.size(), 0);
            bool any = false;

            for (const auto entity : batch)
            {
                const auto index = indexOf(entity);
                if (!index.has_value())
                {
                    continue;
                }

                doomed[*index] = 1;
                sparse[rawValue(entity)] = kNotPresent;
                any = true;
            }

            if (!any)
            {
                return;
            }

            writtenIndices.clear();
            std::size_t out = 0;

            for (std::size_t in = 0; in < dense.size(); ++in)
            {
                if (doomed[in] == 1)
                {
                    continue;
                }

                if (out != in)
                {
                    dense[out] = dense[in];
                    front[out] = front[in];
                    back[out] = back[in];
                    written[out] = written[in];
                    sparse[rawValue(dense[out])] = out;
                }

                if (written[out] == 1)
                {
                    writtenIndices.push_back(out);
                }

                ++out;
            }

            dense.resize(out);
            front.resize(out);
            back.resize(out);
            written.resize(out);
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

            if (written[*index] == 0)
            {
                written[*index] = 1;
                writtenIndices.push_back(*index);
            }
        }

        [[nodiscard]] std::span<const Entity> entities() const noexcept
        {
            return dense;
        }

        void commit()
        {
            front.swap(back);

            for (const auto index : writtenIndices)
            {
                back[index] = front[index];
                written[index] = 0;
            }

            writtenIndices.clear();
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

        void rebuildWrittenIndices()
        {
            writtenIndices.clear();

            for (std::size_t at = 0; at < written.size(); ++at)
            {
                if (written[at] == 1)
                {
                    writtenIndices.push_back(at);
                }
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
        std::vector<std::uint8_t> written;
        std::vector<std::size_t> writtenIndices;
        std::vector<std::uint8_t> doomed;
    };

}
