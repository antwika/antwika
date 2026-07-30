#pragma once

#include "antwika/game/Direction.hpp"

namespace antwika::game
{

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
         * @brief Compare two walkers.
         * @param other The walker to compare against.
         * @return True when both face the same way.
         */
        [[nodiscard]] bool operator==(const Walker &other) const = default;
    };

} // namespace antwika::game
