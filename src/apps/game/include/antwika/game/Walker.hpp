#pragma once

#include <cstdint>

#include "antwika/game/Direction.hpp"

namespace antwika::game
{

    /**
     * @brief How many ticks one cell of walking takes.
     *
     * Two, so a walker covers a cell every other tick rather than every
     * one.
     */
    inline constexpr std::uint8_t kTicksPerStep = 2;

    /**
     * @brief Something that walks the paths, and which way it is facing.
     *
     * Where it is lives in a separate Cell component, so a walker and a
     * path tile are told apart by which components they carry rather than
     * by a flag inside one shared type.
     */
    struct Walker
    {
        Direction facing = Direction::East;

        /**
         * @brief How many more ticks to wait before the next cell.
         *
         * The cadence is per walker and lives in the walker's own
         * component, rather than being a modulus on the tick number: two
         * walkers dropped a tick apart then keep their own rhythm, and
         * a replay regenerates each countdown from the same events that
         * created the walker.
         *
         * Zero on a fresh walker, so it sets off on the first tick it
         * sees.
         */
        std::uint8_t ticksUntilStep = 0;

        /**
         * @brief Compare two walkers.
         * @param other The walker to compare against.
         * @return True when both face the same way and are the same far
         * through their step.
         */
        [[nodiscard]] bool operator==(const Walker &other) const = default;
    };

} // namespace antwika::game
