#pragma once

#include "antwika/input/Position.hpp"

namespace antwika::input
{

    /**
     * @brief Where the pointer is, as a value no recording holds.
     *
     * A distinct type from a bare Position on purpose: a Position that
     * came off an input.* event is state a replay reproduces, and one
     * that came off a PointerHintChannel is not. Wrapping the second
     * means the two cannot be handed to the same function by accident,
     * and that every place one is unwrapped says which it had.
     */
    struct PointerHint
    {
        Position position{};

        /**
         * @brief Compare two hints.
         * @param other The hint to compare against.
         * @return True when both report the same position.
         */
        [[nodiscard]] bool operator==(const PointerHint &other) const = default;
    };

} // namespace antwika::input
