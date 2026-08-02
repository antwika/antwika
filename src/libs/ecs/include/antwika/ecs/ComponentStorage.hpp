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

    /**
     * @brief Double-buffered sparse-set storage for one component type.
     *
     * Holds every entity's T twice: a front buffer that read() always
     * returns, and a back buffer that write() always targets. Nothing in
     * this class ever hands out a mutable reference into front, which is
     * what keeps a system from being able to affect state anyone else
     * reads mid-phase — the only way front changes is commit().
     *
     * Removal is a stable, order-preserving erase (an O(n) shift, not a
     * swap-and-pop), so entities()'s order never depends on unrelated
     * removal timing — a deliberate determinism-over-throughput choice.
     *
     * **The sparse array is indexed by raw entity value and never
     * shrinks.** EntityManager hands out monotonically increasing
     * values and reuses none, so this pool's memory is O(highest entity
     * value ever inserted into it) rather than O(entities it currently
     * holds): a long session churning short-lived entities grows every
     * pool one of them touched, permanently, however few are live at
     * any moment. That is the standing cost of never recycling an index
     * — see EntityManager for why that is worth paying — and it is a
     * `std::size_t` per index rather than a T, so it is the cheap half
     * of the storage.
     *
     * Nothing in the tree has measured it as a problem, so nothing has
     * been done about it. The escape hatch when something does is a
     * paged sparse index — fixed-size pages allocated on first use,
     * with an unused page left unallocated — which keeps indexOf() O(1)
     * and costs one extra indirection. Reusing indices is not the
     * escape hatch, because that is the decision EntityManager has
     * already made the other way.
     */
    template <Component T>
    class ComponentStorage final
    {
    public:
        /**
         * @brief Add or overwrite an entity's component value.
         * @param entity The entity to store the value for.
         * @param value The value to store in both buffers.
         */
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

        /**
         * @brief Remove an entity's component value.
         * @param entity The entity to remove.
         * @throws EcsError if entity has no value stored.
         */
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

        /**
         * @brief Check whether an entity currently has a value stored.
         * @param entity The entity to check.
         * @return True if entity has a value in this storage.
         */
        [[nodiscard]] bool contains(Entity entity) const noexcept
        {
            return indexOf(entity).has_value();
        }

        /**
         * @brief Read an entity's current (front) value.
         * @param entity The entity to read.
         * @return The value from the front buffer.
         * @throws EcsError if entity has no value stored.
         */
        [[nodiscard]] const T &read(Entity entity) const
        {
            const auto index = indexOf(entity);
            if (!index.has_value())
            {
                throw EcsError("ComponentStorage: entity has no component");
            }

            return front[*index];
        }

        /**
         * @brief Stage a new value for an entity, visible after commit().
         * @param entity The entity to write.
         * @param value The value to stage into the back buffer.
         * @throws EcsError if entity has no value stored.
         */
        void write(Entity entity, T value)
        {
            const auto index = indexOf(entity);
            if (!index.has_value())
            {
                throw EcsError("ComponentStorage: entity has no component");
            }

            back[*index] = value;
        }

        /**
         * @brief List every entity with a value, in stable insertion order.
         * @return A view over the front buffer's dense entity list.
         */
        [[nodiscard]] std::span<const Entity> entities() const noexcept
        {
            return dense;
        }

        /**
         * @brief Swap front and back, then reseed back as a copy of the
         * new front, ready for the next phase's writes.
         */
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

} // namespace antwika::ecs
