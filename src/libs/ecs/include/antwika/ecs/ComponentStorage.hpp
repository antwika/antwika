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
     * The sparse array is addressed by an entity's *index* alone, which
     * is what keeps it as big as the live population rather than as big
     * as every entity ever created — see Entity. A slot therefore
     * outlives the entity that claimed it, so every lookup checks the
     * dense entry really is the entity asked for: a handle from an
     * earlier generation of the same index reads as absent here, just
     * as it reads as dead from World::alive().
     */
    template <Component T>
    class ComponentStorage final
    {
    public:
        /**
         * @brief Add or overwrite an entity's component value.
         * @param entity The entity to store the value for.
         * @param value The value to store in both buffers.
         *
         * An entity claiming an index some earlier generation still
         * holds a value for drops that value rather than inheriting it.
         * A World never gets there — it purges every pool before an
         * index is freed — so this only answers for a caller driving a
         * storage directly.
         */
        void insert(Entity entity, T value)
        {
            if (const auto index = indexOf(entity); index.has_value())
            {
                front[*index] = value;
                back[*index] = value;
                return;
            }

            // The slot may still hold whoever had this index before.
            // World::retire() purges every pool before freeing one.
            // So nothing driven by a World can reach this.
            // A caller holding a storage directly still can.
            if (const auto stale = slotOf(entity); stale.has_value())
            {
                remove(dense[*stale]);
            }

            growSparseFor(entity);
            sparse[entityIndex(entity)] = dense.size();
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
                sparse[entityIndex(dense[i - 1])] = i - 1;
            }

            dense.pop_back();
            front.pop_back();
            back.pop_back();
            sparse[entityIndex(entity)] = kNotPresent;
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
            const auto index = entityIndex(entity);
            if (index >= sparse.size())
            {
                sparse.resize(index + 1, kNotPresent);
            }
        }

        // The dense slot this entity's index points at, if any.
        // Whoever occupies it -- see indexOf for the difference.
        [[nodiscard]] std::optional<std::size_t> slotOf(
            Entity entity) const noexcept
        {
            const auto index = entityIndex(entity);
            if (index >= sparse.size() || sparse[index] == kNotPresent)
            {
                return std::nullopt;
            }

            return sparse[index];
        }

        // The dense slot this exact entity occupies, if any.
        // An index outlives the entity that claimed it.
        // So a slot may belong to another generation of that index.
        // Comparing the dense entry is what tells the two apart.
        [[nodiscard]] std::optional<std::size_t> indexOf(
            Entity entity) const noexcept
        {
            const auto slot = slotOf(entity);
            if (!slot.has_value() || dense[*slot] != entity)
            {
                return std::nullopt;
            }

            return slot;
        }

        std::vector<Entity> dense;
        std::vector<T> front;
        std::vector<T> back;
        std::vector<std::size_t> sparse;
    };

} // namespace antwika::ecs
