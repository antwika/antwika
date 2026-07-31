#pragma once

#include <cstdint>

#include "antwika/game/BuildTool.hpp"

namespace antwika::game
{

    /**
     * @brief How many ticks a building takes to send somebody out.
     *
     * Twenty, which at the app's 40 ms tick is a walker every four fifths
     * of a second per building -- brisk enough that one house makes the
     * roads look used, and slow enough that a row of them does not fill
     * the grid in a breath. It is one number here rather than a rate per
     * kind, because a second number is a second thing to tune before
     * anybody has said the first one is wrong.
     */
    inline constexpr std::uint8_t kTicksPerSpawn = 20;

    /**
     * @brief Check whether a building sends walkers out at all.
     *
     * Houses and shops do: both are places people come and go from. A
     * tower does not -- it is what a road is defended from rather than a
     * place anybody lives -- and Road is not a building.
     *
     * @param kind The building's kind.
     * @return True for the kinds that spawn.
     */
    [[nodiscard]] constexpr bool spawnsWalkers(BuildTool kind) noexcept
    {
        return kind == BuildTool::House || kind == BuildTool::Shop;
    }

    /**
     * @brief Marks an entity's cell as built on, and says with what.
     *
     * The tool that placed it is kept rather than a kind of its own, so
     * the palette button, the placement and the picture cannot name three
     * different things.
     *
     * The cell it stands on is a separate Cell component, the same way a
     * Path's is, so the two can be viewed together.
     */
    struct Building
    {
        BuildTool kind = BuildTool::House;

        /**
         * @brief How many more ticks before it sends somebody out.
         *
         * Per building and held in the building's own component, rather
         * than a modulus on the tick number, for exactly the reason
         * Walker::ticksUntilStep is: two houses placed a tick apart would
         * otherwise spawn in lockstep for ever, and a replay regenerates
         * each countdown from the same click that created the building.
         *
         * One short of the interval on a fresh building, exactly as
         * WalkerSystem leaves a walker that has just stepped: a house
         * placed now sends its first walker out kTicksPerSpawn ticks
         * later rather than one tick after that.
         */
        std::uint8_t ticksUntilSpawn =
            static_cast<std::uint8_t>(kTicksPerSpawn - 1);

        /**
         * @brief Compare two buildings.
         * @param other The building to compare against.
         * @return True when both were placed by the same tool and are
         * the same far through their wait.
         */
        [[nodiscard]] bool operator==(const Building &other) const = default;
    };

} // namespace antwika::game
