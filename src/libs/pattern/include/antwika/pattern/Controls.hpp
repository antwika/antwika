#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "antwika/pattern/ParamId.hpp"
#include "antwika/pattern/ParamValue.hpp"

namespace antwika::pattern
{

    /** @brief One named value an event carries. */
    struct Control
    {
        ParamId id = kNoParam;
        ParamValue value;

        /**
         * @brief Compare two controls.
         * @param other The control to compare against.
         * @return True when the id and the value both match.
         */
        [[nodiscard]] bool operator==(const Control &other) const
            = default;
    };

    /**
     * @brief What an event carries, as a set of named values.
     *
     * **This is a pattern's value type, and it is not a note.**
     * A `NoteEvent` here would fuse the algebra to one domain, which is
     * the mistake antwika::wfc avoids by knowing nothing about grids and
     * antwika::pathfinding avoids by knowing nothing about tiles.
     * A pitch, a gain, a filter cutoff and a pattern's own density are
     * all one of these, differing only in an id.
     *
     * **Kept sorted by id**, so equality does not depend on the order
     * controls were set in and iteration is the same on every run.
     * A container that hashed would be neither.
     */
    class Controls final
    {
    public:
        /**
         * @brief Build a set carrying nothing.
         */
        Controls() = default;

        /**
         * @brief Build a set carrying one value.
         * @param id What to name it.
         * @param value What it holds.
         */
        Controls(ParamId id, ParamValue value);

        /**
         * @brief Name a value, replacing any this already carried.
         * @param id What to name it.
         * @param value What it holds.
         */
        void set(ParamId id, ParamValue value);

        /**
         * @brief Look a value up.
         * @param id What it was named.
         * @return The value, or nothing when this carries no such id.
         */
        [[nodiscard]] std::optional<ParamValue> get(ParamId id) const;

        /**
         * @brief Get how many values this carries.
         * @return The count.
         */
        [[nodiscard]] std::size_t size() const noexcept;

        /**
         * @brief Get whether this carries nothing at all.
         * @return True when it is empty.
         */
        [[nodiscard]] bool empty() const noexcept;

        /**
         * @brief Get everything this carries, in ascending id order.
         * @return The controls.
         */
        [[nodiscard]] const std::vector<Control> &all() const noexcept;

        /**
         * @brief Combine two sets, with the other's values winning.
         *
         * The direction is what makes `gain(0.8)` applied to a pattern
         * override a gain the pattern already carried rather than being
         * ignored by it.
         *
         * @param over The set whose values take precedence.
         * @return The combination.
         */
        [[nodiscard]] Controls mergedWith(const Controls &over) const;

        /**
         * @brief Compare two sets.
         * @param other The set to compare against.
         * @return True when they carry the same values under the same
         * ids.
         */
        [[nodiscard]] bool operator==(const Controls &other) const
            = default;

    private:
        std::vector<Control> entries;
    };

} // namespace antwika::pattern
