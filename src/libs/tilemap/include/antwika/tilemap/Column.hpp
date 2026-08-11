#pragma once

#include <cstdint>
#include <vector>

#include "antwika/tilemap/Slab.hpp"

namespace antwika::tilemap
{

    inline constexpr std::int32_t kClearance = 1;

    class Column final
    {
    public:
        [[nodiscard]] const std::vector<Slab> &slabs() const noexcept;

        /**
         * @brief Finds the slab at exactly the given level.
         *
         * @param level The level to look up.
         * @return The slab at that level, or nullptr when absent.
         */
        [[nodiscard]] Slab *slabAt(std::int32_t level) noexcept;

        /**
         * @brief Reads the slab at exactly the given level.
         *
         * @param level The level to look up.
         * @return The slab at that level, or nullptr when absent.
         */
        [[nodiscard]] const Slab *slabAt(
            std::int32_t level) const noexcept;

        /**
         * @brief Finds the highest slab.
         *
         * @return The highest slab, or nullptr when the column is
         *         empty.
         */
        [[nodiscard]] Slab *top() noexcept;

        /**
         * @brief Reads the highest slab.
         *
         * @return The highest slab, or nullptr when the column is
         *         empty.
         */
        [[nodiscard]] const Slab *top() const noexcept;

        /**
         * @brief Reads the highest slab not above the given level.
         *
         * @param level The level to look down from.
         * @return The highest slab at or below the level, or nullptr
         *         when no slab sits that low.
         */
        [[nodiscard]] const Slab *topAtOrBelow(
            std::int32_t level) const noexcept;

        /**
         * @brief Tells whether the given level offers footing.
         *
         * @param level The level of the slab to stand on.
         * @return True when a slab sits at the level and no slab sits
         *         at the level directly above it.
         */
        [[nodiscard]] bool standable(std::int32_t level) const noexcept;

        /**
         * @brief Stores the slab at its level.
         *
         * @param slab The slab to store.
         * @return The stored slab.
         *
         * Ensures: the slabs stay sorted ascending by level, and a
         *          slab already at the level is replaced.
         */
        Slab &place(Slab slab);

        /**
         * @brief Drops the slab at the given level.
         *
         * @param level The level to drop.
         * @return True when a slab held the level and was removed.
         */
        bool remove(std::int32_t level);

        void clear() noexcept;

        [[nodiscard]] bool operator==(const Column &other) const
            = default;

    private:
        std::vector<Slab> slabs_{};
    };

}
