#include "antwika/animation/DirectionalClipSet.hpp"

#include <array>
#include <utility>

#include "antwika/animation/Clip.hpp"
#include "antwika/animation/Facing.hpp"

namespace antwika::animation
{

    DirectionalClipSet::DirectionalClipSet(
        std::array<Clip, kFacingCount> clips)
        : facingClips(std::move(clips))
    {
    }

    const Clip &DirectionalClipSet::clipFor(Facing facing) const noexcept
    {
        return facingClips[facingIndex(facing) % kFacingCount];
    }

} // namespace antwika::animation
