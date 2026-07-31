#pragma once

#include <array>

#include "antwika/animation/Clip.hpp"
#include "antwika/animation/Facing.hpp"

namespace antwika::animation
{

    /**
     * @brief One clip per facing, for something that is drawn
     * differently depending on which way it is going.
     *
     * A walker that turns a corner keeps its elapsed tick count and
     * changes which clip it asks, so the walk cycle carries on through
     * the turn instead of restarting.
     * That only works because a clip holds no time of its own; a player
     * object per facing would each be at a different point.
     *
     * Every clip in the set is already valid, so this constructor has
     * nothing to reject and never throws.
     */
    class DirectionalClipSet final
    {
    public:
        /**
         * @brief Build a set from one clip per facing.
         * @param clips The clips, indexed by facingIndex().
         */
        explicit DirectionalClipSet(std::array<Clip, kFacingCount> clips);

        /**
         * @brief Get the clip for one facing.
         * @param facing The facing to draw.
         * @return The clip for that facing.
         */
        [[nodiscard]] const Clip &clipFor(Facing facing) const noexcept;

    private:
        std::array<Clip, kFacingCount> facingClips;
    };

} // namespace antwika::animation
