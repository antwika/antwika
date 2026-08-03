#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"

namespace antwika::game
{

    /**
     * @brief How long a building burns before the fire runs out.
     *
     * Twenty seconds, which is long enough for a fireman from across
     * the city to matter and short enough that an unattended fire is
     * not a permanent fixture.
     * Derived from kTicksPerSecond like every other period, so changing
     * the pace is one edit in Building.hpp rather than one per rule.
     */
    inline constexpr std::int32_t kBurnDurationTicks =
        20 * kTicksPerSecond;

    /**
     * @brief What is left of a building once the fire has it.
     *
     * Burning first and Debris after, which is the only transition
     * there is: a fire put out early and a fire that ran its course
     * both end on Debris, and Debris ends only under the raze tool.
     */
    enum class RuinState : std::uint8_t
    {
        Burning = 0,  ///< Still alight; a fireman can be sent to it.
        Debris,       ///< Burnt out; only the raze tool clears it.
    };

    /**
     * @brief How many ruin states there are.
     */
    inline constexpr std::size_t kRuinStateCount =
        static_cast<std::size_t>(RuinState::Debris) + 1;

    /**
     * @brief Get a state's index, for addressing a per-state table.
     * @param state The state to index.
     * @return The index, always below kRuinStateCount.
     */
    [[nodiscard]] constexpr std::size_t ruinStateIndex(
        RuinState state) noexcept
    {
        return static_cast<std::size_t>(state);
    }

    /**
     * @brief Get a state's name.
     *
     * One table for BuildingKind's reason: a save file writes the name
     * rather than the index, so appending a state is free and
     * reordering is not silently wrong.
     *
     * @param state The state to name.
     * @return Its name, in the enumeration's own order.
     */
    [[nodiscard]] constexpr std::string_view ruinStateName(
        RuinState state) noexcept
    {
        constexpr std::array<std::string_view, kRuinStateCount> names{
            "burning", "debris"};

        return names[ruinStateIndex(state) % kRuinStateCount];
    }

    /**
     * @brief Get the state a name refers to.
     * @param name The name to look up.
     * @return The state, or nullopt when no state has that name.
     */
    [[nodiscard]] constexpr std::optional<RuinState> ruinStateFromName(
        std::string_view name) noexcept
    {
        for (std::size_t index = 0; index < kRuinStateCount; ++index)
        {
            const auto state = static_cast<RuinState>(index);

            if (ruinStateName(state) == name)
            {
                return state;
            }
        }

        return std::nullopt;
    }

    static_assert(ruinStateName(RuinState::Burning) == "burning");
    static_assert(ruinStateFromName("debris") == RuinState::Debris);
    static_assert(!ruinStateFromName("ashes").has_value());

    /**
     * @brief What is left where a building caught fire.
     *
     * **A component of its own rather than a state on Building, and
     * that is the whole design.** The Building entity dies at
     * ignition, so every system that walks buildings -- spawning,
     * staffing, deliveries, housing, coverage -- ignores a ruin by
     * construction rather than by a filter each of them would have to
     * remember. What survives is the block: the BuildingIndex keeps
     * the footprint claimed, which is what refuses a placement on
     * debris until the raze tool has cleared it.
     *
     * The cell it stands on is a separate Cell component, exactly as a
     * Building's is.
     */
    struct Ruin
    {
        /**
         * @brief What stood here before the fire.
         *
         * The kind rather than a footprint, for footprintOf()'s
         * reason: a copy of the footprint could disagree with the
         * block the index holds, and the kind also names the sheet
         * the debris and fire sprites are drawn from.
         */
        BuildingKind kind = BuildingKind::House;

        /** @brief Whether it is still alight. */
        RuinState state = RuinState::Burning;

        /**
         * @brief Ticks until the fire runs out on its own.
         *
         * Meaningful only while burning, and zero from the moment the
         * state is Debris -- whether the countdown ran out or a
         * fireman arrived first, so the two routes to debris leave
         * one value.
         */
        std::int32_t ticksUntilOut = kBurnDurationTicks;

        /**
         * @brief Compare two ruins.
         * @param other The ruin to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const Ruin &other) const = default;
    };

} // namespace antwika::game
