#pragma once

#include <cstddef>

#include "antwika/animation/Progress.hpp"

namespace antwika::animation
{

    /**
     * @brief What a clip is showing at some tick.
     *
     * An index and a fraction, and nothing else: no rectangle, no
     * texture and no colour, since this library cannot name any of those
     * without depending on antwika::gfx.
     * Turning the index into an atlas slot is the caller's job, and the
     * caller is the only one that knows the picture.
     */
    struct Frame final
    {
        /**
         * @brief The index of the key frame being shown, in the
         * caller's own numbering.
         */
        std::size_t index{0};

        /**
         * @brief How far through that key frame's own duration the tick
         * fell.
         *
         * Always below one while a clip is still running, and exactly
         * one once a one-shot clip has finished.
         */
        Progress progress{};

        /**
         * @brief Whether a one-shot clip has run past its last frame.
         *
         * Never true for a looping clip, which has no end to run past.
         */
        bool finished{false};

        /**
         * @brief Compare two frames field by field.
         * @param other The frame to compare against.
         * @return Whether every field matches.
         */
        [[nodiscard]] bool operator==(
            const Frame &other) const noexcept = default;
    };

} // namespace antwika::animation
