#pragma once

#include <cstdint>
#include <optional>

#include "antwika/game/Cell.hpp"
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
         * @brief The cell this walker is stepping out of, if any.
         *
         * What a renderer needs to draw a walker part of the way between
         * two cells rather than jumping it a whole one.
         *
         * It is **simulation state and not a render-side channel**, which
         * is the distinction worth being careful about here: unlike
         * input::PointerHintChannel, a live run and its replay have to
         * agree on where a walker came from, because both of them draw
         * the same picture from it.
         *
         * Nothing but the previous cell will do. Working it back out as
         * step(at, opposite(facing)) is right in the middle of a
         * straight run and wrong exactly where there was no previous
         * cell at all -- a walker just placed, just spawned, restored
         * from a save, or sitting on a tile with no way off it. Those
         * are real states, and a plain Cell could only say so by naming
         * a cell the walker was never on.
         *
         * Absent on a fresh walker, so its first frame is drawn where it
         * stands rather than sliding in from somewhere it has never been.
         */
        std::optional<Cell> from{};

        /**
         * @brief Compare two walkers.
         * @param other The walker to compare against.
         * @return True when both face the same way, are the same far
         * through their step, and came from the same place.
         */
        [[nodiscard]] bool operator==(const Walker &other) const = default;
    };

} // namespace antwika::game
