#pragma once

#include <cstddef>

#include <antwika/time/Tick.hpp>

namespace antwika::animation
{

    /**
     * @brief One frame of a clip: what to show, and for how long.
     *
     * The index is the caller's own number and this library never looks
     * inside it.
     * An app addressing a texture atlas puts a slot number here; an app
     * with four separate images puts 0 to 3.
     * That is what keeps antwika::gfx off this library's dependency
     * list.
     */
    struct KeyFrame final
    {
        /**
         * @brief What to show, in whatever numbering the caller uses.
         */
        std::size_t index{0};

        /**
         * @brief How many ticks this frame stays up, never zero.
         */
        time::Tick durationTicks{1};

        /**
         * @brief Compare two key frames field by field.
         * @param other The key frame to compare against.
         * @return Whether both fields match.
         */
        [[nodiscard]] bool operator==(
            const KeyFrame &other) const noexcept = default;
    };

} // namespace antwika::animation
